#include"index.h"

DEFINE_string(input_path,"../data/tmp/raw_input","数据输入路径");
DEFINE_string(output_path,"/home/master/Git/sort/index/data/output/index_file","索引文件路径");

int main(int argc,char* argv[]){
    base::InitApp(argc,argv);
    doc_index::Index* index = doc_index::Index::Instance();
    CHECK(index->Build(fLS::FLAGS_input_path));
    CHECK(index->Save(fLS::FLAGS_output_path));
    return 0;
}
