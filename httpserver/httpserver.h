#pragma once
#include<iostream>
#include<stdio.h>
#include<sstream>
#include<vector>
#include<sys/socket.h>
#include<unordered_map>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<sys/wait.h>
#include<stdlib.h>
#include<string.h>
#include<fcntl.h>
#include<base/base.h>
//获取静态文件的完整路径
//用来存放请求的结构体
typedef struct sockaddr sockaddr;
typedef struct sockaddr_in sockaddr_in;

//命名空间
namespace httpserver{

typedef std::unordered_map<std::string,std::string> Headlers;
typedef struct request{
	std::string method;
	std::string url;
	std::string url_path;
	std::string url_argu;
	//std::string version;
	Headlers headler;
	std::string body;
}Request;
//用来存放响应的结构体
typedef struct response{
	//std::string version;
	int state;
	std::string message;
	Headlers headler;
	std::string body;
    std::string cgi_resp;
    //CGI 程序的返回内容
}Response;

class http_server;

//用来存放上下文，随时可以拓展的结构体
typedef struct Context{
	Request request;
	Response  response;
	int socket_fd;
	sockaddr_in addr;
    http_server* service; 
}Context;

class http_server{

public:
	//start server
	int start(int argc,char* argv[]);
    static void* ThreadEntry(void* con);
    void PrintRequest(const Context* context);
private:
	//read request
    int Parseline(std::string first_line,std::string* method,std::string* url);
    int Parseurl(std::string*  url,std::string* url_argu,std::string* url_path);
    int ParseHeadler(std::string* eadler_line,httpserver::Headlers* headler);
	int readrequest(Context* context);
	//write response
    void GetFilePath(std::string url_path,std::string* file_path);
	int writeresponse(Context* context);
    int ProcessStaticFile(Context* context);
    int ProcessCGI(Context* context);
	//deal request
	int Handlerrequest(Context* context);
	
    void process404(Context* context);
};
}//end pair
