#include"doc_searcher.h"
#include<base/base.h>
#include<boost/algorithm/string.hpp>
DEFINE_int32(desc_max_size,160,"描述的最大长度");

namespace doc_server{

bool DocSearcher::Search(const Request& req,Response* resp){
    //对查询进行分词
    //根据分词结果查找倒排
    //根据触发结果进行排序
    //根据排序进行查找正排
    //记录处理日志
    Context context(&req,resp);
    CutQuery(&context);
    Retrieve(&context);
    Rank(&context);
    PackageResponse(&context);
    Log(&context);
}

bool DocSearcher::CutQuery(Context* context){
    //使用结巴分词将关键词切分，去掉暂停词
    //调用Index的函数
    
    Index* index = Index::Instance();
    index->CutWordWithoutStopWord(context->req->query(),&context->words);
    return true;
}

bool DocSearcher::Retrieve(Context* context){
    //将所有的关键词的倒排信息存放
    //循环每个词，循环每个词里的每个weight数据结构
    Index* index = Index::Instance();
    for(const auto& word : context->words){
        const doc_index::InvertedList* inverted_list
            = index->GetInvertedList(word);
        if(inverted_list == NULL){
            continue;
        }
        for(size_t i = 0;i<inverted_list->size();i++){
            const auto& weight = (*inverted_list)[i];
            context->all_query_chain.push_back(&weight);
        }
    }
    return true;
}

bool DocSearcher::Rank(Context* context){
    //将所有的倒排信息进行整体的排序
    //直接采用sort库函数即可
    std::sort(context->all_query_chain.begin(),
              context->all_query_chain.end(),CmpWeightPtr);
    return true;
}

bool DocSearcher::CmpWeightPtr(const Weight* w1,const Weight* w2){
    return w1->weight() > w2->weight();
}

bool DocSearcher::PackageResponse(Context* context){
    //根据all_query_chain中的数据进行正排查询
    //将文档中的所有信息存放在相应的Item数组字段中
    Index* index=Index::Instance();
    const Request* req = context->req;
    Response* resp = context->resp;
    resp->set_sid(req->sid());
    resp->set_timestamp(common::Timeutil::TimeStamp());
    resp->set_err_code(0);
    for(const auto* weight : context->all_query_chain){
        const auto* doc_info = index->GetDocInfo(weight->id());
        auto* item=resp->add_item();
        item->set_title(doc_info->title());
        item->set_desc(GenDesc(weight->first_pos(),doc_info->content()));
        item->set_jump_url(doc_info->jump_url());
        item->set_show_url(doc_info->show_url());
    }
    return true;
}

std::string DocSearcher::GenDesc(int first_pos,const std::string& content){
    //主要显示的是关键字，第一次的位置和之前的部分内容
    //需要找到第一个位置，如果没有，就直接从正文的开始位置
    int64_t desc_beg = 0;
    if(first_pos != -1){
        desc_beg = common::StringUtil::FindSentenceBeg(content,first_pos);
    }
    std::string desc;
    if(desc_beg + fLI::FLAGS_desc_max_size >= (int32_t)content.size()){
        desc = content.substr(desc_beg);
    }else{
        desc = content.substr(desc_beg,fLI::FLAGS_desc_max_size);
        desc[desc.size() - 1] = '.';
        desc[desc.size() - 2] = '.';
        desc[desc.size() - 3] = '.';
    }

    ReplaceEscape(&desc);
    return desc;
}

void DocSearcher::ReplaceEscape(std::string* desc){
    boost::algorithm::replace_all(*desc,"&","&amp;");
    boost::algorithm::replace_all(*desc,"\"","&quot;");
    boost::algorithm::replace_all(*desc,"<","&lt;");
    boost::algorithm::replace_all(*desc,">","&gt;");
}

bool DocSearcher::Log(Context* context){
    LOG(INFO) << "[Request]" << context->req->Utf8DebugString();
    LOG(INFO) << "[Response]" << context->resp->Utf8DebugString();
    return true;
}

}//end doc_server
