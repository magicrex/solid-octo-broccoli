#pragma once
#include<vector>
#include<unordered_map>
#include"index.pb.h"
#include<cppjieba/Jieba.hpp>
#include<base/base.h>
#include"util.hpp"
#include<fstream>
namespace doc_index{

typedef doc_index_proto::DocInfo DocInfo;
typedef doc_index_proto::Weight Weight;
typedef std::vector<DocInfo> ForWardIndex;
typedef std::vector<Weight> InvertedList;
typedef std::unordered_map<std::string,InvertedList> InvertedIndex;


struct WordCnt{
    int title_cnt;
    int content_cnt;
    int first_pos;

    WordCnt():title_cnt(0),content_cnt(0),first_pos(-1) {}
};

typedef std::unordered_map<std::string,WordCnt>  WordCntMap;
//a.构建，raw_input中的相关内容进行解析在内存中构造出索引结构(hash
//b,保存，把内存中的索引结构进行序列化，存在文件中，序列化依赖于刚才的index.proto 制作索引的
//可执行程序来调用保存
//c.加载，把磁盘上的索引文件读取出来，反序列化，生成内存中的索引结构 搜索服务器
//d.反解，内存中的索引结果按照一定的格式打印出来，方便测试
//e.查正排，给定文档id获取到文档的详细信息
//f.查倒排,给定关键词，获取到和关键词相关的文档列表
class Index{
public:
    Index();

    static Index* Instance(){
        if(inst_ == NULL){
            inst_ = new Index();
        }
        return inst_;
    }

    //从raw_input文件中读数据，在内存中构建索引结构
    bool Build(const std::string& input_path);

    //把内存中的索引数据保存到磁盘上
    bool Save(const std::string& output_path);

    //把磁盘上的文件加载到内存的索引结构汇中
    bool Load(const std::string& index_path);

    //调试用的接口，把内存中的所以数据按照一定的格式打印到文件中
    bool Dump(const std::string& forward_dump_path,
              const std::string& inverted_dump_path);

    //根据doc_id获取到文档的详细信息
    const DocInfo* GetDocInfo(uint64_t doc_id) const;

    //根据关键词获取到文档id
    const InvertedList* GetInvertedList(const std::string key) const;

    void CutWordWithoutStopWord(const std::string& query,std::vector<std::string>* words);

private:
    ForWardIndex forward_index_;
    InvertedIndex inverted_index_;
    cppjieba::Jieba jieba_;
    common::DictUtil stop_word_dict_;    
    //以下为具体每步函数
    const DocInfo* BuildForward(const std::string& line);
    void BuildInverted(const DocInfo& doc_info);
    void SortInverted();
    void SplitTitle(const std::string& title,DocInfo* doc_info);
    void SplitContent(const std::string& content,DocInfo* doc_info);
    int CalcWeight(const WordCnt word_pair);
    static bool CmpWeight(const Weight& w1,const Weight& w2);
    static Index* inst_;
};//end Index

}//end doc_index
