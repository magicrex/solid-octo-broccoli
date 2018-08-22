#include<base/base.h>
#include<sofa/pbrpc/pbrpc.h>
#include"server.pb.h"
#include"util.hpp"
#include<iostream>
#include"doc_searcher.h"

DEFINE_string(port,"10000","服务器端口号");
DEFINE_string(index_path,"/home/master/Git/sort/index/data/output/index_file","索引文件的路径");

namespace  doc_server{
    typedef doc_server_proto::Request Request;
    typedef doc_server_proto::Response Response;

    class DocServerAPIImpl: public doc_server_proto::DocServerAPI {
    public:
        void Search(::google::protobuf::RpcController* controller,
                const Request* req,Response* resp,
                ::google::protobuf::Closure* done){
            (void) controller;
            resp->set_sid(req->sid());
            resp->set_timestamp(common::Timeutil::TimeStamp());

            DocSearcher searcher;
            LOG(INFO)<< "Search begin";
            searcher.Search(*req,resp);
            LOG(INFO)<< "Search end";
            done->Run();
        }
    };
}//end doc_server

int main(int argc,char* argv[]){
    base::InitApp(argc,argv);
    using namespace sofa::pbrpc;
    std::cout<<"==========================="<<std::endl;    
    std::cout<<"========服务器开始========="<<std::endl;    
    std::cout<<"==========================="<<std::endl;    
    std::cout<<"==========================="<<std::endl;    
    //索引模块的加载和初始化
    doc_index::Index* index = doc_index::Index::Instance();
    LOG(INFO) <<fLS::FLAGS_index_path;
    CHECK(index->Load(fLS::FLAGS_index_path));
    LOG(INFO) <<"Load Done!";
    //定义了一个Rpc对象，主要是为了定义线程池中线程的个数
    RpcServerOptions option;
    option.work_thread_num = 4;
    //定义了一个RpcServer对象,和ip端口号关联到一起
    RpcServer server(option);
    CHECK(server.Start("127.0.0.1:"+ fLS::FLAGS_port));
    //定义了一个DocServerAPIImpl,并注册到RpcServer对象中
    doc_server::DocServerAPIImpl* service_impl
        = new doc_server::DocServerAPIImpl();
    server.RegisterService(service_impl);
    LOG(INFO) <<"server start";
    server.Run();
    return 0;
    
}
