//
//  web-server11.cpp
//  CS118-pj1
//
//  Created by GuoYang on 10/30/16.
//  Copyright © 2016 Guo,Y. All rights reserved.
//

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
#include <sstream>
#include <chrono>
#include <ctime>

#include "HttpAbstraction.h"

#define BUFSIZE 1024
#define BACKLOG 10
//#define TIMEALIVE 30

//Global host and port
const char* hostname;
const char* port;
std::string file_dir;

// A Lookup function for file type:
std::string getContentType(std::string url){
    
    std::map<std::string,std::string> content_type;
    content_type["htm"]="text/html";
    content_type["html"]="text/html";
    content_type["txt"]="text/plain";
    content_type["xml"]="text/xml";
    content_type["css"]="text/css";
    content_type["png"]="image/png";
    content_type["gif"]="image/gif";
    content_type["jpg"]="image/jpg";
    content_type["jpeg"]="image/jpeg";
    content_type["zip"]="application/zip";
    content_type["mp3"]="audio/mp3";
    content_type["wav"]="audio/wav";
    content_type["aiff"]="audio/aiff";
    content_type["mp4"]="video/mp4";
    content_type["mpg"]="video/mpg";
    content_type["wmv"]="video/wmv";
    
    size_t pos = url.find_last_of(".");
    if(pos==-1)
        return "unknown type" ;
    std::string postfix=url.substr(pos+1);
    if(content_type.find(postfix)==content_type.end())
        return "unknown type" ;
    
    return content_type[postfix];
    
}



void threadfun(int client_fd)
{
    std::cout<<"New thread for Client:"<<client_fd<<std::endl;
    std::string instring;
    char inbuf[BUFSIZE];
    long recvLen;
    bool connectionAlive=true;
    while (connectionAlive)
    {
        instring ="";
        memset(inbuf, '\0', sizeof(inbuf));
        recvLen = 0;
        
        //Read in request
        while (instring.find("\r\n\r\n") == std::string::npos)
        {
            recvLen = recv(client_fd, inbuf, BUFSIZE,0);
            if(recvLen==-1){
                perror("read");
                exit(1);
            }
            instring.append(inbuf, recvLen);
        }
        
        //Only consider firstline and header
        size_t requestEnd=instring.find("\r\n\r\n")+4;
        byteblob request_wire;
        for(int i=0;i<requestEnd;i++){
            request_wire.push_back(instring[i]);
        }
        
        //Decode request
        HttpRequest server_req;
        HttpResponse server_res;
        byteblob res_payload;
        server_req.consume(request_wire);
        bool valid=server_req.decode();
        
        //Get Current time
        std::time_t cur_time= std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        server_res.setHeader("Date", std::ctime(&cur_time));

        //400 Bad Request
        if(!valid){
            server_res.setVersion("HTTP/1.0");
            server_res.setStatus(400);
            server_res.setDescription("Bad Request");
            std::string str="Your browser sent a request that this server could not understand.\n";
            res_payload.assign(str.begin(),str.end());
            server_res.setPayload(res_payload);
            server_res.setHeader("Content-Type", "text/html");
            server_res.setHeader("Content-Length", std::to_string(str.length()));
        }
        //Look for file
        else{
            //Close connection
            
            if(server_req.getHeaderValue("Connection")=="close"){
                connectionAlive=false;
            }
            
            server_res.setVersion(server_req.getVersion());
            FILE* fileptr;
            long long file_size=0;
            const char *file_path=(file_dir+server_req.getUrl()).c_str();
            std::cout<<file_path<<std::endl;
            if((fileptr=fopen(file_path, "r"))!=nullptr)
            {//200 OK
                server_res.setStatus(200);
                server_res.setDescription("OK");
                // obtain file size:
                fseek (fileptr , 0 , SEEK_END);
                file_size = ftell (fileptr);
                rewind (fileptr);//set position of stream to the beginning
                
                // allocate memory to contain the whole file:
                unsigned char * file_buf;
                if ((file_buf = (unsigned char*) malloc (file_size)) == NULL)
                {
                    fputs ("Memory error",stderr);
                    exit(1);
                }
                memset(file_buf,0,file_size);
                
                // copy the file into the buffer:
                size_t result;
                if ((result = fread (file_buf,1,file_size,fileptr)) != file_size)
                {
                    fputs ("Reading error\n",stderr);
                    exit(1);
                }
                
                res_payload.insert(res_payload.end(),file_buf,file_buf+file_size);
                server_res.setPayload(res_payload);
                
                fclose (fileptr);
                server_res.setHeader("Content-Type", getContentType(server_req.getUrl()));
                server_res.setHeader("Content-Length", std::to_string(file_size));
                
            }
            else{//404 Not Found
                std::string str="The requested URL "+server_req.getUrl()+" was not found on this server.\n";
                res_payload.assign(str.begin(),str.end());
                server_res.setPayload(res_payload);
                server_res.setStatus(404);
                server_res.setDescription("Not found");
                server_res.setHeader("Content-Type", "text/html");
                server_res.setHeader("Content-Length", std::to_string(str.length()));
            }
            
        }
        if (server_req.getHeaderValue("Connection").compare("close")==0) {
            server_res.setHeader("Connection","close");
        }
        
        byteblob outwire=server_res.encode();
        
        //Send out response
        if (send(client_fd, outwire.data(),outwire.size(), 0) == -1) {
            perror("send");
            exit(1);
        }
        
        //Http 1.0: close connection immediately
        if(server_req.getVersion().compare("HTTP/1.1")!=0)
        {
            std::cout<<"Http 1.0: Data Send.Client:"<<client_fd<<std::endl;
            close(client_fd);
            connectionAlive=false;
           // return;
        }

    }
    close(client_fd);
    std::cout<<"Connection closed.Client:"<<client_fd<<std::endl;
    return;
}

int main(int argc, const char* argv[])
{
    if(argc<4||argc>4){
        if(argc!=1){
            fprintf(stdout, "Usage:web_server [host] [port] [file_dir]\n Default: localhost 4000 . \n");
            exit(1);
        }
        hostname=NULL;//REMEMBER TO SET FLAG FOR HOSTNAME!!ai_flags = AI_PASSIVE;
        port="4000";
        file_dir=".";
        
    }else{
        hostname=argv[1];
        port=argv[2];
        //file_dir="."+std::string(argv[3]);
        file_dir="."+std::string(argv[3]);
    }
    
    int status=0;
    int sockfd;
    
    //Prepare hints
    struct addrinfo hints;
    struct addrinfo* servinfo,*p;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET; // IPv4
    hints.ai_socktype = SOCK_STREAM; // TCP
    if(hostname==NULL) hints.ai_flags = AI_PASSIVE;
    
    //Get socket addresses
    if ((status = getaddrinfo(hostname,port, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        return 1;
    }
    
    //Loop to bind the first available socket address
    for(p = servinfo; p != NULL; p = p->ai_next) {
        
        // create a socket
        sockfd = socket(p->ai_family, p->ai_socktype,0);
        if (sockfd == -1) {
            perror("socket");
            continue;
        }
        // allow others to reuse the address
        int yes = 1;
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }
        // bind address to socket
        if ( bind(sockfd, p->ai_addr,p->ai_addrlen)== -1) {
            close(sockfd);
            perror("bind");
            continue;
        }
        break; // binded successfully
    }
    //no socket to bind
    if (p == NULL) {
        fprintf(stderr, "bind error: failed to bind\n");
        exit(1);
    }
    // free the linked list
    freeaddrinfo(servinfo);
    
    //Set socket to listen
    if (listen(sockfd,BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }
    
    //Child thread for new connection
    //std::thread *threadptr;
    while (true)
    {
        // Accept a new connection
        struct sockaddr_in client_addr;
        socklen_t addr_size = sizeof(client_addr);
        int client_fd = accept(sockfd, (struct sockaddr*)&client_addr, &addr_size);
        if (client_fd == -1) {
            perror("accept");
            continue;
        }
        std::thread(threadfun, client_fd).detach();
        
    }
    return 0;
}

