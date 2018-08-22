.PHONY:all

BASE=-std=c++11 -lboost_system -lboost_filesystem
DATA1=-I../include -I../lib -lglog -lgflags
TEMP=-lctemplate -lpthread
all:httpserver 

httpserver:httpserver.cpp  httpserver_main.cpp
	g++ $^ -o $@ $(BASE) -lpthread $(DATA1) $(DATA2) -g

.PHONY:clean
clean:
	rm -f httpserver 
