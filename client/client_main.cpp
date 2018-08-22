#include<base/base.h>
#include<sofa/pbrpc/pbrpc.h>
#include<ctemplate/template.h>
#include"server.pb.h"
#include"./util.hpp"
#include<iostream>
#include<string>
DEFINE_string(server_addr,"127.0.0.1:10000","请求的搜索服务器的地址");
DEFINE_string(template_path,"../wwwroot/index3.tpl","模板的路径");

namespace doc_client{
    typedef doc_server_proto::Request Request;
    typedef doc_server_proto::Response Response;
    
    void HttpResponse(const std::string& body){
        std::cout<<"Content-Type: text/html"<<"\n";
        std::cout<<"Content-Length:"<<body.size()<<"\n";
        std::cout<<"\n";
        std::cout<<body;
    }

    int GetQueryString(std::string& buf){
        char* method = getenv("REQUEST_METHOD");
        if(method == NULL){
            fprintf(stderr,"REQUEST_METHOD failed\n");
            return -1;
        }
        if(strcmp(method,"GET") == 0){
            char* query_string = getenv("QUERY_STRING");
            if(query_string == NULL){
                fprintf(stderr,"QUERY_STRING failed\n");
                return -1;
            }
            std::vector<std::string> arr;
            common::StringUtil::Split(query_string,"=",&arr);
            buf=arr[1];

        }else{
            fprintf(stderr,"METHOD TYPE failed\n");
            return -1;
        }
        return 0;
    }

    void PackageRrquest(Request* req){
        //分别设置三个参数
        req->set_sid(0);
        req->set_timestamp(common::Timeutil::TimeStamp());
        std::string buf;
        GetQueryString(buf);
        std::string query;
        query="query="+buf;
        req->set_query(query.c_str());
        LOG(INFO)<< query;

    }

    void Search(const Request& req,Response* resp){
        //主要是给搜索服务器发送一个请求
        //并接受响应
        using namespace sofa::pbrpc;
        //先定义一个PRC client对象
        RpcClient client;
        //再定义一个RpcChannel对象，描述一个连接
        RpcChannel channel(&client,fLS::FLAGS_server_addr);
        //定义一个DocServerAPI_Stub，描述调用服务器
        doc_server_proto::DocServerAPI_Stub stub(&channel);
        //定义一个ctrl对象，用于网络控制的对象
        RpcController ctrl;
        ctrl.SetTimeout(3000);
        //调用远程服务
        stub.Search(&ctrl,&req,resp,NULL);
        //检查是否成功
        if(ctrl.Failed()){
            std::cerr << "RPC Search failed\n";

        }else{
            std::cerr << "RPC Search OK\n";
        }
    }

    void ParseResponse(const Response& resp){
        ctemplate::TemplateDictionary dict("Page");
        for(int i=0;i<resp.item_size();++i){
            ctemplate::TemplateDictionary* table
                =dict.AddSectionDictionary("TABLE");
            table->SetValue("VAL1",resp.item(i).title());
            table->SetValue("VAL2",resp.item(i).desc());
            table->SetValue("VAL3",resp.item(i).jump_url());
            table->SetValue("VAL4",resp.item(i).show_url());
        }
        if(1){
            dict.ShowSection("TABLE"); 
        }
        ctemplate::Template* tpl = ctemplate::Template::GetTemplate(
                                                                    fLS::FLAGS_template_path,
                                                                    ctemplate::DO_NOT_STRIP);
        std::string html;
        tpl->Expand(&html,&dict);
        HttpResponse(html);
        return ;
    }

    void CallServer(){
        //构造一个请求发送给搜索服务器
        //把服务器返回的响应进行解析
        //将解析的结果构造成http响应输出
        Request req;
        Response resp;
        PackageRrquest(&req);
        Search(req,&resp);
        ParseResponse(resp);
        return ;
    }

}






int main(int argc,char* argv[]){
    base::InitApp(argc,argv);
    doc_client::CallServer();
    return 0;
}
