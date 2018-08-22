#include"index.h"

DEFINE_string(input_path,"../data/output/index_file","数据输入路径");
DEFINE_string(forward_output_path,"../data/output/forward_file","索引文件路径");
DEFINE_string(inverted_output_path,"../data/output/inverted_file","索引文件路径");

int main(int argc,char* argv[]){
    base::InitApp(argc,argv);
    doc_index::Index* index = doc_index::Index::Instance();
    CHECK(index->Load(fLS::FLAGS_input_path));
    CHECK(index->Dump(fLS::FLAGS_forward_output_path,fLS::FLAGS_inverted_output_path));
    return 0;
}
