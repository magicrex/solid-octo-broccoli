#pragma once
#include<iostream>
#include<sys/socket.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<string>
#include<vector>
#include<fstream>
#include<unordered_map>
#include<unordered_set>
#include<boost/algorithm/string.hpp>
#include<boost/regex.hpp>
#include<boost/algorithm/string/regex.hpp>
#include<boost/filesystem.hpp>
#include<time.h>
#include<sys/time.h>

namespace common{

class Timeutil{
	public:
		static int64_t TimeStamp(){
			struct timeval tv;
			gettimeofday(&tv,NULL);
			return tv.tv_sec;
		}
		static int64_t TimeStampUs(){
			struct timeval tv;
			gettimeofday(&tv,NULL);
			return 1000*1000*tv.tv_sec+tv.tv_usec;
		}
		
};//end Timeutil


class FileUtil{
public:
    
    

    static bool ToFile(const std::string& output_path,
                      const std::string& content) {
        std::ofstream file(output_path.c_str());
        if (!file.is_open()) {
            return false;

        }
        file.write(content.data(), content.size());
        file.close();
        return true;
    }

    static bool FromFile(const std::string& input_path, 
                             std::string* content) {                                              
        std::ifstream file(input_path.c_str());
        if (!file.is_open()) {
            return false;
        }   
        // 需要先获取到文件的长度
        // 把文件光标放到文件末尾
        file.seekg(0, file.end);
        // 获取到文件光标的位置
        int length = file.tellg();
        file.seekg(0, file.beg);
        content->resize(length);
        file.read(const_cast<char*>(content->data()), length);
        file.close();
        return true;
    }



    static int Readline(int fd,std::string* line){
        //以\n \r \r\n为界定标识的
        //返回时不带界定标识符
        line->clear(); 
        line->shrink_to_fit();
        while(true){
            char c='\0';
            ssize_t read_size=recv(fd,&c,1,0);
            if(read_size < 0){
                return -1;
            }
            if(c == '\r'){
                //窥探下一个字符
                recv(fd,&c,1,MSG_PEEK);
                if(c == '\n'){
                    recv(fd,&c,1,0);
                }else{
                    break;
                }
            }
            if(c == '\n'){
                break;
            }
            line->push_back(c);
        }//end while
        return 1;
    }//end Readline
    //写入一个string中，不适用于数据过大
    static int ReadN(int fd,size_t len,std::string* output){
        output->clear();
        output->shrink_to_fit();
        char c='\0';
        while(len--){
            recv(fd,&c,1,0);
            output->push_back(c);
        }
        return 1;
    }//end ReadN
    //上传文件时数据过大，写入缓存文件中
    static int ReadNFile(int fd,size_t len,std::string input){
        std::ofstream cache;
        std::string cachename("./wwwroot/cache/");
        cachename=cachename+input;
        cache.open(cachename,std::ofstream::out);
        char c='\0';
        while(len--){
            recv(fd,&c,1,0);
            cache<<c;
        }
        return 1;
    }//end ReadNFile

    static bool IsDir(const std::string& url){
        return boost::filesystem::is_directory(url);
    }

    static int ReadAll(const std::string& path,std::string* output){
        //读取文件的所有内容
        std::ifstream file(path.c_str());
        if(!file.is_open()){
            return -1;
        }
        //调整文件指针的位置
        file.seekg(0,file.end);
        //查询文件指针的位置
        int  length=file.tellg();
        //再次把指针设置到文件开头
        file.seekg(0,file.beg);
        output->resize(length);
        file.read(const_cast<char*>(output->c_str()),length);
        file.close();
        return 1;
    }

    static int ReadAll(int fd,std::string* output){
        while(true){
            char buf[1024*5]={0};
            ssize_t read_size=read(fd,buf,sizeof(buf)-1);
            if(read_size<0){
                return -1;
            }
            if(read_size==0){
                return 1;
            }
            buf[read_size]='\0';
            *output += buf;
        }
        return 1;
    }
};//end FileUtil


class StringUtil{
public:
    static int Split(const std::string& input,const std::string& split_char,std::vector<std::string>* output){
        //参数一输入字符串，参数二用来作为分割的字符，参数三输出一个string的数组
        boost::split(*output,input,boost::is_any_of(split_char),boost::token_compress_off);
        //输出参数的引用，输入参数，分割符可以有多个每个参数都起作用，表示有连续分割符时，是否作为一个空字符。 
        return 1;
    }//end Split

    static void Split(const std::string& input,
                      std::vector<std::string>* output,
                      const std::string& split_char) {
        boost::split(*output, input,
                     boost::is_any_of(split_char),
                     boost::token_compress_off);

    }

    static int Split_Regex(const std::string& input,const std::string& split_string,std::vector<std::string>* output){
        boost::split_regex(*output,input,boost::regex(split_string.c_str()));
        return 1;
    }
    typedef std::unordered_map<std::string,std::string> UrlParam;
    static int ParseUrlParan(const std::string& input,UrlParam* output){
        std::vector<std::string> parmas;
        Split(input,"&",&parmas);
        for(auto i : parmas){
            std::vector<std::string> kv;
            Split(i,"=",&kv);
            if(kv.size() != 2){
                continue;
            }
            (*output)[kv[0]]=kv[1];
        }
        return 1;
    }

    static int64_t FindSentenceBeg(const std::string& content ,int32_t first_pos){
        for(int32_t i = first_pos ; i>=0;--i){
            if(content[i] == ';' || content[i] == ','
               || content[i] == '?' || content[i] == '!'
               || (content[i] == '.' && content[i+1] == ' ' )){
                return i+1;
            }
        }
        return 0;
    }
};//end StringUtil


class DirUtil{
public:
    static int CreateDir(std::string name){
        std::string prepath=("./wwwroot/AllFile/");
        prepath=prepath+name;
        int ret=mkdir(prepath.c_str(),S_IRWXU|S_IRWXG|S_IROTH|S_IXOTH);
        if(ret==0)
            return 1;
        else
            return 0;
    }
};//end DirUtil


class UrlUtil{
public:
    static char dec2hexChar(short int n) {
        if ( 0 <= n && n <= 9  ) {
            return char( short('0') + n  );
        } else if ( 10 <= n && n <= 15  ) {
            return char( short('A') + n - 10  );
        } else {
            return char(0);
        }
    }

    static short int hexChar2dec(char c) {
        if ( '0'<=c && c<='9'  ) {
            return short(c-'0');

        } else if ( 'a'<=c && c<='f'  ) {
            return ( short(c-'a') + 10  );

        } else if ( 'A'<=c && c<='F'  ) {
            return ( short(c-'A') + 10  );

        } else {
            return -1;
        }
    }

    static std::string escapeURL(const std::string &URL)
    {
        std::string result = "";
        for ( unsigned int i=0; i<URL.size(); i++  ) {
            char c = URL[i];
            if (
                ( '0'<=c && c<='9'  ) ||
                ( 'a'<=c && c<='z'  ) ||
                ( 'A'<=c && c<='Z'  ) ||
                c=='/' || c=='.'

               ) {
                result += c;

            } else {
                int j = (short int)c;
                if ( j < 0  ) {
                    j += 256;

                }
                int i1, i0;
                i1 = j / 16;
                i0 = j - i1*16;
                result += '%';
                result += dec2hexChar(i1);
                result += dec2hexChar(i0);
            }
        }
        return result;
    }

    static std::string deescapeURL(const std::string &URL) {
        std::string result = "";
        for ( unsigned int i=0; i<URL.size(); i++  ) {
            char c = URL[i];
            if ( c != '%'  ) {
                result += c;

            } else {
                char c1 = URL[++i];
                char c0 = URL[++i];
                int num = 0;
                num += hexChar2dec(c1) * 16 + hexChar2dec(c0);
                result += char(num);
            }
        }
        return result;
    }
};//end UrlUtil


class DictUtil{
public:
    bool Load(const std::string& path){
        std::ifstream file(path.c_str());
        if(!file.is_open()){
            return false;

        }
        std::string line;
        while(std::getline(file,line)){
            set_.insert(line);

        }
        return true;

    }

    bool Find(const std::string& key) const{
        return set_.find(key) != set_.end();
    }


private:
    std::unordered_set<std::string> set_;

};//end DictUtil
}//end common
