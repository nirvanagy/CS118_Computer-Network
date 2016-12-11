#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <thread>
#include <iostream>
#include <fstream>
#include <chrono>
#include <ctime>
#include <cassert>

#define BUFSIZE 1024
#include "HttpAbstraction.h"
using namespace std;

//parse URL
struct parsed_url {
public:
    string path;
    string host;
    string port;
};
parsed_url parseUrl(string url){
    parsed_url res;
    string r_path, r_host, r_port;
    string http("http://");
    
    if (url.compare(0, http.size(), http) == 0) {
        unsigned long pos = url.find_first_of("/:", http.size());
        
        if (pos == string::npos) {
            // No port or path
            pos = url.size();
        }
        
        // extract host name
        r_host = url.substr(http.size(), pos - http.size());
        
        unsigned long ppos = url.find_first_of("/", pos);
        if (ppos == string::npos)
            ppos = url.size();
        if (pos < url.size() && url[pos] == ':') {
            // A port is provided
            
            if (ppos == string::npos) {
                // No path provided, assume port is the rest of string
                ppos = url.size();
            }
            r_port = url.substr(pos+1, ppos-pos-1);
        }
        r_path = url.substr(ppos);  // the rest is path
        
    } else {
        perror( "Not an HTTP url" );
        exit(2);
    }
    
    res.path = r_path;
    res.host = r_host;
    res.port = r_port;
    return res;
    
}


int main(int argc, char *argv[])
{
    //Parse all URLs
    if(argc == 1){
        fprintf(stdout, "Usage:web-client [URL] [URL]... \n");
        exit(1);
    }
    int n_url=argc-1;
    //int n_url=1;
    parsed_url *url = new parsed_url[n_url];

    //string test_url="http://localhost:4000/Makefile";
    
    for(int i=1; i<=n_url; i++){
        url[i-1] = parseUrl(argv[i]);
        if(url[i-1].port == ""){
            url[i-1].port="80";
        }
        if(url[i-1].path == "/"){
            url[i-1].path = "/index.html";
        }
    }
    
    //Create sockets and Connect
    for(int i=0; i<n_url; i++){
        // prepare hints
        struct addrinfo hints;
        struct addrinfo* servinfo,*p;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET; // IPv4
        hints.ai_socktype = SOCK_STREAM; // TCP
        
        // get address
        int status=0;
        if ((status =getaddrinfo(url[i].host.c_str(),url[i].port.c_str(),&hints, &servinfo)) != 0) {
            fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
            return 1;
        }
        //Loop to connect the first available socket address
        int sockfd;
        for(p = servinfo; p != NULL; p = p->ai_next) {
            // create a socket
            sockfd = socket(p->ai_family, p->ai_socktype,0);
            if (sockfd == -1) {
                perror("client:socket");
                continue;
            }
            // connect address to socket
            if (connect(sockfd, p->ai_addr,p->ai_addrlen)== -1) {
                close(sockfd);
                perror("client:connect");
                continue;
            }
            break; // connected successfully
        }
        //no socket to connect
        if (p == NULL) {
            fprintf(stderr, "onnect error: failed to connect\n");
            exit(1);
        }
        // free the linked list
        freeaddrinfo(servinfo);
        
        //Send Request
        HttpRequest clt_req;
        clt_req.setUrl(url[i].path);
        clt_req.setMethod("GET");
        clt_req.setVersion("HTTP/1.0");
        clt_req.setHeader("Host", url[i].host);
        clt_req.setHeader("Connection","close");
        byteblob req_wire = clt_req.encode();
        string outstring=string(req_wire.begin(),req_wire.end());
        if (send(sockfd, outstring.c_str(),outstring.size(), 0) == -1)
        {
            perror("clint:send");
            return 4;
        }
        
        //Read Response
        HttpResponse clt_res;
        string inString;
        string fileString;
        char inBuf[BUFSIZE];
        size_t recvLen;
        size_t fileLen;
        
        while(true){
            
            recvLen =recv(sockfd, inBuf, BUFSIZE,0);
            if(recvLen==-1){
                perror("client:read");
                exit(1);
            }
            inString.append(inBuf, recvLen);
            /*
            if(fileLen<0){
                inString.append(inBuf, recvLen);
                if(inString.find("\r\n\r\n") == std::string::npos)continue;
                else{
                    size_t sizeOfHeader=inString.find("\r\n\r\n")+4;//index: size-1
                    fileString.append(inString.begin()+sizeOfHeader,inString.end());
                    fileLen=inString.size()-sizeOfHeader;
                    assert(fileLen==fileString.size());
                    byteblob request_wire;
                    for(int i=0;i<sizeOfHeader;i++){
                        request_wire.push_back(inString[i]);
                    }
                    clt_res.consume(request_wire);
                    clt_res.decode();
                    contentLen=atoi(clt_res.getHeaderValue("Content-Length").c_str());
                }
            }
            else{
                fileLen+=recvLen;
                fileString.append(inBuf,recvLen);
                if (fileLen==contentLen){
                    byteblob payload_wire;
                    for(int i=0;i<fileString.size();i++){
                        payload_wire.push_back(fileString[i]);
                    }
                    clt_res.setPayload(payload_wire);
                    break;
                }
            }*/
            if(recvLen==0)break;
        }
        byteblob request_wire;
        for(int i=0;i<inString.size();i++){
            request_wire.push_back(inString[i]);
        }
        clt_res.consume(request_wire);
        clt_res.decode();
        size_t contentLen = atoi(clt_res.getHeaderValue("Content-Length").c_str());
        fileLen = clt_res.getPayload().size();
        if(contentLen!=fileLen){
            std::cerr<<"Incomplete Response with URL:"<<url[i].host<<url[i].path<<endl;
            return 1;
        }
        
        //Close connection
        close(sockfd);
        
        // Store the file
        if(clt_res.getStatus()==404||clt_res.getStatus()==400)
            cout<<clt_res.getDescription()<<endl;
        else{
            std::size_t name_pos = url[i].path.find_last_of("/");
            string filename = url[i].path.substr(name_pos+1);
            ofstream file;
            if (filename.size() == 0) filename = "index.html";
            std::cerr<<"file name is: "<<filename;
            file.open(filename);
            byteblob file_content=clt_res.getPayload();
            file <<string(file_content.begin(),file_content.end());
            file.close();
        }
    }
    return 0;

}

