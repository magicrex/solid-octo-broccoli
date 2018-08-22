#include"httpserver.h"


int main(int argc,char* argv[])
{
    std::cout<<"*********************************"<<std::endl;
    std::cout<<"*************服务器运行**********"<<std::endl;
    std::cout<<"*********************************"<<std::endl;
    std::cout<<"*********************************"<<std::endl;
    base::InitApp(argc,argv);
	httpserver::http_server server;
	server.start(argc,argv);
	return 0;
}

