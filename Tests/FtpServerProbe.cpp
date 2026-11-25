#include "FtpServer.h"
#include <iostream>
#include <unistd.h>
int main(int argc,char** argv) {
    if(argc!=2)return 2;
    std::string pass;if(!std::getline(std::cin,pass))return 2;
    FileWorkspace workspace(argv[1]);FtpServer server(workspace);
    if(!server.Start(FileWorkspace::Join(argv[1],"UserFiles"),"127.0.0.1",0,"owner",pass))return 3;
    std::cout<<server.Port()<<std::endl;
    for(unsigned i=0;i<25000;++i){server.Tick();usleep(1000);}
}
