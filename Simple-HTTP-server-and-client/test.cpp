#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <map>

#include <iostream>
#include <sstream>
#include <string>
#include "HttpAbstraction.h"
using namespace std;

/*int
main()
{
  // create a socket using TCP IP
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);

  // allow others to reuse the address
  int yes = 1;
  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
    perror("setsockopt");
    return 1;
  }

  // bind address to socket
  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(20000);     // short, network byte order
  addr.sin_addr.s_addr = inet_addr("127.0.0.1");
  memset(addr.sin_zero, '\0', sizeof(addr.sin_zero));

  if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
    perror("bind");
    return 2;
  }

  // set socket to listen status
  if (listen(sockfd, 1) == -1) {
    perror("listen");
    return 3;
  }

  // accept a new connection
  struct sockaddr_in clientAddr;
  socklen_t clientAddrSize = sizeof(clientAddr);
  int clientSockfd = accept(sockfd, (struct sockaddr*)&clientAddr, &clientAddrSize);

  if (clientSockfd == -1) {
    perror("accept");
    return 4;
  }

  char ipstr[INET_ADDRSTRLEN] = {'\0'};
  inet_ntop(clientAddr.sin_family, &clientAddr.sin_addr, ipstr, sizeof(ipstr));
  std::cout << "Accept a connection from: " << ipstr << ":" <<
    ntohs(clientAddr.sin_port) << std::endl;

  // read/write data from/into the connection
  bool isEnd = false;
  char buf[20] = {0};
  std::stringstream ss;

  while (!isEnd) {
    memset(buf, '\0', sizeof(buf));

    if (recv(clientSockfd, buf, 20, 0) == -1) {
      perror("recv");
      return 5;
    }

    ss << buf << std::endl;
    std::cout << buf << std::endl;


    if (send(clientSockfd, buf, 20, 0) == -1) {
      perror("send");
      return 6;
    }

    if (ss.str() == "close\n")
      break;

    ss.str("");
  }

  close(clientSockfd);
	//For tests:string2byteblob:

	//test-1
	HttpRequest requestTesting;
	requestTesting.setVersion("HTTP/1.0");
	requestTesting.setUrl("/a.txt");
	requestTesting.setMethod("GET");
	requestTesting.setHeader("Host","11");
    requestTesting.setHeader("HA","22 ");
	byteblob encoded= requestTesting.encode();
    HttpRequest requestTesting2;
    requestTesting2.consume(encoded);
   if(requestTesting2.decode()){
        cout<<requestTesting2.getVersion()<<"*"<<endl;
        cout<<requestTesting2.getUrl()<<"*"<<endl;
        cout<<requestTesting2.getMethod()<<"*"<<endl;
        cout<<requestTesting2.getHeaders()<<"*"<<endl;
    }
    else cout<<"invalid"<<endl;
    void printbyteblob(byteblob bytes);
    HttpResponse responseTesting;
    responseTesting.setVersion("HTTP/1.0");
    responseTesting.setStatus(200);
    responseTesting.setDescription("ok");
    responseTesting.setHeader("Date","11/11/2016");
    responseTesting.setHeader("HA","22 ");
    responseTesting.setPayload(encoded);
    byteblob encoded2= responseTesting.encode();
    printbyteblob(encoded2);
    HttpResponse responseTesting2;
    if(!responseTesting2.consume(encoded2))cout<<"incomplete";
    if(responseTesting2.decode()){
        cout<<responseTesting2.getVersion()<<"*"<<endl;
        cout<<responseTesting2.getStatus()<<"*"<<endl;
        cout<<responseTesting2.getDescription()<<"*"<<endl;
        cout<<responseTesting2.getHeaders()<<"*"<<endl;
        printbyteblob(responseTesting2.getPayload());
    }
    else cout<<"invalid"<<endl;

  return 0;
}
void printbyteblob(byteblob bytes){
    for(int i=0;i<bytes.size();i++){
        cout<<bytes[i];
    }
}*/

/*byteblob cvt(string s){
	byteblob res;
	for(int i=0;i<s.size();i++){
		res.push_back(s[i]);
	}
	return res;
}*/
