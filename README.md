## 快速运行

\- 测试环境

  \- Ubuntu 22.04.2 LTS

  \- MYSQL 8.0.33



### 服务端启动

- 导入数据库表

~~~shell
cd TinyFileServer

mysql -u username -p password	   //登录mysql

create database yourdb;          //创建数据库  

USE yourdb;

SOURCE database/test.sql         // 导入数据库表

SHOW tables;                     //显示以下内容则导入成功  
~~~

+----------------------+

| VirtualFileTable   |

| log          |

| token         |

| user         |

+----------------------+



- 修改Mysql.c中数据库初始化信息

~~~c
const char* server=  "localhost";

const char* user=   "username";

const char* password="password";

const char* database="yourdb";
~~~

- build

~~~shell
cd server/src

make
~~~



- 启动

~~~
./server [-p port] [-t threadNum]
~~~

-p，自定义端口号

  默认9006

-t，线程数量

  默认为8



### 客户端启动

- 修改配置文件client/src/config.conf

~~~shell
# 服务器的Ip和port
ip = 127.0.0.1   
port = 10001
~~~





项目采用C/S模型和线程池模型，使用tcp协议进行消息传递。



业务1：注册——将账户和密文密码存放在用户表

  业务逻辑：

  客户端输入用户名，发送给服务器

  服务器收到用户名，生成salt，将两者插入数据库，并将salt发送给客户

  客户收到salt值后，输入用户密码，用salt和密码生成密文，发送给服务器

  服务器收到密文后，插入进数据库

  服务器为该用户在虚拟文件表中创建一条记录——即该用户的“家目录”

  服务器更新该用户的user表中的pwd项为/home



业务2：登录

  客户端输入用户名，发送给服务器

  服务器收到用户名，从数据库中查找该用户的salt，如果查找成功，则发送给客户端；如果查找失败发送一个代号给客户端（-1）表示用户名错误

  客户收到salt值后，输入用户密码，用salt和密码生成密文，发送给服务器

  服务器收到密文后，与数据库中该用户的密文比较，如果相同，则发送1给客户端代表登录成功，否则发送0，代表登录失败



业务3：cd——仅支持cd  ..和cd 当前目录下的目录

  服务器端接收到cd命令（此时并不确定命令正确与否）查询数据库，获取该用户的pwd

​    判断命令是否为cd ..

​      是则判断pwd是否已到达家目录/home

​        是则发送reach home directory~，

​        否则从pwd中解析出上一级目录，发送给客户端，并更新pwd

​      否则从pwd中解析出当前工作目录（cwd），查找cwd在虚拟文件表中的fileid，通过owner、filename、parent_id、filetype四个字段从数据库中去查找当前工作目录下是否有目标目录

​        存在则更新user表中的pwd，并将pwd发送给客户端

​        不存在则发送no such directory给客户端

​      

业务4：日志功能

  服务器每收到一条命令，便将其插入log表中



业务5：ls

  服务器端接收到ls命令.查询数据库，获取该用户的pwd，从pwd中获取当前工作目录（cwd）,从数据库中查询cwd的fileid（作为parent_id来查询其下的文件），通过owner和parent_id两个字段，查询虚拟文件表，并将结果发送给客户端



业务6：pwd

  服务器端接收到pwd命令.查询数据库，获取pwd发送给客户



业务7mkdir

  服务器端接收到mkdir命令.查询数据库，获取该用户的pwd，从pwd中获取当前工作目录（cwd）,从数据库中查询cwd的fileid（作为新建目录的parent_id），插入一条记录到虚拟文件表，回执消息给客户端

  



业务8 puts 上传文件 &&&秒传功能

  客户端打开文件，如果打开失败，说明命令出错，目标文件不存在，向服务器发送fileSize=0；打开成功则向服务器发送文件大小fileSize。计算文件的md5码将其发送给服务器

  服务器收到fileSize，选择分支。fileSize=0，此次业务结束；fileSize>0，接收文件的md5码，根据文件的md5码查询虚拟文件表，查看该文件是否存在。不存在给客户端发送1，并以md5码为文件名创建该文件，准备接收数据；传送结束后，在虚拟文件表中插入一条记录；如果存在（该用户可能是文件主，此时不允许重复上传）根据文件的md5码查看文件主，如果就是该用户，则不允许重复上传，给客户端回复3，否则给客户端回复2，并在虚拟文件表中为该用户插入一条关于该文件的记录，并将虚拟文件表中所有md5相同的记录的count加一（为了实现删除操作）



  

业务9 gets 下载文件 &&&断点重传&&&零拷贝技术

  服务器端接收到gets命令.查询数据库，获取该用户的pwd，从pwd中获取当前工作目录（cwd）,从数据库中查询cwd的fileid。通过owner、parent_id、filename三个字段从虚拟文件表中查找目标文件的md5码 

​    如果不存在，给客户端发送fileSize=0

​    如果存在，获取该文件的大小fileSize，给客户端发送文件总大小

  客户端收到回执消息.如果服务器端有该文件——>判断本地是否有该文件。

​    如果不存在，则向服务端发送downSize=0，并创建该文件，准备下载

​    如果该文件存在，向服务端发送文件大小downSize。并判断downSize是否等于fileSize，如果相等，则跳出此次循环；否则将文件偏移downSize个字节，以此实现断点续传（注意：如果采用splice则不能以append模式打开文件）

  服务器端收到客户端发来的downSize，同样判断downSize是否等于fileSize，如果相等，跳出此次循环；不相等则开始下载，用sendfile零拷贝技术

  客户端开始接收文件内容



业务10 rm  删除文件

  服务端：第一步查找是否有该文件，

​      第二步查找该文件的count值

​        如果大于1，在虚拟文件表中将该用户关于该文件的记录删除，并将虚拟文件表中所有md5值一样的文件的count减一

​        如果等于1，根据文件的MD5码去filepool中删除文件，并在虚拟文件表中将该用户关于该文件的记录删除