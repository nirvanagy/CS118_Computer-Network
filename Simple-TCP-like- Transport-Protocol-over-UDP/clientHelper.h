#ifndef CLIENTHELPER_H_
#define CLIENTHELPER_H_

#include <stdlib.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/time.h>
#include <iostream>

#include "TcpPacket.h"

#define TIME_OUT_MSEC 500000

class Client {
public:
	Client() {
		m_maxNoOfPackets = MAX_RCV_WIN / MAX_PKT_LEN;
	};

	void init(const char *hostName, int portNum);
	void connect();
	void receive();
	void finish();

private:
	// Member parameters
	int m_sockfd;
	int m_filefd;
	int m_maxNoOfPackets;
	uint16_t m_seqNo;
	uint16_t m_ackNo;
	uint16_t m_baseNo;
	uint16_t m_seqFinNoServer;
	ssize_t m_lastFilePacketSize;

	uint8_t m_buff[MAX_RCV_WIN];
	bool m_buffWrittedIdx[MAX_RCV_WIN / MAX_PKT_LEN];
	bool m_isServerFin;
	bool m_isLastFilePacket;
	struct sockaddr_in m_serverAddr;

	// Member function
	ssize_t sendACK(bool retransmission);
	void increaseAckNo(ssize_t pktDataSize);
	void writeBuff(const TcpPacket& packet, ssize_t pktDataSize);
	void updateBuff();
	ssize_t writeFile(ssize_t pktDataSize);

};
#endif // CLIENTHELPER_H_