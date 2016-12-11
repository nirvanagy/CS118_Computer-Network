#ifndef HTTP_H
#define HTTP_H

#include <vector>
#include <string>
#include <map>
#include <iostream>

/*
 * Request:request-line(a method, URI, and protocol version)
 * header fields(request modifiers, client information, and representation metadata)
 * an empty line to indicate the end of the header section
 * a message body containing the payload body

 * Response:a status line(the protocol version, a success or error code, and textual reason phrase)
 * header fields (server information, resource metadata, and representation metadata)
 * an empty line to indicate the end of the header section
 *  a message body containing the payload body
   */
/*Example:
   Client request:

     GET /hello.txt HTTP/1.1
     User-Agent: curl/7.16.3 libcurl/7.16.3 OpenSSL/0.9.7l zlib/1.2.3
     Host: www.example.com
     Accept-Language: en, mi


   Server response:

     HTTP/1.1 200 OK
     Date: Mon, 27 Jul 2009 12:28:53 GMT
     Server: Apache
     Last-Modified: Wed, 22 Jul 2009 19:15:56 GMT
     ETag: "34aa387-d-1568eb00"
     Accept-Ranges: bytes
     Content-Length: 51
     Vary: Accept-Encoding
     Content-Type: text/plain

     Hello World! My payload includes a trailing CRLF.*/

typedef std::string httpversion;
typedef std::string httpmethod;
typedef int httpstatus;
typedef std::vector<char> byteblob;

class HttpMessage{

private:
	std::map<std::string,std::string>m_headers;
	httpversion m_version;
	//byteblob encodeHeaderLines();//m_headers->byteblob

public:
	byteblob m_firstline;
	std::vector<byteblob> m_headerlines;
	byteblob m_payload;

	HttpMessage();
	void decodeHeaderLines();//m_headerlines->m_headers;
	virtual bool consume(byteblob wire)=0;//parse message into firstline, m_headerlines and payload
	virtual bool decode()=0;//decode firstline+decodeHeaderlines()+setPayload
	virtual byteblob encode()=0;//encode firstline,encode result of getHeaders,payload
	httpversion getVersion();
	void setVersion(httpversion version);
	std::string getHeaderValue(std::string key);
	std::string getHeaders();
	void setHeader(std::string key,std::string value);
	byteblob getPayload();
	void setPayload(byteblob payload);
};

//About consume() and decode():since decode won't care about /r/n, it could be the case that even without the \r\n the meaning can br resolved. So the right behaviour should be once found the message is incomplete, stop  befor decoding.
//consume()only deal with format, which is enough for request. But for response, need more operation to check the body is complete.
class HttpRequest:public HttpMessage{
private:
	httpmethod m_method;
	std::string m_url;
public:
	HttpRequest();
	virtual bool decode();//deal with invalid request
	virtual bool consume(byteblob wire);//deal with incomplete format;
	virtual byteblob encode();
	httpmethod getMethod();
	void setMethod(httpmethod method);
	std::string getUrl();
	void setUrl(std::string url);
};

class HttpResponse:public HttpMessage{
private:
	httpstatus m_status;
	std::string m_description;
public:
	HttpResponse();
	virtual bool decode();//deal with invalid response;
	virtual bool consume(byteblob wire);//deal with incomplete format;
	virtual byteblob encode();
	httpstatus getStatus();
	void setStatus(httpstatus status);
	std::string getDescription();
	void setDescription(std::string description);
};



#endif
