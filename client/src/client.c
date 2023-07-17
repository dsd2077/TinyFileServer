//TODO:keep the server nomal once client quit abruptly
#define _GNU_SOURCE	   /* See feature_test_macros(7) */
#include "../include/multi_process.h"
#define SPLICESIZE 8192

char msg[1] = {0};
char token[1024];
struct sockaddr_in ser_addr;

int main(int argc,char *argv[])
{
    char ip[64];
    int port;
    read_config(ip, &port);
    //初始化一个socket描述符，用于tcp通信
    int serverFd;
    serverFd=socket(AF_INET,SOCK_STREAM,0);
    printf("serverFd = %d\n", serverFd);
    ERROR_CHECK(serverFd,-1,"socket");

    bzero(&ser_addr,sizeof(ser_addr));
    ser_addr.sin_family=AF_INET;
    ser_addr.sin_port=htons(port);
    ser_addr.sin_addr.s_addr=inet_addr(ip);

    int ret=connect(serverFd,(struct sockaddr*)&ser_addr,sizeof(ser_addr));
    ERROR_CHECK(ret,-1,"connect");
    send(serverFd, "1", 1, 0);//'1'表示第一次连接               ///为什么需要这一步？

    char username[16] = {0};
    char * password;
    char *encryptedpd = NULL;//encrypted password
    char salt[16] = {0};
    char recv_buf[1024];
    char input[1024] = {0};
    char message[1024] = {0};

    printf("0  for log in\n"); 
    printf("1  for register\n");
    scanf("%s",input);

    if (input[0] == '1') {
        // 如果注册失败怎么接受消息？
        printf("begain register\n");
        printf("please input username:");
        scanf("%s",input);
        sprintf(message, "signin %s",  input);
        send(serverFd,message,strlen(message),0);

        recv(serverFd, salt, sizeof(salt),0);           //这里不一定接受到盐值，有可能注册失败

        password=getpass("please input password:");
        encryptedpd = crypt(password,salt);

        send(serverFd, encryptedpd, strlen(encryptedpd), 0);
        printf("register success!\n");
    }

    while(1)
    {
        printf("please input username:");
        scanf("%s",username);
        send(serverFd,username,strlen(username),0);//<login:1>

        recv(serverFd,salt,sizeof(salt),0);//<login:2>
        if (strncmp(salt,"-1",1) == 0) {
            printf("incorrect username,please try again\n");
            continue;
        }

        password=getpass("please input password:");
        encryptedpd = crypt(password,salt);

        send(serverFd,encryptedpd,strlen(encryptedpd),0);//<login:3>

        recv(serverFd,msg,1,0);//<login:4>
        if(msg[0] == '1')
        {
            printf("login success!\n");
            bzero(token, sizeof(token));
            recv(serverFd, token, sizeof(token), 0);
            /* printf("token = %s\n",token); */
            break;
        }
        printf("incorrect password,please try again!\n");
    }

    char cmd[128] = {0};
    while (1)
    {   
        bzero(cmd, sizeof(cmd));
        read(0, cmd, 128);//read()读取时不会丢掉换行
        ret = send(serverFd,cmd,strlen(cmd)-1,0);//发送命令
        ERROR_CHECK(ret, -1, "send");
        if (0 == ret) {
            printf("服务器已超时断开，请重新连接\n");
            exit(-1);
        }

        bzero(recv_buf,1024);
        if(strncmp("cd", cmd, 2) == 0)
        {
            /* system("clear"); */
            recv(serverFd,recv_buf,sizeof(recv_buf),0);
            printf("%s\n", recv_buf);
        }
        else if(strncmp("ls", cmd, 2) == 0)
        {
            /* system("clear"); */
            recv(serverFd, recv_buf, sizeof(recv_buf), 0);
            printf("%s\n",recv_buf);
        }
        else if (strncmp("mkdir", cmd, 5) == 0)
        {
            recv(serverFd,recv_buf,sizeof(recv_buf),0);
            printf("%s\n",recv_buf);
        }
        else if (strncmp("rm", cmd, 2) == 0)
        {
            recv(serverFd, recv_buf, sizeof(recv_buf), 0);
            printf("%s\n",recv_buf);
        }
        else if(strncmp("pwd", cmd, 3) == 0)
        {
            /* system("clear"); */
            bzero(recv_buf,sizeof(recv_buf));
            recv(serverFd,recv_buf,sizeof(recv_buf),0);
            printf("%s\n", recv_buf);
        }
        else if(strncmp("puts", cmd, 4) == 0)
        {
            char cmd2[1024] = {0};
            strcpy(cmd2, cmd);
            pthread_t pthid;
            pthread_create(&pthid, NULL, threadFunc1, cmd2);
            printf("执行长命令分离\n\n");
            continue;
        }
        else if( strncmp("gets", cmd, 4)== 0 )
        {
            char cmd3[1024] = {0};
            strcpy(cmd3, cmd);
            pthread_t pthid;
            pthread_create(&pthid, NULL, threadFunc2, cmd3);
            printf("执行长命令分离\n\n");
            continue;
        }
        else if( strncmp("help", cmd, 4) == 0 )
        {
            printf("-----------------------------------------------------------------\n");
            printf("1.cd dir/..   enter specified directory or superior directory\n");
            printf("2.ls          show files and directories on current directory\n");
            printf("5.pwd         print current working path\n");
            printf("3.puts file   upload a file ,not allow upload a directory\n");
            printf("4.gets file   download a file ,not allow download a directory\n");
            printf("6.mkdir dir   creat a new directory\n");
            printf("7.other command will not respond\n");
            printf("-----------------------------------------------------------------\n");
        }
        else 
        {
            printf("can't find the command!\n");
            printf("please type 'help' for help\n");
            continue ;
        }
        printf("------------------------\n\n");
    }
}

int recvn(int serverFd,void* pstart,int len)
{
    int total=0,ret;
    char *p=(char*)pstart;
    while(total<len)
    {
        ret=recv(serverFd,p+total,len-total,0);
        total+=ret;//每次接收到的字节数加到total上
    }
    return 0;
}

int sendn(int fd_send, char* send_buf, int len)
{
    int sum = 0 ;
    int nsend ;
    while(sum < len)
    {
        nsend = send(fd_send, send_buf + sum, len - sum, 0);
        sum += nsend ;

    }
    return sum ;
}

int map_recv(int serverFd,void* pstart,int len)
{
    int total=0,ret;
    char *p=(char*)pstart;
    int fd = open("log",O_RDWR);
    while(total<len)
    {
        ret=recv(serverFd,p+total,len-total,0);
        total+=ret;//每次接收到的字节数加到total上
        write(fd,&total,sizeof(total));
        lseek(fd,0,SEEK_SET);
    }
    return 0;
}
