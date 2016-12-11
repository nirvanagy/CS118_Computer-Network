
// Tcp Control Block
//
#ifndef CS118PROJECT_TCB_H
#define CS118PROJECT_TCB_H

#define SSTHRESH 15360
#define TIMEOUT 500000

#include "TcpPacket.h"
#include <string>
#include <queue>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <iostream>
#include <sys/time.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <math.h>
#include <sys/time.h>

using namespace std;

#define TIME_OUT_MSEC 500000
#define INITIAL_WIN_SIZE 1024
#define INITIAL_SSTHRESH 15360
#define MSL 60

class TCB {

public:
    struct Buffer {
        TcpPacket packet;
        long int timestamp;
        int len;
    };
    TCB(string f, int port_num);

    void init();
    void accept();
    void send();
    void tear_down();

    string filePath;
    int fd = 0;
    int fileLen = 0;
    int filePointer = 0;

    int portNum = 0;
    int sockfd = 0;

    uint16_t cwnd;
    uint16_t wnd;
    uint16_t ssthresh;
    uint16_t seqNo;
    uint16_t ackNo;
    uint16_t base;
    uint16_t ackedSeqNo;
    queue<Buffer*> buffer;

    uint16_t rcvWin = 0;
    struct sockaddr client_addr;
    socklen_t  client_len = sizeof(struct sockaddr);

    void send_file();
    int send_packet(TcpPacket packet, int len);
    int send_packet_header(TcpPacket packet);
    void re_transmit();

    void enable_socket_timeout(int second, int usec);

    void update_windows_size(bool timeout);
    long int get_timestamp();
    bool within_buffer(int seq);
};

#endif //CS118PROJECT_TCB_H
