//
// Created by 陈昊 on 11/22/16.
//
#include <stdlib.h>
#include <random>

#ifndef CS118PROJECT_TCPPACKET_H
#define CS118PROJECT_TCPPACKET_H

#define MAX_PKT_LEN 1024
#define MAX_RCV_WIN	30720
#define MAX_SEQ_NUM 30719
#define MAX_ACK_NUM	30719
#define CLIENT_RCVWS 15360

#define TCP_FIN 1
#define TCP_SYN 2
#define TCP_ACK 4
#define TCP_FINACK 5

enum EVENT_TYPE {
    RECEIVE_ACK,
    RECEIVE_SYN,
    RECEIVE_SYNACK,
    RECEIVE_FIN,
    RECEIVE_FINACK,
    RECEIVE_ERROR
};

struct TcpPacket {
    struct TcpHeader {
        uint16_t seqNo;
        uint16_t ackNo{0};
        uint16_t rcvWin{CLIENT_RCVWS};
        uint8_t reserved{0};
        uint8_t flags{0};
    }header;
    uint8_t payload[MAX_PKT_LEN];
};

uint16_t getRandSeqNo();
int getEventType(const TcpPacket &packet);

#endif //CS118PROJECT_TCPPACKET_H
