//============================================================================
// Name        : cs118-proj1.cpp
// Author      : 
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <iostream>
#include <vector>
#include <string>
//#include <cstdlib>
#include <map>
//#include <sstream>
#include <cassert>
#include"HttpAbstraction.h"

using namespace std;

//-----Class HttpMessage
HttpMessage::HttpMessage(){
	m_version="";
	m_payload.clear();
	m_headers.clear();
	m_firstline.clear();
	m_headerlines.clear();
}
void HttpMessage::setVersion(httpversion version){
	m_version=version;
}
httpversion HttpMessage::getVersion(){
	return m_version;
}

void HttpMessage::setHeader(string key, string value)
{
	m_headers[key] = value;
}
string HttpMessage::getHeaders()
{
	string res = "";
    map<string, string>::iterator it = m_headers.begin();
	while (it != m_headers.end())
	{
        if(it->first=="Date")m_headers["Date"].pop_back();
        res = res + it->first + ":" + it->second + "\r\n";
		it++;
	}
	//res = res + "\r\n";
	return res;
}
string HttpMessage::getHeaderValue(string key)
{
	if(m_headers.find(key)!=m_headers.end())
		return m_headers[key];
	else return "NA";
}

void HttpMessage::setPayload(byteblob payload){
	m_payload=payload;
}
byteblob HttpMessage::getPayload(){
	return m_payload;
}

void HttpMessage::decodeHeaderLines(){

	for(int i=0;i<m_headerlines.size();i++){
		byteblob line=m_headerlines[i];
		int j=0;
		string key="";
		string val="";
		while(line[j]!=':'){
			key+=line[j];
			j++;
		}
		j++;
		while(line[j]==' '){//optional leading whitespace before value
			j++;
		}
		while(j<line.size()){
			val+=line[j];
            j++;
		}
		setHeader(key,val);
	}
}

//-----Class HttpRequest
HttpRequest::HttpRequest(){
	m_method="";
	m_url="";
}
void HttpRequest::setMethod(httpmethod method){
	m_method=method;
}
httpmethod HttpRequest::getMethod(){
	return m_method;
}
void HttpRequest::setUrl(string url){
	m_url=url;
}
string HttpRequest::getUrl(){
	return m_url;
}

byteblob HttpRequest::encode(){
	byteblob res;
	string temp="";
    temp=temp+getMethod()+" "+getUrl()+" "+getVersion()+"\r\n"+getHeaders()+"\r\n";
	res.assign(temp.begin(),temp.end());
	return res;
}

bool HttpRequest::consume(byteblob wire){
	size_t n=wire.size();
	//Incomplete request
	if(!(wire[n-1]=='\n'&&wire[n-2]=='\r'&&wire[n-3]=='\n'&&wire[n-4]=='\r')) return false;
	//First line
	int i=0;
	while(wire[i]!='\r'){
		m_firstline.push_back(wire[i]);
		i++;
	}
	i+=2;
	//Headers
	byteblob headerline;
	while(wire[i]!='\r'){
		headerline.clear();
		while(wire[i]!='\r'){
			headerline.push_back(wire[i]);
			i++;
		}
		m_headerlines.push_back(headerline);
		i+=2;
	}
	assert((i+2)==wire.size());
	return true;
}

bool HttpRequest::decode(){
	//decode first line
	int i=0;
	string method="";
	string url="";
	string version="";
	//method
    if(m_firstline.empty())return false;
	while(m_firstline[i]!=' '){
		method+=m_firstline[i];
		i++;
	}
	if(method=="GET"){//request method is case-sensitive
		setMethod(method);
	}else return false;
	//url
	i++;
	while(m_firstline[i]!=' '){
		url+=m_firstline[i];
		i++;
	}
	if(url=="/") setUrl("/index.html");
	else setUrl(url);
	//version
	i++;
	while(i<m_firstline.size()){
		version+=m_firstline[i];
		i++;
	}
	if(version!="HTTP/1.1"&&version!="HTTP/1.0")return false;// HTTP-version is case-sensitive.
	else setVersion(version);

	//decode headers
	// header names are case-insensitive, Must has host field
	decodeHeaderLines();
	if(getHeaderValue("Host")=="NA") return false;
	return true;
}

//------Class HttpResponse
HttpResponse::HttpResponse(){
	m_status=0;
	m_description="";
}
void HttpResponse::setStatus(httpstatus status){
	m_status=status;
}
httpstatus HttpResponse::getStatus(){
	return m_status;
}
void HttpResponse::setDescription(string description){
	m_description=description;
}
string HttpResponse::getDescription(){
	return m_description;
}
byteblob HttpResponse::encode(){
	byteblob res;
	string temp="";
    temp=temp+getVersion()+" "+to_string(getStatus())+" "+getDescription()+"\r\n"+getHeaders()+"\r\n";
	res.assign(temp.begin(),temp.end());
	for(int i=0;i<m_payload.size();i++){
		res.push_back(m_payload[i]);
	}
	return res;
}
bool HttpResponse::consume(byteblob wire){//!!!how to figure out if the response is complete???
	//Assume the transmission errors are lacking chars from the end.
	//First line
	int i=0;
	while(wire[i]!='\r'){
		if(i>=wire.size())return false;
		m_firstline.push_back(wire[i]);
		i++;
	}
    if(wire[i+1]!='\n')return false;
	i+=2;
	//Headers
	byteblob headerline;
	while(wire[i]!='\r'){
		if(i>=wire.size())return false;
		headerline.clear();
		while(wire[i]!='\r'){
			if(i>=wire.size())return false;
			headerline.push_back(wire[i]);
			i++;
		}
        if(wire[i+1]!='\n')return false;
		m_headerlines.push_back(headerline);
		i+=2;
	}
    if(wire[i+1]!='\n')return false;
	i+=2;
	//payload
	//possible empty body
	while(i<wire.size()){
		m_payload.push_back(wire[i]);
        i++;
	}
	return true;
}
bool HttpResponse::decode(){
	//decode first line
	int i=0;
	string version="";
	string status="";
	string description="";
	//version
	while(m_firstline[i]!=' '){
		version+=m_firstline[i];
		i++;
	}
	if(version!="HTTP/1.1"&&version!="HTTP/1.0")return false;// HTTP-version is case-sensitive.
	else setVersion(version);
	//status
    i++;
	while(m_firstline[i]!=' '){
		status+=m_firstline[i];
		i++;
	}
	if(status.size()!=3)return false;//status code is 3-digit
	else setStatus(atoi(status.c_str()));
	//description
	i++;
	while(i<m_firstline.size()){
		description+=m_firstline[i];
		i++;
	}
	setDescription(description);

	//decode headers
	decodeHeaderLines();
	if(getHeaderValue("Date")=="NA") return false;
    //decode payload
    //setPayload(m_payload);
	return true;
}
