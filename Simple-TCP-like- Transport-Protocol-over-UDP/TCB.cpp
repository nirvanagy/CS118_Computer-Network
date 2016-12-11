
#include "TCB.h"

using namespace std;

TCB::TCB(string f, int port_num) {
    this->filePath = f;
    this->portNum = port_num;
}

void TCB::init() {
    fd = open(filePath.data(), O_RDONLY);
    if (fd < 0) {
        perror("file open fail");
        exit(-1);
    }
    struct stat st;
    stat(filePath.data(), &st);
    fileLen = st.st_size;
    filePointer = 0;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Socket create fail");
        exit(-1);
    }
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(portNum);
    addr.sin_addr.s_addr = INADDR_ANY;
    int yes = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
        perror("setsockopt");
        exit(1);
    }
    if (::bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("Bind fail");
        exit(-1);
    }

    seqNo = getRandSeqNo();
    base = seqNo+1;
    cwnd = INITIAL_WIN_SIZE;
    wnd = cwnd;
    ssthresh = INITIAL_SSTHRESH;
    rcvWin = INITIAL_SSTHRESH;
}

void TCB::accept() {
    TcpPacket recv_packet;
    socklen_t  client_len = sizeof(struct sockaddr);
    int len = 0;
    int event = 0;

    while ((len = recvfrom(sockfd, &recv_packet, sizeof(recv_packet), 0, (struct sockaddr*)&client_addr, &client_len))) {
        if (len < 0) {
            perror("recvfrom fail");
            exit(-1);
        }
        recv_packet.header.seqNo = ntohs(recv_packet.header.seqNo);
        recv_packet.header.ackNo = ntohs(recv_packet.header.ackNo);
        recv_packet.header.rcvWin = ntohs(recv_packet.header.rcvWin);
        rcvWin = recv_packet.header.rcvWin;
        ackNo = recv_packet.header.seqNo + 1;
        event = getEventType(recv_packet);
        // cout << "event: " << event << endl;
        if (event == RECEIVE_SYN) {
            // cout << "SYN received" << endl;
            TcpPacket send_packet;
            send_packet.header.ackNo = ackNo;
            send_packet.header.seqNo = seqNo;
            send_packet.header.flags |= (TCP_SYN + TCP_ACK);
            cout << "Sending packet " << seqNo << " " << cwnd << " " << ssthresh << " SYN" << endl;
            len = send_packet_header(send_packet);
            if (len < 0) {
                perror("send header");
                exit(-1);
            }
            enable_socket_timeout(0, TIME_OUT_MSEC);
            while ((len = recvfrom(sockfd, &recv_packet, sizeof(recv_packet), 0, (struct sockaddr*) &client_addr, &client_len))) {
                if (len < 0) {
                    cout << "Sending packet " << seqNo << " " << cwnd << " " << ssthresh << " Retransmission SYN" << endl;
                    send_packet_header(send_packet);
                } else {
                    recv_packet.header.ackNo = ntohs(recv_packet.header.ackNo);
                    recv_packet.header.seqNo = ntohs(recv_packet.header.seqNo);
                    if (getEventType(recv_packet) == RECEIVE_ACK && recv_packet.header.ackNo == seqNo+1) {
                        seqNo += 1;
                        break;
                    } else {
                        send_packet_header(send_packet);
                    }
                }
            }
            break;
        }
    }
}

void TCB::send() {
    TcpPacket recv_packet;
    int len = 0;

    send_file();
    while (true) {
        len = recvfrom(sockfd, &recv_packet, sizeof(recv_packet), 0, (struct sockaddr*) &client_addr, &client_len);
        if (len < 0) {
            update_windows_size(true);
            if (!buffer.empty()) {
                if (send_packet(buffer.front()->packet, buffer.front()->len) < 0) {
                    perror("retransmit fail");
                    exit(-1);
                }
                cout << "Sending packet " << buffer.front()->packet.header.seqNo << " " << wnd << " " << ssthresh << " Retransmission" << endl;
                // cout << "buffer end " <<buffer.back()->packet.header.seqNo << endl;
                buffer.front()->timestamp = get_timestamp();
            }
        } else {
            recv_packet.header.ackNo = ntohs(recv_packet.header.ackNo);
            rcvWin = ntohs(recv_packet.header.rcvWin);
            // cout << "receive windows size: " << rcvWin << endl;
            cout << "Receiving packet " << recv_packet.header.ackNo << endl;
            if (within_buffer(recv_packet.header.ackNo)) {
                // cout << "in buffer" << endl;
                while (!buffer.empty() && buffer.front()->packet.header.seqNo != recv_packet.header.ackNo) {
                    delete buffer.front();
                    buffer.pop();
                }
                seqNo = recv_packet.header.ackNo;
            } else {
                // cout << "not int buffer" << endl;
                // cout << "next seq: " << buffer.back()->packet.header.seqNo+buffer.back()->len << endl;
                if (recv_packet.header.ackNo == (buffer.back()->packet.header.seqNo+buffer.back()->len)%(MAX_SEQ_NUM+1)){
                    // cout << "will pop" << endl;
                    while (!buffer.empty()) {
                        delete buffer.front();
                        buffer.pop();
                    }
                    seqNo = recv_packet.header.ackNo;
                }
            }
            update_windows_size(false);
            // cout << "after updating: " << wnd << endl;
            send_file();
        }
        if (filePointer == fileLen && buffer.empty()) break;
    }
}

void TCB::tear_down() {
    TcpPacket send_packet,recv_packet;
    int len = 0;
    send_packet.header.seqNo = seqNo;
    send_packet.header.ackNo = ackNo;
    send_packet.header.flags = TCP_FIN;
    if (send_packet_header(send_packet) < 0) {
        perror("send fin fail");
        exit(-1);
    }
    cout << "Sending packet "<< send_packet.header.seqNo << " " << wnd << " " << ssthresh << " FIN" << endl;
    while ((len = recvfrom(sockfd, &recv_packet, sizeof(recv_packet), 0, (struct sockaddr*) &client_addr, &client_len))) {
        if (len < 0) {
            if (send_packet_header(send_packet) < 0) {
                perror("resend fin fail");
                exit(-1);
            }
            cout << "Sending packet "<< send_packet.header.seqNo << " " << wnd << " " << ssthresh << " Retransmission FIN" << endl;
        } else {
            int event = getEventType(recv_packet);
            if (event == RECEIVE_FINACK) {
                send_packet.header.flags = TCP_ACK;
                recv_packet.header.seqNo = ntohs(recv_packet.header.seqNo);
                send_packet.header.ackNo = recv_packet.header.seqNo+1;
                if (send_packet_header(send_packet) < 0) {
                    perror("send ack fail");
                    exit(-1);
                }
                enable_socket_timeout(2*MSL, 0);
                while (recvfrom(sockfd, &recv_packet, sizeof(recv_packet), 0, (struct sockaddr*) &client_addr, &client_len) > 0) {
                    if (getEventType(recv_packet) == RECEIVE_FINACK) {
                        if (send_packet_header(send_packet) < 0) {
                            perror("send ack fail");
                            exit(-1);
                        }
                    }
                }
                close(sockfd);
                break;
            }
        }
    }
}

void TCB::send_file() {
    TcpPacket packet;
    int tmp1 = 0;
    if (filePointer >= fileLen) return;
    int seq = buffer.empty() ? seqNo : (buffer.back()->packet.header.seqNo+buffer.back()->len)%(MAX_SEQ_NUM+1);

    while (filePointer < fileLen && (buffer.size()+1)*MAX_PKT_LEN <= wnd) {
        int len = min(MAX_PKT_LEN, fileLen-filePointer);
        int tmp = read(fd, &packet.payload, len);
        if (tmp < 0) {
            perror("read file fail");
            exit(-1);
        }
        filePointer += tmp;
        packet.header.ackNo = ackNo;
        packet.header.seqNo = seq;
        packet.header.flags = TCP_ACK;
        seq = (seq+tmp)%(MAX_SEQ_NUM+1);
        if ((tmp1 = send_packet(packet, len)) < 0) {
            perror("send error");
            exit(-1);
        }
        // cout << "send actual packet length: " << tmp1 << endl;
        cout << "Sending packet " << packet.header.seqNo << " " << wnd << " " << ssthresh << endl;
        Buffer* buf = new Buffer();
        buf->timestamp = get_timestamp();
        buf->packet = packet;
        buf->len = tmp;
        buffer.push(buf);
    }
}

int TCB::send_packet(TcpPacket packet, int len) {
    packet.header.ackNo = htons(packet.header.ackNo);
    packet.header.seqNo = htons(packet.header.seqNo);
    // cout << "send packet length: " << len << endl;
    return ::sendto(sockfd, &packet, 8+len, 0, (struct sockaddr*)&client_addr, sizeof(client_addr));
}

int TCB::send_packet_header(TcpPacket packet) {
    packet.header.ackNo = htons(packet.header.ackNo);
    packet.header.seqNo = htons(packet.header.seqNo);
    return ::sendto(sockfd, &packet.header, sizeof(packet.header), 0, (struct sockaddr*)&client_addr, sizeof(client_addr));
}

void TCB::enable_socket_timeout(int second, int usec) {
    struct timeval tv;
    tv.tv_sec = second;
    tv.tv_usec = usec;
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
        perror("setsocketopt fail");
        exit(-1);
    }
}

long int TCB::get_timestamp() {
    struct timeval tp;
    gettimeofday(&tp, NULL);
    long int ms = tp.tv_sec * 1000 + tp.tv_usec / 1000;
    return ms;
}

bool TCB::within_buffer(int seq) {
    if (buffer.empty()) return false;
    int start = buffer.front()->packet.header.seqNo;
    int end = buffer.back()->packet.header.seqNo;
    if (end >= start) {
        if (seq >= start && seq <= end) return true;
        return false;
    } else {
        if (seq >= start || seq <= end) return true;
        return false;
    }
}

void TCB::update_windows_size(bool timeout){
    int curWin;
    if(timeout){
        ssthresh=max(1/2*cwnd,INITIAL_WIN_SIZE);
        cwnd=INITIAL_WIN_SIZE+1024;
        curWin=min(rcvWin,cwnd);
        curWin=curWin-curWin%1024;
    }
    else{
        if(cwnd<=ssthresh){
            cwnd+=1024;
        }
        else{
            cwnd+=1024*1024/cwnd;
        }
        curWin=min(rcvWin,cwnd);
        curWin=curWin-curWin%1024;
    }
    wnd = max(curWin,1024);
}
