#include "../include/head.h"
# include "../include/threadpool.h"
#include <stdarg.h>

const int DEBUG = 1;
extern char *optarg; // 引用optarg变量

void parse_arg(int argc, char*argv[]){
    int opt;
    const char *str = "p:t:";
    // 使用getopt函数解析命令行选项
    while ((opt = getopt(argc, argv, str)) != -1) {
        switch (opt) {
            case 'p':
                PORT = atoi(optarg);
                break;
            case 't':
                THREADNUM = atoi(optarg);
                break;
            default:
                break;
        }
    }
}

int tcpInit(int port)
{
    int listenFd = socket(AF_INET,SOCK_STREAM,0);
    ERROR_CHECK(listenFd,-1,"socket");
    
    struct sockaddr_in ser_addr;
    bzero(&ser_addr,sizeof(ser_addr));
    ser_addr.sin_family = AF_INET;
    ser_addr.sin_port = htons(port);
    ser_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    //设置可重用处于time_wait状态的地址
    int reuse = 1;
    int ret = setsockopt(listenFd,SOL_SOCKET,SO_REUSEADDR,&reuse,sizeof(int));
    ERROR_CHECK(ret,-1,"setsockopt");

    //bind the ip and port
    ret = bind(listenFd,(struct sockaddr*)&ser_addr,sizeof(ser_addr));
    ERROR_CHECK(ret,-1,"bind");

    listen(listenFd,128);
    ERROR_CHECK(ret,-1,"listen");

    return listenFd;
}


int epoll_add(int epfd,int fd)
{
    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = fd;

    int ret = epoll_ctl(epfd,EPOLL_CTL_ADD,fd,&event);
    ERROR_CHECK(ret,-1,"epoll_ctl");

    return 0;
}

int transFile(int new_fd)
{
    while(1);               ///这是什么操作？
    int ret;
    train_t t;
    //send the name of the file 
    t.data_len = strlen(FILENAME);
    strcpy(t.buf,FILENAME);
    ret = send(new_fd,&t,4+t.data_len,MSG_NOSIGNAL);
    ERROR_CHECK(ret,-1,"send");

    //open the file 
    int fd = open(FILENAME,O_RDONLY);
    ERROR_CHECK(fd,-1,"open");
    
    //send the size of the file 
    struct stat buf;
    ret = fstat(fd,&buf);
    ERROR_CHECK(ret,-1,"fstat");
    t.data_len = sizeof(buf.st_size);
    memcpy(t.buf,&buf.st_size,t.data_len);
    ret = send(new_fd,&t,4+t.data_len,MSG_NOSIGNAL);
    ERROR_CHECK(ret,-1,"send");

    //send the content of the file
    ret = sendfile(new_fd,fd,NULL,buf.st_size);
    ERROR_CHECK(ret,-1,"sendfile");
    return 0;
}

void Dprintf(const char* format, ...)
{
    if (DEBUG > 0) {
        va_list args;
        va_start(args, format);
        vprintf(format, args);
        va_end(args);
    }
}