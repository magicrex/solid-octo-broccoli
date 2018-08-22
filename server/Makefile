PROTOC=../bin/protoc

FLAG= -g -std=c++11 -lpthread -I../include -L../lib 
FLAGS= -lsofa-pbrpc -lprotobuf -lglog -lgflags -lz -lsnappy

server:server_main.cc server.pb.cc doc_searcher.cpp ../index/cpp/libindex.a
	g++ $^ -o $@ $(FLAG) $(FLAGS)

server.pb.cc:server.proto
	$(PROTOC) server.proto --cpp_out=.

.PHONY:clean
clean:
	rm -f server server.pb.*



