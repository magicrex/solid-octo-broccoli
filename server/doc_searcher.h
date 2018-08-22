#pragma once

#include"server.pb.h"
#include"../index/cpp/index.h"

namespace doc_server{
typedef doc_server_proto::Request Request;
typedef doc_server_proto::Response Response;
typedef doc_index_proto::Weight Weight;
typedef doc_index::Index Index;

struct Context{
    const Request* req;
    Response* resp;

    std::vector<std::string> words;
    std::vector<const Weight*> all_query_chain;

    Context(const Request* request , Response* response)
        :req(request),resp(response){  }
};

class DocSearcher{
public:
    //搜索流程的入口函数
    bool Search(const Request& req,Response* resp);

private:
    //对查询西进行分词
    bool CutQuery(Context* context);
    //根据查询词进行结果触发
    bool Retrieve(Context* context);
    //根据触发结果进行排序
    bool Rank(Context* context);
    //根据排序的结构拼装成响应
    bool PackageResponse(Context* context);
    //打印请求日志
    bool Log(Context* context);
    //排序需要的请求函数
    static bool CmpWeightPtr(const Weight* w1,const Weight* w2);
    //生成描述信息
    std::string GenDesc(int first_pos,const std::string& content);
    //替换 html中的转义字符
    static void ReplaceEscape(std::string* desc);

    
};//end DocSearcher
}//end doc_server
