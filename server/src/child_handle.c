//子进程处理函数：用来完成客户端发送过来的任务
# include "../include/head.h"
# include "../include/workque.h"
# include "../include/threadpool.h"

char username[16] = {0};


int signin(int clientFd) {
    char salt[128] = {0};
    char query[1024] = {0};//mysql statement buf
    char password[128] = {0};
    printf("begain registration\n");

    int ret = recv(clientFd,username,sizeof(username),0);//<register:1>

    generate_salt(salt);

    sprintf(query, "insert into user(username, salt) values('%s', '%s')", username, salt);
    insert(query);

    send(clientFd, salt, strlen(salt), 0);//<register:2>

    recv(clientFd, password, sizeof(password), 0);//<register:3>

    sprintf(query, "update user set password= '%s' where username = '%s'",password,username);
    update(query);

    //create a home directory
    sprintf(query, "insert into VirtualFileTable(parent_id,filename,filetype,owner) values(0,'home','d','%s')", username);
    insert(query);

    sprintf(query,"%s%s%s%s%s","update user set pwd = ","'/home'"," where username = '",username,"'");
    update(query);
    return 0;
}

int login(int clientFd){
    char query[1024] = {0};//mysql statement buf
    char password[128] = {0};
    char retval[1024] = {0};//mysql return value buf
    char token[128] = {0};

    while (1) {
        bzero(username,sizeof(username));
        int ret = recv(clientFd,username,sizeof(username),0);//<login:1>

        bzero(query,sizeof(query));
        bzero(retval,sizeof(retval));
        sprintf(query,"%s%s%s","select salt,password from user where username = '",username,"'");
        ret =Query(query,retval);
        if(-1 == ret) {
            send(clientFd,"-1",1,0);//<login:2>"-1"表示登陆失败
            continue;
        }
        else {
            send(clientFd,retval,11,0);//<login:2>
        }
        recv(clientFd,password,sizeof(password),0);//<login:3>

        if (strncmp(retval+12,password,strlen(password)) == 0) {
            send(clientFd,"1",1,0);//<login:4>send 1 on behalf of succuss

            //生成token值
            generate_salt(token);
            //先尝试插入，如果插入失败则选择更新
            sprintf(query, "insert into token(user, token) values('%s', '%s')", username, token);
            ret = insert(query);
            if (-1 == ret) {
                sprintf(query, "update token set token = '%s' where user = '%s'",token, username);
                ret = update(query);
            }

            bzero(retval, sizeof(retval));
            sprintf(retval,"%s %s", username, token);

            send(clientFd, retval, sizeof(retval), 0);
            break;
        } else {
            send(clientFd,"0",1,0);//<login:4>send 0 on behalf of failure
        }
    }
}

int child_handle(pQueNode_t pNew)
{
    int clientFd = pNew->clientFd;
    printf("clientFd = %d\n", clientFd);
    int pos = pNew->pos;//当前clientfd在循环队列中的位置
    char cmd[1024] = {0};//command buffer
    int ret = 0;
    char password[128] = {0};
    char salt[128] = {0};
    char token[128] = {0};
    char msg[1] = {0};//message buf to communicate with client
    char query[1024] = {0};//mysql statement buf
    char retval[1024] = {0};//mysql return value buf

    printf("receive task mission start!\n");

    ret = recv(clientFd, msg, sizeof(msg), 0);
    printf("msg = %s\n", msg);

    //注册
    if ('1' == msg[0]) {
        ret = signin(clientFd);
        login(clientFd);
    }
    //登陆
    else if ('0' == msg[0])
    {
        login(clientFd);
    }
    //上传下载文件的二次登陆
    else {
        printf("验证二次登陆\n");
        char buf[1024]  = {0};
        bzero(username, sizeof(username));
        recv(clientFd, buf, sizeof(buf), 0);

        int i;
        for (i = 0; buf[i] != ' '; ++i) {
            username[i] = buf[i];
        }

        strcpy(token, buf+i+1);
        sprintf(query,"select token from token where user = '%s'", username);
        Query(query, retval);
        if (0 == strcmp(retval, token)) {
            send(clientFd, "1", 1, 0);//给客户端发送1，代表认证成功
        }
        else {
            send(clientFd, "0", 1, 0);//给客户端发送0，代表认证失败
            printf("二次登陆认证失败\n");
            return -1;
        }
    }

    while(1)
    {
        /* bzero(cmd,sizeof(cmd)); */
        memset(cmd, 0, sizeof(cmd));
        ret = recv(clientFd, cmd, sizeof(cmd), 0);//接收命令本身
        printf("clientFd = %d, cmd = %s\n",clientFd, cmd);
        if (-1 == ret || 0 == ret) {
            return -1;
        }

        if ( '2' != msg[0]) {
            /* printf("before move pos = %d\n",pos); */
            //找到当前clientFd在循环队列中的结点
            pQueNode_t ptemp = circular_que[pos];
            //将该结点挪动到curPos位置上
            if (ptemp ) {
                circular_que[pos] = NULL;
                ptemp->pNext = circular_que[curPos];
                circular_que[curPos] = ptemp;
                pos = curPos;
            }
            /* printf("after move pos = %d\n",pos); */
        }

        //user's operation is been logged
        memset(query, 0, sizeof(query));
        snprintf(query, sizeof(query), "insert into log(user, operation) values('%s', '%s')", username, cmd);
        insert(query);

        if(strncmp("cd", cmd, 2) == 0)
        {
            do_cd(clientFd,cmd);
        }
        else if(strncmp("ls", cmd, 2) == 0)
        {
            do_ls(clientFd);
        }
        else if( strncmp("puts", cmd, 4)== 0 && '2' == msg[0])
        {
            do_puts(clientFd,cmd);
            break;
        }
        else if( strncmp("gets", cmd, 4)== 0 && '2' == msg[0] )
        {
            do_gets(clientFd,cmd);
            break;
        }
        else if(strncmp("pwd", cmd, 3) == 0) 
        {
            do_pwd(clientFd,username);
        }
        else if (strncmp("mkdir", cmd, 5) == 0 )
        {
            do_mkdir(clientFd,cmd);
        }
        else if (strncmp("rm", cmd, 2) == 0)
        {
            do_remove(clientFd, cmd);
        }
        else 
        {
            continue ;
        }
    }
    printf("mission conplete\n");
    close(clientFd);//disconnecting with client 

    return 0;
}
int do_cd(int clientFd,char *cmd) 
{
    char query[1024] = {0};
    char retval[1024] = {0};
    int file_id;
    int i,len;
    char dir_name[16] = {0};
    char pwd[128] = {0};
    int ret;

    //get the target directory name from command
    sscanf(cmd+3,"%s",dir_name);

    //fetch "pwd" from user Table
    sprintf(query,"%s%s%s","select pwd from user where username = '",username,"'");
    Query(query,pwd);

    if(strcmp(dir_name,"..") == 0) {//the target directory is superior directory

        if(strcmp(pwd,"/home") == 0) {//reach the user's home directory
            memcpy(retval,"reach home directory~",21);
            send(clientFd,retval,strlen(retval),0);//<cd:case1>

        } else {//didn't reach the user's home directory
            len = strlen(pwd);
            for(i=len-1 ;pwd[i]!='/';i--);
            //strncpy(pwd,pwd,i);//check out what will happen:strncpy will not truncate the remaining data
            //memcpy(pwd,pwd,i);//memcpy also doesn't work
            bzero(pwd+i,sizeof(pwd)-i);

            sprintf(query,"%s%s%s%s'","update user set pwd = '",pwd,"' where username = '",username);
            update(query);
            send(clientFd,pwd,strlen(pwd),0);//<cd:case2>
        }

    } 
    else {
        file_id = getCwdFileId();

        //Query whether the target file exists
        bzero(retval,sizeof(retval));
        sprintf(query,"select id from VirtualFileTable where owner = '%s' and filename = '%s' and parent_id = %d and filetype = 'd'",username,dir_name,file_id);
        ret = Query(query,retval);

        if(-1 == ret) {//the directory doesn's exist
            memcpy(retval,"no such directory",17);
            send(clientFd,retval,strlen(retval),0);//<cd:case3>

        } else {//the directory exist
            snprintf(pwd,sizeof(pwd), "%s/%s",pwd,dir_name);
            sprintf(query,"%s%s%s%s'","update user set pwd = '",pwd,"' where username = '",username);
            update(query);
            send(clientFd,pwd,strlen(pwd),0);//<cd:case4>
        }
    }
    return 0;
}
int do_ls(int clientFd) 
{
    char query[1024] = {0};
    char retval[1024] = {0};
    int parentId;
    int ret;

    parentId = getCwdFileId();

    //query Virtual File Table according to file_id(parent_id) and username
    bzero(retval,sizeof(retval));
    sprintf(query,"select filename, filetype, owner,filesize from VirtualFileTable where owner = '%s' and parent_id = %d",username,parentId);
    ret = Query(query,retval);

    if(-1 == ret) {
        memcpy(retval,"the directory is empty",22);
    }
    send(clientFd,retval,strlen(retval),0);
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

int do_pwd(int clientFd,char *username)
{
    char query[1024] = {0};
    char retval[1024] = {0};

    sprintf(query,"%s%s%s","select pwd from user where username = '",username,"'");
    Query(query,retval);

    send(clientFd,retval,strlen(retval),0);

    return 0;
}

int do_remove(int clientFd,char *cmd) 
{
    int ret;
    char file_name[128]={0};
    char query[1024] = {0};
    int file_id = 0;
    int count = 0;
    char retval[1024] = {0};
    char md5[64] = {0};

    ret = chdir("../filepool");
    ERROR_CHECK(ret, -1, "chdir");

    sscanf(cmd+3, "%s", file_name);
    printf("%s\n",file_name);

    file_id = getCwdFileId();


    sprintf(query,"select md5sum from VirtualFileTable where owner = '%s' and parent_id = %d and filename = '%s'",username,file_id,file_name);
    ret = Query(query,md5);

    //the file doesn's exist
    if(ret == -1)
    {
        bzero(retval, sizeof(retval));
        strcpy(retval, "no such file! ");
    }
    //the file exist
    else {
        sprintf(query,"select count from VirtualFileTable where owner = '%s' and parent_id = %d and filename = '%s'",username,file_id,file_name);
        ret = Query(query, retval);
        count = atoi(retval);

        //delete the record of file from VirtualFileTable 
        bzero(query, sizeof(query));
        sprintf(query,"delete from VirtualFileTable where parent_id = %d and md5sum = '%s' and owner = '%s'", file_id, md5, username);
        Delete(query);

        if (1 == count) {
            ret = unlink(md5);
        }
        else {
            count -= 1;
            bzero(query, sizeof(query));
            sprintf(query, "update VirtualFileTable set count  = %d where md5sum = '%s'", count, md5);
            update(query);
        }

        strcpy(retval, "remove success!");
    }

    send(clientFd, retval, strlen(retval), 0);
    return 0;
}
int do_gets(int clientFd,char *cmd)
{
    char file_name[64] = {0};
    off_t fileSize = 0;//the total size of the file
    off_t downSize = 0;//the downloaded size of the file
    int ret = 0;
    char query[1024] = {0};
    int file_id;
    char md5[128] = {0};

    chdir("../filepool");

    file_id = getCwdFileId();

    sscanf(cmd+4, "%s", file_name);

    sprintf(query,"select md5sum from VirtualFileTable where owner = '%s' and parent_id = %d and filename = '%s'",username,file_id,file_name);
    ret = Query(query,md5);

    if(-1 == ret) {
        fileSize = 0;
        ret = send(clientFd, &fileSize, sizeof(fileSize), 0);//<gets:1:case1>
        return 0;
    } else {
        int fd = open(md5,O_RDONLY);

        struct stat statbuf;
        stat(md5, &statbuf);//get the size of the file
        fileSize = statbuf.st_size;

        send(clientFd, &fileSize, sizeof(fileSize), 0);//<gets:1:case2>

        //receive the qualification command to testify wheather there is a identical file
        recv(clientFd, &downSize, sizeof(downSize), 0);//<gets:2>

        if (downSize == fileSize) {
            return 0;
        }

        lseek(fd,+downSize,SEEK_SET);

        //send the content of the file
        ret = sendfile(clientFd, fd, NULL, fileSize-downSize);    
        ERROR_CHECK(ret,-1,"sendfile");

    }
    return 0;
}
int do_puts(int clientFd,char *cmd)
{
    int data_len;
    char buf[1024]={0};
    char file_name[64] = {0};
    off_t file_len;
    char md5sum[64] = {0};
    char retval[1024] = {0};
    char query[1024] = {0};
    int ret = 0;
    int file_id;
    char msg[1] = {0};

    chdir("../filepool");

    sscanf(cmd+4,"%s",file_name);

    recv(clientFd, &file_len, sizeof(file_len), 0);//<puts:1>接收命令回执消息，如果命令合法，受到文件大小，否则受到0
    printf("111111\n");
    if(file_len == 0)//命令不合法，接收到客户端发过来的0，直接返回
    {
        return 0;
    }
    recv(clientFd,md5sum,sizeof(md5sum),0);//<puts:2>
    printf("222222\n");

    file_id = getCwdFileId();

    bzero(retval,sizeof(retval));
    sprintf(query,"select owner from VirtualFileTable where md5sum = '%s'",md5sum);
    ret =Query(query,retval);

    if(-1 == ret) {//the file doesn't exist in the file pool
        memcpy(msg,"1",1);
        send(clientFd,msg,1,0);//<puts:3:case1>

        int fd = open(md5sum,O_WRONLY|O_CREAT,0666);
        ERROR_CHECK(fd,-1,"open");

        printf("3333333\n");
        //receive the content of the file
        while(1) 
        {
            recvn(clientFd, &data_len, 4);
            if (data_len > 0) {
                recvn(clientFd, buf, data_len);
                write(fd,buf,data_len);
            } else {
                break;
            }
        }

        printf("444444\n");
        //alter the Virtual file table
        sprintf(query,"insert into VirtualFileTable(parent_id,filetype,filename,owner,filesize,md5sum, count) values(%d, 'f', '%s', '%s', %ld, '%s', 1)", file_id, file_name, username, file_len, md5sum);
        insert(query);
        printf("55555555\n");

    } else {//the file is already exist in the file pool

        bzero(retval,sizeof(retval));
        sprintf(query,"select owner from VirtualFileTable where md5sum = '%s' and parent_id = %d",md5sum,file_id);
        ret =Query(query,retval);

        if(strcmp(retval,username) != 0) {
            memcpy(msg, "2", sizeof(msg));
            send(clientFd, msg, sizeof(msg), 0);//<puts:3:case2>


            int count = 0;
            bzero(retval,sizeof(retval));
            bzero(query, sizeof(query));
            sprintf(query,"select count from VirtualFileTable where md5sum = '%s'", md5sum);
            Query(query,retval);
            count = atoi(retval);

            sprintf(query,"insert into VirtualFileTable(parent_id,filetype,filename,owner,filesize,md5sum, count) values(%d, 'f', '%s', '%s',%ld,'%s', %d)", file_id, file_name, username, file_len,md5sum, count);
            insert(query);

            count += 1;
            bzero(query, sizeof(query));
            sprintf(query, "update VirtualFileTable set count  = %d where md5sum = '%s'", count, md5sum);
            update(query);

        } else {//the user has already uploaded the file
            memcpy(msg, "3", sizeof(msg));
            send(clientFd,retval,1,0);//<puts:3:case3>
        }
    }
    return 0;
}
int recvn(int sfd,void* pstart,int len)
{
    int total=0,ret;
    char *p=(char*)pstart;
    while(total<len)
    {
        ret = recv(sfd, p+total, len-total, 0);
        total+=ret;//每次接收到的字节数加到total上
    }
    return 0;
}

void get_salt(char *salt,char *passwd) 
{ 
    int i,j; 
    for(i=0,j=0;passwd[i] && j != 3;++i) 
    { 
        if(passwd[i] == '$') 
            ++j; 
    }
    strncpy(salt,passwd,i-1); 
}

int generate_salt(char *salt) {
    char str[8] = {0}; 
    int i,flag;
    srand(time(NULL));//通过时间函数设置随机数种子，使得每次运行结果随机。
    for(i = 0; i < 8; i ++) {
        flag = rand()%3;
        switch(flag) {
        case 0:
            str[i] = rand()%26 + 'a';
            break;
        case 1: 
            str[i] = rand()%26 + 'A'; 
            break;
        case 2:
            str[i] = rand()%10 + '0';
            break;
        } 
    }
    sprintf(salt,"%s%s","$6$",str);
    return 0;
}

int do_mkdir(int clientFd,char *cmd)
{
    char query[1024] = {0};
    char retval[1024] = {0};
    int parentId;
    char dir_name[16] = {0};

    //get the directory name from command
    sscanf(cmd+6,"%s",dir_name);

    parentId = getCwdFileId();
    //insert a new directory information into Virtual File Table
    sprintf(query,"insert into VirtualFileTable(parent_id,filename,filetype,owner) values(%d, '%s', 'd', '%s')", parentId, dir_name, username);
    insert(query);

    memcpy(retval,"mkdir success~",13);
    send(clientFd,retval,strlen(retval),0);

    return 0;

}

int getCwdFileId()
{
    char query[1024] = {0};
    char retval[1024] = {0};
    char cur_dir[16] = {0};
    int len, i, file_id;

    //fetch "pwd" from user table
    sprintf(query,"%s%s%s","select pwd from user where username = '",username,"'");
    Query(query,retval);

    //get current working directory from pwd
    len = strlen(retval);
    for(i=len-1 ;retval[i]!='/';i--);
    memcpy(cur_dir,retval+i+1,len-i+1);

    //fetch cwd's id from Virtual File Table which is a string in retval and transfer it to an integer
    bzero(retval,sizeof(retval));
    sprintf(query,"%s%s%s%s'","select id from VirtualFileTable where owner = '",username,"' and filename = '",cur_dir);
    Query(query,retval);
    file_id = atoi(retval);

    return file_id;

}
