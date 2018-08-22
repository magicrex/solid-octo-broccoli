PROTOC=../../bin/protoc
BASEO =-std=c++11 -lpthread  -I../../include -L../../lib 
BASET =-lprotobuf -lgflags -lglog -g -lboost_system
.PHONY:all
all:index_builder index_dump 
	
index_builder:index_builder.cpp libindex.a
	g++ index_builder.cpp ./libindex.a $(BASEO) $(BASET) -o $@
index_dump:index_dump.cpp libindex.a
	g++ index_dump.cpp ./libindex.a $(BASEO) $(BASET) -o $@
libindex.a:index.cpp index.pb.cc 
	gcc -c index.cpp -o index.o $(BASEO) $(BASET)
	gcc -c index.pb.cc -o index.pb.o $(BASEO) $(BASET)
	ar -crv libindex.a index.pb.o index.o

index.pb.cc:index.proto 
	$(PROTOC) index.proto --cpp_out=.
	

.PHONY:clean
clean:
	rm -f index.pb.h index.pb.cc index.o index.pb.o 
	rm -f libindex.a index_builder index_dump
