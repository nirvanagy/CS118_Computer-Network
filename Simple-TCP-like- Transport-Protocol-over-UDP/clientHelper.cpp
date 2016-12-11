#include "TcpPacket.h"
#include "clientHelper.h"

using namespace std;

void Client::init(const char *hostName, int portNum) {
    m_filefd = creat("received.data", 0666);
    if(m_filefd == -1) {
        cerr << "File creating failed!" << endl;
        exit(EXIT_FAILURE);
    }else {
        cout << "File creating succeeded!" << endl;
    }
    for(int i = 0; i < m_maxNoOfPackets; i++)
        m_buffWrittedIdx[i] = false;

    struct hostent *server = gethostbyname(hostName);
    if(server == NULL) {
        cerr << "Error: host isn't existed!" << endl;
        exit(EXIT_FAILURE);
    }
    memset((char*)&m_serverAddr, 0, sizeof(m_serverAddr));
    m_serverAddr.sin_family = AF_INET;
    m_serverAddr.sin_port = htons(portNum);
    memcpy((void*)&m_serverAddr.sin_addr, server->h_addr_list[0], server->h_length);

    m_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(m_sockfd < 0) {
        cerr << "Error: socket creating failed!" << endl;
        exit(EXIT_FAILURE);
    }
    cout << "Socket creating succeeded!" << endl;

    // set timeout
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = TIME_OUT_MSEC;
    if(setsockopt(m_sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv,sizeof(tv)) < 0) {
        cerr << "Error: set timeout failed" << endl;
        exit(EXIT_FAILURE);
    }

    m_seqNo = getRandSeqNo();
    m_seqFinNoServer = 40000;
    m_isServerFin = false;
    m_isLastFilePacket = false;
    m_lastFilePacketSize = 0;
}

void Client::connect() {
    bool rtrSYN = false;
    bool isConnect = false;
    TcpPacket packet[2];

    while(!isConnect) {
        if(rtrSYN)
            cout << "Sending packet " << m_ackNo << " Retransmission SYN" << endl;
        else
            cout << "Sending packet " << m_ackNo << " SYN" << endl;

        packet[0].header.seqNo = htons(m_seqNo);
        packet[0].header.flags = TCP_SYN;

        if(sendto(m_sockfd, &packet[0], sizeof(packet[0]), 0, (struct sockaddr*)&m_serverAddr, sizeof(m_serverAddr)) < 0) {
            cerr << "Error: send SYN packet failed!" << endl;
            exit(EXIT_FAILURE);
        }

        socklen_t addrlen = sizeof(m_serverAddr);
        ssize_t rc;
        if((rc = recvfrom(m_sockfd,  &packet[1], sizeof(packet[1]), 0, (struct sockaddr*)&m_serverAddr, & addrlen)) < 0){
            if(errno == EAGAIN || errno == EWOULDBLOCK) {
                rtrSYN = true;  // Timeout
                continue;  // The syn packet lost, need to retransmit syn
            } else {
                cerr << "Error: reveive packet failed!" << endl;
                exit(EXIT_FAILURE);
            }
        }

        // If run here, the syn packet has been sent
        packet[1].header.seqNo = ntohs(packet[1].header.seqNo);
        packet[1].header.ackNo = ntohs(packet[1].header.ackNo);
        cout << "Receiving packet " << packet[1].header.seqNo << endl;

        if (getEventType(packet[1]) == RECEIVE_SYNACK && packet[1].header.ackNo == m_seqNo + 1){
            m_seqNo = m_seqNo + 1;
            m_baseNo = m_ackNo = packet[1].header.seqNo + 1;

            if (sendACK(false) < 0) {
                cerr << "Error: send SYNACK packet failed!" << endl;
                exit(EXIT_FAILURE);
            }

            isConnect = true;
        }else {
            cerr << "Error: receive bad packet from server!" << endl;
            exit(EXIT_FAILURE);
        }

    }

}
void Client::receive() {
    while(!m_isServerFin) {
        TcpPacket packet;
        socklen_t addrlen = sizeof(m_serverAddr);
        ssize_t rc;

        rc = recvfrom(m_sockfd, &packet, sizeof(packet), 0, (struct sockaddr*) &m_serverAddr, &addrlen);
        if(rc < 0) {
            if(errno == EAGAIN || errno == EWOULDBLOCK)
                continue;   // Timeout;
            else {
                cerr << "Error: reveive packet failed!" << endl;
                exit(EXIT_FAILURE);
            }
        }
        packet.header.seqNo = ntohs(packet.header.seqNo);
        packet.header.ackNo = ntohs(packet.header.ackNo);
        cout << "Receiving packet " << packet.header.seqNo << endl;

        // check whether did I receive the last packet
        if(getEventType(packet) == RECEIVE_ACK && rc - 8 < MAX_PKT_LEN) {
            m_lastFilePacketSize = rc - 8;
            m_isLastFilePacket = true;
        }

        if(packet.header.seqNo == m_ackNo) {
            // receive next right packet
            writeBuff(packet, rc - 8);
            updateBuff();
            sendACK(false);
        }else if(packet.header.seqNo < m_ackNo){
            // receive out-of-order packet
            if(packet.header.seqNo + MAX_RCV_WIN - m_ackNo < CLIENT_RCVWS) {
                writeBuff(packet, rc - 8);
                sendACK(false);
            }else {
                sendACK(true);
            }
        }else {
            // receive out-of-order packet
            if(packet.header.seqNo - m_ackNo < CLIENT_RCVWS)
                writeBuff(packet, rc - 8);
            sendACK(false);
        }
    }
}

void Client::finish() {
    bool rtrFIN = false;
    bool canBeClose = false;
    TcpPacket packet[2];

    while(!canBeClose) {
        if(rtrFIN)
            cout << "Sending packet " << m_ackNo << " Retransmission FIN" << endl;
        else
            cout << "Sending packet " << m_ackNo << " FIN" << endl;

        packet[0].header.ackNo = htons(m_ackNo);
        packet[0].header.seqNo = htons(m_seqNo);
        packet[0].header.flags = TCP_FINACK;

        if(sendto(m_sockfd, &packet[0], sizeof(packet[0]), 0, (struct sockaddr*)&m_serverAddr, sizeof(m_serverAddr)) < 0) {
            cerr << "Error: send FIN packet failed!" << endl;
            exit(EXIT_FAILURE);
        }

        socklen_t addrlen = sizeof(m_serverAddr);
        ssize_t rc;
        if((rc = recvfrom(m_sockfd,  &packet[1], sizeof(packet[1]), 0, (struct sockaddr*)&m_serverAddr, & addrlen)) < 0){
            if(errno == EAGAIN || errno == EWOULDBLOCK) {
                rtrFIN = true;  // Timeout
                continue;  // The syn packet lost, need to retransmit syn
            } else {
                cerr << "Error: reveive packet failed!" << endl;
                exit(EXIT_FAILURE);
            }
        }
        packet[1].header.seqNo = ntohs(packet[1].header.seqNo);
        packet[1].header.ackNo = ntohs(packet[1].header.ackNo);
        if(getEventType(packet[1]) == RECEIVE_ACK && packet[1].header.ackNo == m_seqNo + 1) {
            canBeClose = true;
        }
    }
    close(m_sockfd);
    close(m_filefd);
}

ssize_t Client::sendACK(bool rst) {
    TcpPacket packet;
    packet.header.ackNo = htons(m_ackNo);
    packet.header.seqNo = htons(m_seqNo);
    packet.header.rcvWin = htons(CLIENT_RCVWS);

    if(m_isServerFin) {
        return 0;
    }else {
        packet.header.flags = TCP_ACK;
        if (rst)
            cout << "Sending packet " << ntohs(packet.header.ackNo) << " Retransmission" << endl;
        else
            cout << "Sending packet " << ntohs(packet.header.ackNo) << endl;

    }
    ssize_t rc = sendto(m_sockfd, &packet.header, sizeof(packet.header), 0,  (struct sockaddr*)&m_serverAddr, sizeof(m_serverAddr));
    return rc;
}

void Client::increaseAckNo(ssize_t pktDataSize) {
    if(m_ackNo + pktDataSize < MAX_RCV_WIN) {
        m_ackNo += pktDataSize;
    }else {
        m_ackNo += pktDataSize - MAX_RCV_WIN;
    }
}

void Client::writeBuff(const TcpPacket& packet, ssize_t pktDataSize) {
    // check if server sends fin or not
    if(getEventType(packet) == RECEIVE_FIN) {
        m_seqFinNoServer = packet.header.seqNo;
    }
    // copy the payload into buffer and mark writtenIdx for controlling order
    if(packet.header.seqNo >= m_baseNo) {
        // needn't wrap around seqNo
        memcpy(&m_buff[packet.header.seqNo - m_baseNo], packet.payload, pktDataSize);
        m_buffWrittedIdx[(packet.header.seqNo - m_baseNo) / MAX_PKT_LEN] = true;
    }else {
        // need to wrap around seqNo
        memcpy(&m_buff[packet.header.seqNo + MAX_SEQ_NUM + 1 - m_baseNo], packet.payload, pktDataSize);
        m_buffWrittedIdx[(packet.header.seqNo + MAX_SEQ_NUM + 1 - m_baseNo) / MAX_PKT_LEN] = true;
    }
}

void Client::updateBuff() {
    bool canWriteFile = m_ackNo >= m_baseNo ? (m_buffWrittedIdx[(m_ackNo - m_baseNo) / MAX_PKT_LEN]) : (m_buffWrittedIdx[(m_ackNo + MAX_SEQ_NUM + 1 - m_baseNo) / MAX_PKT_LEN]);

    while(canWriteFile) {
        if(m_ackNo == m_seqFinNoServer) {
            increaseAckNo(1);
            m_isServerFin = true;
            break;
        }else {
            ssize_t sizeToWrite;
            ssize_t writtenBytes;

            if(m_isLastFilePacket)
                sizeToWrite = m_lastFilePacketSize;
            else
                sizeToWrite = MAX_PKT_LEN;

            writtenBytes = writeFile(sizeToWrite);

            if(writtenBytes  == -1) {
                cerr << "Error: File writting failed" << endl;
                exit(EXIT_FAILURE);
            }
            if (m_ackNo >= m_baseNo)
                m_buffWrittedIdx[(m_ackNo - m_baseNo) / MAX_PKT_LEN] = false;
            else
                m_buffWrittedIdx[(m_ackNo + MAX_ACK_NUM + 1 - m_baseNo) / MAX_PKT_LEN] = false;
            //cout << "m_ackNo Before " << m_ackNo << endl;
            increaseAckNo(writtenBytes);
            //cout << "m_ackNo After " << m_ackNo << endl;
        }

        canWriteFile = m_ackNo >= m_baseNo ? (m_buffWrittedIdx[ (m_ackNo - m_baseNo) / MAX_PKT_LEN]) : (m_buffWrittedIdx[ (m_ackNo + MAX_SEQ_NUM + 1 - m_baseNo) / MAX_PKT_LEN]);
    }


}
ssize_t Client::writeFile(ssize_t pktDataSize) {
    ssize_t writtenBytes;
    if(m_ackNo >= m_baseNo) {
        writtenBytes = write(m_filefd, &m_buff[m_ackNo - m_baseNo], pktDataSize);
    }else {
        writtenBytes = write(m_filefd, &m_buff[m_ackNo - m_baseNo + MAX_SEQ_NUM + 1], pktDataSize);
    }
    return writtenBytes;
}