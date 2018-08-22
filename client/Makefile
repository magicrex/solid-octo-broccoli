PROTOC=../bin/protoc

FLAG= -g -std=c++11 -lpthread -I../include -L../lib 
FLAGS= -lprotobuf -lsofa-pbrpc -lglog -lgflags -lz -lsnappy -lctemplate

client:client_main.cpp server.pb.cc ./libindex.a
	g++ $^ -o $@ $(FLAG) $(FLAGS)
	cp client ../wwwroot/

server.pb.cc:server.proto
	$(PROTOC) server.proto --cpp_out=.

.PHONY:clean
clean:
	rm -f client server.pb.*



