//
// Created by 陈昊 on 11/22/16.
//

#include "TcpPacket.h"

using namespace std;

uint16_t getRandSeqNo() {
    random_device rd;
    default_random_engine dre(rd());
    uniform_int_distribution<uint16_t> uniform_dist(0, MAX_SEQ_NUM-1);
    return uniform_dist(dre);
}

int getEventType(const TcpPacket &packet) {
    uint8_t flags = packet.header.flags;
    if(flags == TCP_ACK)
        return RECEIVE_ACK;
    else if(flags == TCP_SYN)
        return RECEIVE_SYN;
    else if((flags & TCP_SYN) && (flags & TCP_ACK))
        return RECEIVE_SYNACK;
    else if(flags == TCP_FIN)
        return RECEIVE_FIN;
    else if((flags & TCP_FIN) && (flags & TCP_ACK))
        return RECEIVE_FINACK;
    
    return RECEIVE_ERROR;
}
