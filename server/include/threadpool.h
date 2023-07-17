//线程池使用的结构体及相关操作
# ifndef __THREADPOOL_H__
# define __THREADPOOL_H__

# include "head.h"
# include "workque.h"
typedef struct {
    int threadNum;      //线程数量
    pthread_t *pthid;   //线程ID数组
    que_t que;          //任务队列       
}threadPool_t, *pThreadPool_t;

//私有协议：小火车
typedef struct{
    int data_len;
    char buf[1000];
}train_t;

int threadPoolInit(pThreadPool_t pPool, int threadNum);
int threadPoolStart(pThreadPool_t pPool);

int tcpInit(int port);
int transFile(int clientFd);
int epoll_add(int epfd,int fd);
int child_handle(pQueNode_t pNew);
int sendn(int fd_send, char* send_buf, int len);
int recvn(int sfd,void *buf,int len);
int do_cd(int new_fd,char *buf);//
int do_ls(int new_fd) ;
int do_pwd(int new_fd,char *username);
int do_remove(int new_fd,char *cmd) ;
int do_gets(int new_fd,char *cmd);  
int do_puts(int new_fd,char *cmd);
int do_mkdir(int new_fd,char *cmd);
void get_salt(char *salt,char *passwd);
int map_recv(int sfd,void* pstart,int len);
void parse_arg(int argc, char*argv[]);
int insert(char *query);
int generate_salt(char *salt);
int update(char *query);
int Query(char *query,char *retval);
int Delete(char *query);
int getCwdFileId();

# endif

