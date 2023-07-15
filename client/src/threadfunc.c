#define _GNU_SOURCE
#include "../include/multi_process.h"
extern char msg[1];
extern char token[1024];
extern struct sockaddr_in ser_addr;
# define SPLICESIZE 8192
//puts线程函数
void *threadFunc1(void *p)
{
    char *cmd = (char *)p;

    //初始化一个socket描述符，用于tcp通信
    int serverFd = socket(AF_INET,SOCK_STREAM,0);
    if (-1 == serverFd) {
        printf("error in socket\n");
        return NULL;
    }

    int ret=connect(serverFd,(struct sockaddr*)&ser_addr,sizeof(ser_addr));
    if (-1 == ret) {
        printf("error in connect\n");
        return NULL;
    }
    /* printf("serverFd = %d, cmd = %s", serverFd, cmd); */

    //'2'表示第二次连接
    send(serverFd, "2", 1, 0);

    //发送2标志，代表二次登陆
    ret = send(serverFd, "2", 1, 0);
    send(serverFd, token, strlen(token), 0);

    recv(serverFd, msg, 1, 0);
    if ('0' == msg[0]) {
        printf("二次登陆认证失败\n");
        return NULL;
    }

    send(serverFd, cmd, strlen(cmd)-1, 0);
    
    train_t t;
    char file_name[64] = {0};
    char md5sum[64] = {0};
    off_t fileSize = 0;//the total size of the file
    off_t slice = 0;
    time_t start, end;

    sscanf(cmd+4,"%s",file_name);
    int fd = open(file_name,O_RDONLY);
    if (fd == -1) {
        printf("invalid command!type 'help' for help\n");
        fileSize = 0;
        send(serverFd, &fileSize, sizeof(fileSize), 0);//<puts:1:case1>命令不合法，给服务器发送文件大小等于0
    } else {
        struct stat statbuf;
        fstat(fd, &statbuf);
        fileSize = statbuf.st_size;
        slice = fileSize / 10000;

        send(serverFd, &fileSize, sizeof(fileSize), 0);//<puts:1:case2>

        //caculate md5sum of the file
        cal_md5(file_name,md5sum);

        //send the md5sum of the file
        send(serverFd,md5sum,strlen(md5sum),0);//<puts:2>

        recv(serverFd,msg,1,0);//<puts:3>

        if (msg[0] == '1') {//<puts:3:case1>
            time(&start);
            //send the content of the file
            while((t.data_len = read(fd, t.buf, sizeof(t.buf))))//once reaching the end of the file ,t.data_len will be set zero
            {
                send(serverFd, &t, t.data_len+4, 0);
            }

            //send a zero on be half of the end of the file
            t.data_len = 0;
            send(serverFd,&t,4,0);
            printf("100.00%%\n");
            printf("upload success!\n");
            time(&end);
            printf("cost time = %lds\n", end-start);
        } else if (msg[0] == '2') {//<puts:3:case2>
            printf("秒传成功！\n");

        } else {
            printf("the file has already been uploaded,do not repeat operation\n");
        }
    }

    close(serverFd);
    pthread_exit(NULL);
}

//gets线程函数
void *threadFunc2(void *p)
{
    char *cmd = (char *)p;

    //初始化一个socket描述符，用于tcp通信
    int serverFd = socket(AF_INET,SOCK_STREAM,0);
    if (-1 == serverFd) {
        printf("error in socket\n");
        return NULL;
    }

    int ret = connect(serverFd,(struct sockaddr*)&ser_addr,sizeof(ser_addr));
    if (-1 == ret) {
        printf("error in connect\n");
        return NULL;
    }

    //'2'表示第二次连接
    send(serverFd, "2", 1, 0);

    //发送2标志，代表二次登陆
    ret = send(serverFd, "2", 1, 0);
    send(serverFd, token, strlen(token), 0);

    recv(serverFd, msg, 1, 0);
    if ('0' == msg[0]) {
        printf("二次登陆认证失败\n");
        return NULL;
    }

    send(serverFd, cmd, strlen(cmd)-1, 0);

    char file_name[64] = {0};
    off_t fileSize = 0;//the total size of the file
    off_t downSize = 0;//the size that have been downloaed
    off_t lastSize = 0;//记录上次打印进度条的大小
    int fd;


    recv(serverFd, &fileSize, sizeof(fileSize), 0);//<gets:1> 
    printf("fileSize = %ld\n", fileSize);
    if(0 == fileSize)
    {
        printf("no such file!\n");
    }

    sscanf(cmd+4,"%s",file_name);

    fd = open(file_name,O_RDWR);//try to open the file,if the file doesn't exist,this function will return -1,
    if(fd == -1)//file does not exist
    {
        downSize= 0;
        send(serverFd, &downSize, sizeof(downSize), 0);//<puts:2:case1>

        fd = open(file_name,O_WRONLY|O_CREAT,0666);//create a new file
    }
    else//the file exist
    {
        struct stat statbuf;
        stat(file_name,&statbuf);//get the size of the file
        downSize = statbuf.st_size;
        send(serverFd, &downSize, sizeof(downSize) ,0);//<puts:2:case1>
        ret = lseek(fd, downSize, SEEK_SET);
    }
    printf("downSize = %ld\n", downSize);


    if (downSize == fileSize) {
        printf("the file has been download!\n");
    }

    //receive the content of the file
    int fds[2];
    pipe(fds);

    struct timeval start, end;
    bzero(&start, sizeof(start));
    bzero(&end, sizeof(end));
    gettimeofday(&start, NULL);

    lastSize = downSize;

    while(downSize < fileSize)
    {
        /* 第一次从接收缓冲区读到管道写端 */
        ret = splice(serverFd, NULL, fds[1], NULL, SPLICESIZE, 0);

        //第二次从管道中读出来，写到文件里
        ret = splice(fds[0], NULL, fd, NULL, ret, 0);
        downSize += ret;

    }

    gettimeofday(&end, NULL);

    printf("download success!\n");
    printf("cost time=%lds\n",(end.tv_sec-start.tv_sec));

    close(serverFd);
    pthread_exit(NULL);
}















