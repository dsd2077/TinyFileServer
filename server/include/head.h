#ifndef __HEAD_H__
#define __HEAD_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h> 
#include <unistd.h>
#include <dirent.h>
#include <grp.h>
#include <pwd.h>
#include <time.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <signal.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <ctype.h>
#include <sys/epoll.h>
#include <sys/sendfile.h>
#include <shadow.h>
#include <crypt.h>
#include <sys/mman.h>
#define ARGS_CHECK(argc,num){if(argc!=num){\
    fprintf(stderr,"参数错误！\n");\
    return -1;\
    }}
#define ERROR_CHECK(ret,num,msg){if(ret==num){\
    perror(msg);\
    return -1;\
    }}
#define TERROR_CHECK(ret,msg){if(ret != 0){\
    printf("%s is failed,%s\n",msg,strerror(ret));\
    return -1;\
    }}

#define FILENAME "file"

extern int  PORT;                   //端口
extern int THREADNUM;                  //线程数
extern const int MAX_EVENT_NUMBER; //最大事件数

#endif

