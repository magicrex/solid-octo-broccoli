#include"index.h"

DEFINE_string(dict_path,"./jieba_dict/jieba.dict.utf8","词典库");
DEFINE_string(hmm_path,"./jieba_dict/hmm_model.utf8","hmm 词典库");
DEFINE_string(user_path,"./jieba_dict/user.dict.utf8","用户自定义词典");
DEFINE_string(idf_path,"./jieba_dict/idf.utf8","idf 词库");
DEFINE_string(stop_word_path,"./jieba_dict/stop_words.utf8","暂停词库");

namespace doc_index {


////////////////////////////////////////////////////
//Init函数
///////////////////////////////////////////////////
Index* Index::inst_=NULL;

Index::Index():jieba_(fLS::FLAGS_dict_path,
                      fLS::FLAGS_hmm_path,
                      fLS::FLAGS_user_path,
                      fLS::FLAGS_idf_path,
                      fLS::FLAGS_stop_word_path){
    if(!stop_word_dict_.Load(fLS::FLAGS_stop_word_path)){
        LOG(FATAL) << "加载失败";
    }
}



////////////////////////////////////////////////////
//Bulid函数
///////////////////////////////////////////////////
void Index::SplitTitle(const std::string& title,DocInfo* doc_info){
    std::vector<cppjieba::Word> words;
    jieba_.CutForSearch(title,words);
    if(words.size() == 0){
        LOG(FATAL) << "标题出错，没有标题";
    }
    for(size_t i=0;i<words.size();i++){
        auto* token = doc_info->add_title_token();
        token->set_begin(words[i].offset);
        if(i + 1 < words.size()){
            token->set_end(words[i+1].offset);
        }else{
            token->set_end(title.size());
        }
    }
}
void Index::SplitContent(const std::string& content,DocInfo* doc_info){
    std::vector<cppjieba::Word> words;
    jieba_.CutForSearch(content,words);
    if(words.size()==0){
        LOG(FATAL) << "内容出错，没有内容";
    }
    for(size_t i=0;i<words.size();i++){
        auto* token = doc_info->add_content_token();
        token->set_begin(words[i].offset);
        if(i + 1 < words.size()){
            token->set_end(words[i+1].offset);
        }else{
            token->set_end(content.size());
        }
    }
}




const DocInfo* Index::BuildForward(const std::string& line){
    //line实际上是一个html中的所有内容，所以需要读出Doinfo的
    //每一字段的内容，进行字符串切分，并赋值给每个字段
    //比较难得是最后两个分词结果
    //将Doinfo插入到正排索引
    //showurl并没有做处理，使他和jump相同
    //实际上是jump的域名部分
    std::vector<std::string> tokens;
    common::StringUtil::Split(line,&tokens,"\3");
    if(tokens.size() !=3){
        LOG(FATAL) << "Split error tokens.size= "<<tokens.size();
    }
    DocInfo doc_info;
    doc_info.set_id(forward_index_.size());
    doc_info.set_title(tokens[1]);
    doc_info.set_content(tokens[2]);
    doc_info.set_jump_url(tokens[0]);
    doc_info.set_show_url(tokens[0]);
    SplitTitle(tokens[1],&doc_info);
    SplitContent(tokens[2],&doc_info);
    forward_index_.push_back(doc_info);
    return &forward_index_.back();
}

int Index::CalcWeight(const WordCnt word_pair){
    int weight= word_pair.title_cnt * 30 + word_pair.content_cnt;  
    return weight;
}


void Index::BuildInverted(const DocInfo& doc_info){
    //根据每个词的次数作为权重，并存入hash表中
    //value有两个一个是标题次数，一个内容次数
    //根据统计结果，更新到倒排索引之中
    //做到大小不敏感，暂停词需要干掉
    WordCntMap word_cnt_map;
    for(int i=0;i<doc_info.title_token_size();i++){
        const auto& token=doc_info.title_token(i);
        std::string word=doc_info.title().substr(token.begin(),token.end() - token.begin());
        boost::to_lower(word);
        if(stop_word_dict_.Find(word)){
            continue;
        }
        ++word_cnt_map[word].title_cnt;
    }
    for(int i=0;i<doc_info.content_token_size();i++){
        const auto& token=doc_info.content_token(i);
        std::string word=doc_info.content().substr(token.begin(),token.end() - token.begin());
        boost::to_lower(word);
        if(stop_word_dict_.Find(word)){
            continue;
        }
        ++word_cnt_map[word].content_cnt;
        if(word_cnt_map[word].content_cnt == 1){
            word_cnt_map[word].first_pos = token.begin();
        }
    }

    for (const auto& word_pair : word_cnt_map){
        Weight weight;
        weight.set_id(doc_info.id());
        weight.set_weight(CalcWeight(word_pair.second));
        weight.set_first_pos(word_pair.second.first_pos);
        InvertedList& inverted_list = inverted_index_[word_pair.first];
        inverted_list.push_back(weight);
    }
}

bool Index::CmpWeight(const Weight& w1,const Weight& w2){
    return w1.weight() > w2.weight();
}

void Index::SortInverted(){
    for(auto& inverted_pair:inverted_index_){
        InvertedList& inverted_list = inverted_pair.second;
        std::sort(inverted_list.begin(),inverted_list.end(),CmpWeight);
    }
}

//从raw_input文件中读数据，在内存中构建索引结构
bool Index::Build(const std::string& input_path){
    LOG(INFO) << "Index Build";
    //1.按行读取文件内容，针对读到的每一行数据进行处理
    //2.把这一行数据制作成一个 DocInfo
    //3.更新倒排信息
    //4.针对所有的倒排进行排序
    std::ifstream file(input_path.c_str());
    CHECK(file.is_open()) <<"input_path: "<<input_path;
    std::string line;
    while(std::getline(file,line)){
        const DocInfo* doc_info = BuildForward(line);
        CHECK(doc_info!=NULL);
        BuildInverted(*doc_info);
    }
    SortInverted();
    file.close();
    LOG(INFO)<< "Index Build Done.";
    return true;
}
////////////////////////////////////////////////////
//Save函数
///////////////////////////////////////////////////



//把内存中的索引数据保存到磁盘上
bool Index::Save(const std::string& output_path){
    //内存中的索引保存到两个文件中
    //分别是forward.txt Inverted.txt
    //首先要把数据从数据结构中去出来中取出来
    //然后存放在文件中
    //在protrbuf中已经定义了一个index结构只需要将
    //数据存入，然后序列化存入文件中即可
    LOG(INFO) <<"Index Save begin";
    std::string proto_data;
    doc_index_proto::Index index;
    for(const auto& item : forward_index_){
        auto* index_add_forward = index.add_forward_index();
        *index_add_forward = item;
    }
    for(const auto& item : inverted_index_){
        auto* index_add_inverted = index.add_inverted_index();
        index_add_inverted->set_key(item.first);
        for(auto& it : item.second){
            auto* Invert_add_weight = index_add_inverted->add_doc_list();
            *Invert_add_weight = it;
        }
    }
    index.SerializeToString(&proto_data);
    CHECK(common::FileUtil::ToFile(output_path,proto_data)); 
    LOG(INFO) <<"Index Save end";
    return true;
}




////////////////////////////////////////////////////
//Load函数
///////////////////////////////////////////////////

//把磁盘上的文件加载到内存的索引结构汇中
bool Index::Load(const std::string& index_path){
    //先将数据从文件中读出来
    //接着是反序列化，再将数据存入到一个index数据结构中
    //将数据结构赋值给对象的成员变量
    LOG(INFO) <<"Index Load Begin";
    std::cout<<"begin"<<std::endl;
    std::string proto_data;
    std::cout<< index_path.c_str()<<std::endl;
    CHECK(common::FileUtil::FromFile(index_path,&proto_data));
    std::cout<<"duquwenjian "<<std::endl;
    doc_index_proto::Index index;
    index.ParseFromString(proto_data);
    LOG(INFO) <<"Index Load Begin";
    for(int i=0;i < index.forward_index_size() ; i++){
        const auto& doc_info = index.forward_index(i);
        forward_index_.push_back(doc_info);
    }
    LOG(INFO) <<"Index Load Begin";
    for(int i=0 ; i<index.inverted_index_size() ; i++){
        const auto& forward_info = index.inverted_index(i);
        InvertedList& invert_list = inverted_index_[forward_info.key()];
        for(int j = 0; j<forward_info.doc_list_size();j++){
            const auto& weight = forward_info.doc_list(j);
            invert_list.push_back(weight);
        }
    }
    std::cout<<"Load end2"<<std::endl;
    return true;
}




////////////////////////////////////////////////////
//dump函数
///////////////////////////////////////////////////

//调试用的接口，把内存中的所以数据按照一定的格式打印到文件中
bool Index::Dump(const std::string& forward_dump_path,
                 const std::string& inverted_dump_path){
    //直接进行遍历，将正排索引和倒排索引按行的格式打印到一个文件中
    LOG(INFO) << "Index Dump begin";
    std::ofstream FDump_file(forward_dump_path);
    CHECK(FDump_file.is_open());
    for(size_t i=0;i<forward_index_.size();i++ ){
        const auto& doc_info = forward_index_[i];
        FDump_file << doc_info.Utf8DebugString();
    }
    FDump_file.close();
    std::ofstream IDump_file(inverted_dump_path);
    CHECK(IDump_file.is_open());
    for(const auto& inverted_pair : inverted_index_){
        IDump_file << inverted_pair.first <<"\n";
        for (const auto& item : inverted_pair.second){
            IDump_file << item.Utf8DebugString()<<"\n";
        }
        IDump_file <<"==================="<<"\n";
    }
    IDump_file.close();
    LOG(INFO) << "Index Dump end";
    return true;
}



////////////////////////////////////////////////////
//正排索引
///////////////////////////////////////////////////

//根据doc_id获取到文档的详细信息
const DocInfo* Index::GetDocInfo(uint64_t doc_id) const{
    if(doc_id >= forward_index_.size()){
        return NULL;
    }
    return &forward_index_[doc_id];
}



////////////////////////////////////////////////////
//倒排索引
///////////////////////////////////////////////////

//根据关键词获取到文档id
const InvertedList* Index::GetInvertedList(const std::string key) const{
    auto it = inverted_index_.find(key);
    if(it == inverted_index_.end())
        return NULL;
    return  &(it->second);
}//end Index



void Index::CutWordWithoutStopWord(const std::string& query,std::vector<std::string>* words){
    words->clear();
    std::vector<std::string>tmp;
    jieba_.CutForSearch(query,tmp);
    for(std::string& token : tmp){
        boost::to_lower(token);
        if(stop_word_dict_.Find(token)){
            continue;
        }
        words->push_back(token);
    }
}

}//end doc_index
