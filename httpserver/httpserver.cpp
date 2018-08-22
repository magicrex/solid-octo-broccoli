#include"httpserver.h"
#include"util.hpp"

namespace httpserver{
    //将打印请求进行声明
    void PrintRequest(const Context* context);
    //解析请求的第一行，即方法和url
    //主要是进行字符串的分割
    int Parseline(std::string first_line,std::string* method,std::string* url){
        std::vector<std::string> output;
        common::StringUtil::Split(first_line," ",&output);
        if(output.size()!=3){
            LOG(ERROR)<<" first line loss";
            return -1;
        }
        *method=output[0];
        *url=common::UrlUtil::deescapeURL(output[1]);
        return 1;
    }
    //解析请求的第一行的url部分，即目标文件和参数
    //主要是字符串的分割
    int Parseurl(const std::string  url,std::string* url_argu,std::string* url_path){
        int pos=url.find('?');
        if(pos==0){
            return -1;
        }
        (*url_path).assign(url,0,pos);
        (*url_argu).assign(url,pos+1,url.size()-pos);
        return 1;
    }
    //解析请求的第二部分通用首部
    //主要是按行读取并使用冒号分割,并缓存到一个map数据结构中
    int ParseHeadler(const std::string Headler_line , httpserver::Headlers* headler){
        size_t pos=Headler_line.find(':');
        if(pos==std::string::npos){
            LOG(ERROR)<< " Headlers error";
            return -1;
        }
        if(pos+2>=Headler_line.size()){
            LOG(ERROR)<< " Headlers error";
            return -1;
        }
        (*headler)[Headler_line.substr(0,pos)]
            = Headler_line.substr(pos+2);
        return 1;
    }

    //读取请求并解析
    //是线程的第一步，进行读取请求，为后面处理作准备
    int http_server::readrequest(Context* context)
    {
        Request* req=&context->request;
        std::string first_line;
        int ret =0;
        ret=common::FileUtil::Readline(context->socket_fd,&first_line);
        if(ret<0){
            LOG(ERROR)<<"requset Readline error";
            return -1;
        }
        ret = httpserver::Parseline(first_line,&req->method,&req->url);
        if(ret<0)
        {
            LOG(INFO)<<"request Parseline error";
            return -1;
        }
        ret=httpserver::Parseurl(req->url,&req->url_argu,&req->url_path);
        if(ret<0){
            LOG(INFO)<<"request Parseurl error";
            return -1;
        }
        std::string headler_line;
        while(1){
            ret=common::FileUtil::Readline(context->socket_fd,&headler_line);
            if(headler_line.empty()){
                break;
            }  
            ret=httpserver::ParseHeadler(headler_line,&req->headler);
            if(ret<0){
                LOG(ERROR)<<"ParseHeadler error";
                return -1;
            }
        }
        //分为get、post和其他类型
        //get方法不需要读取主体，post读取主体,其他方法即位错误
        if(req->method=="GET")
        {
            return 1;
        }
        else if(req->method=="POST")
        {
            //先读取长度，为后面读取内容做准备
            //读取文件类型，根据类型判断是读到一个string或者缓存文件中
            int contlen;
            httpserver::Headlers::iterator it;
            it =req->headler.find("Content-Length");
            if(it!=req->headler.end()){
                int n=std::stoi((*it).second);
                contlen=n;
            }else{
                LOG(ERROR)<<" request error";
                return -1;
            }   

            it =req->headler.find("Content-Type");
            if(it!=req->headler.end()){
                std::vector<std::string> type;
                common::StringUtil::Split(req->headler["Content-Type"],";",&type);
                if(type.empty()){
                    LOG(ERROR)<<" request error";
                    return -1;
                }else{
                    if(type[0]=="application/x-www-form-urlencoded"){
                        ret=common::FileUtil::ReadN(context->socket_fd,contlen,&req->body);
                        return 1;
                    }else if(type[0]=="multipart/form-data"){
                        //将内容读入一个缓存文件中，以sessid作为缓存文件名
                        it=req->headler.find("Cookie");   
                        if(it!=req->headler.end()){
                            std::vector<std::string> cookie;
                            common::StringUtil::Split(req->headler["Cookie"],"=",&cookie);
                            common::FileUtil::ReadNFile(context->socket_fd,contlen,cookie[1]);
                            return 1;
                        }else{
                            LOG(ERROR)<<" request error";
                            return -1;
                        }
                    }else{
                        LOG(ERROR)<<" request error";
                        return -1;
                    }
                }
            }else{
                LOG(ERROR)<<" request error"<<"\n";
                return -1;
            }   
        }else if(req->method=="DELETE"){
            return 1; 
        }else
        {
            return -1;
        }
        return 1;
    }//end readrequest

    //线程处理的第三步，构造响应报文返回给浏览器
    int http_server::writeresponse(Context* context)
    {
        const Response* resp=&context->response;
        std::stringstream ss; 
        ss << "HTTP/1.1 " << resp->state << " "<<resp->message <<"\n"; 
        if(resp->cgi_resp==""){
            for(auto item : resp->headler){
                ss<< item.first <<": " <<item.second<<"\n";
            }
            ss<<"\n";
            ss<<resp->body;
        }else{
            ss<<resp->cgi_resp;
        }
        const std::string& str=ss.str();
        write(context->socket_fd,str.c_str(),str.size()); 
        return 1;
    }

    //构造绝对路径，将网络的路径参数构造成一个服务器绝对路径
    //主要是默认文件的路径和一般文件的路径
    void http_server::GetFilePath(std::string url_path,std::string* file_path){
        *file_path="../wwwroot"+url_path;
        if(common::FileUtil::IsDir(file_path->c_str())){
            if(file_path->back()!='/'){
                file_path->push_back('/');
            }
            *file_path=(*file_path)+"index.html";
        }
    }

    //判断GET方法的处理
    //判断文件是否存在，或是否可执行
    int http_server::ProcessStaticFile(Context* context){
        const Request* req=&context->request;
        Response* resp=&context->response;

        std::string file_path;
        GetFilePath(req->url_path,&file_path);
        if((access(file_path.c_str(),F_OK))!=-1){
            if((access(file_path.c_str(),X_OK))!=-1){
                ProcessCGI(context);
                return 1;
            }else{
                int ret=common::FileUtil::ReadAll(file_path,&resp->body);
                return 1;
            }
        }else{
            return -1;
        }
    }


    //请求的动态处理
    int http_server::ProcessCGI(Context* context){
        //1.如果是POST请求，父进程就要把body写入到管道中
        // 阻塞式对去管道，尝试把子进程的结果读取出来，并放到Respponse中
        // 对子进程进行进程等待
        //2.fork，子进程流程
        // 把标准输入输出进行重定向
        //先获取到要替换的可执行文件
        //由CGI可执行程序完成动态页面的计算，并且写回数据到管道
        //先创建一对匿名管道全双工通信
        const Request& req = context->request;
        Response* resp=&context->response;
        int fd1[2],fd2[2];
        pipe(fd1);
        pipe(fd2);
        int father_write = fd1[1];
        int father_read = fd2[0];
        int child_write = fd2[1];
        int child_read = fd1[0];

        //GET 方法 QUERY_STRING 请求的参数 
        //POST 请求 CONTENT_LENGTH 长度
        //fork 父子进程流程不同
        pid_t ret=fork();

        if(ret<0){
            perror("fork");
            goto END;
        }else if(ret > 0){
            //父进程
            //父进程负责将post的body写入管道1
            //将最后终结果从管2读出到response中
            close(child_read);
            close(child_write);
            if(req.method=="POST"){
                write(father_write,req.body.c_str(),req.body.size());
            }
            common::FileUtil::ReadAll(father_read,&resp->cgi_resp);
            wait(NULL);
        }else{
            //子进程
            //设置环境变量 METHOD请求方法 
            std::string env1="REQUEST_METHOD="+req.method;
            std::string env2;
            if(req.method=="GET"){
                std::string file_path;
                GetFilePath(req.url_path,&file_path);
                if((access(file_path.c_str(),F_OK))!=-1){
                    if((access(file_path.c_str(),X_OK))==-1){
                        process404(context);
                    }
                }else{
                    process404(context);
                }
                env2="QUERY_STRING="+req.url_argu;
                char * const envp[] = {const_cast<char*>(env1.c_str()),const_cast<char*>(env2.c_str()), NULL};
                close(father_read);
                close(father_write);
                dup2(child_read,0);
                dup2(child_write,1);
                if((execle(file_path.c_str(),file_path.c_str(),NULL,envp))==-1){
                    LOG(ERROR)<<"file_path:";
                    LOG(ERROR)<<"execle error";
                }
            }else{
                LOG(INFO)<<"request error";
                process404(context);
            }
        }
END:
        close(father_read);
        close(father_write);
        close(child_read);
        close(child_write);
        if(ret<0)
            return -1;
        else
            return 1;

    }//end ProcessCGI

    //线程的第二步，请求处理
    //主要是进行判断，具体处理由cgi进行
    int http_server::Handlerrequest(Context* context)
    {
        const Request* req=&context->request;
        Response* resp=&context->response;
        resp->state=200;
        resp->message="OK";
        if(req->method=="GET"){
            return ProcessStaticFile(context);
        }else if(req->method=="POST"){
            return ProcessCGI(context);
        }else if(req->method=="DELETE"){
            return ProcessCGI(context);
        }else{
            LOG(ERROR)<<"Unsupport Method"<< req->method <<"\n";  
            return -1;
        }
        return 1;
    }

    //构造一个404页面，所有错误均使用404
    void http_server::process404(Context* context)
    {
        Response* resp=&context->response;
        resp->state=404;
        resp->message="Not Found";
        resp->body="<html><head><meta charset=\"utf8\"><title>NotFound</title></head><body><h1>页面出错</h1></body></html>";
        std::stringstream ss;
        ss << resp->body.size();
        std::string size;
        ss >> size;
        resp->headler["Content-Length"]=size;
    }

    //打印请求方便记录
    //并不打印body部分
    void http_server::PrintRequest(const Context* context){
        const Request* req=&context->request;
        std::cout<<"HTTP1.1 "<<req->method<<" "<<req->url<<std::endl;
        Headlers::const_iterator it=req->headler.begin();
        for(;it!=req->headler.end();it++){
            std::cout<<(*it).first<<": "<<(*it).second<<std::endl;
        }
        std::cout<<std::endl;
        std::cout<<"body "<<std::endl;
        std::cout<<req->body<<std::endl;
    }

    //线程入口函数，每个线程的主要逻辑
    //分为读取、处理、返回三步
    int threadcount=1;
    void* http_server::ThreadEntry(void* con)
    {
        threadcount++;
        Context* context =reinterpret_cast<Context*>(con);
        int ret=context->service->readrequest(context);
        if(ret<0)
        {
            context->service->process404(context);
            goto END;
        }
        context->service->PrintRequest(context);
        ret=context->service->Handlerrequest(context);
        if(ret<0)
        {
            context->service->process404(context);
            goto END;
        }
END:
        context->service->writeresponse(context);
        delete context;
        close(context->socket_fd);
        return 0;
    }

    //整个服务器的主要流程
    //将服务器启动，绑定端口并集进行监听
    //使用多线程提高服务器的并发
    int http_server::start(int argc,char* argv[])
    {
        if(argc!=3)
        {
            std::cout<<"user IP Port"<<std::endl;
            return -1;
        }

        int sockt_fd=socket(AF_INET,SOCK_STREAM,0);
        if(sockt_fd<0)
        {
            LOG(FATAL)<<"socket";
            return 2;
        }
        //给socket加一个参数使得文件描述符重用，不至于出现大量的timw_wait
        int opt=1;
        setsockopt(sockt_fd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));
        sockaddr_in server_sock;
        server_sock.sin_family=AF_INET;
        server_sock.sin_port=htons(atoi(argv[2]));
        server_sock.sin_addr.s_addr=inet_addr(argv[1]);

        int ret=bind(sockt_fd,(sockaddr*)&server_sock,sizeof(server_sock));
        if(ret<0)
        {
            LOG(FATAL)<<"bind";
            return 3;
        }

        if(listen(sockt_fd,5)<0)
        {
            LOG(FATAL)<<"listen";
            return 4;
        }
        while(1)
        {
            sockaddr_in client_addr;
            socklen_t len=sizeof(client_addr);
            int client_socket_fd =accept(sockt_fd,(sockaddr*)&client_addr,&len);
            if(client_socket_fd<0)
            {
                LOG(INFO)<<"accept";
                continue;
            }
            pthread_t tid;
            Context* context=new Context();
            context->addr =client_addr;
            context->socket_fd=client_socket_fd;
            pthread_create(&tid,NULL,ThreadEntry,reinterpret_cast<void*>(context));
            pthread_detach(tid);
        }
        return 0;
    }//end start
}//end namespace
