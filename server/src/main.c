# include "../include/head.h"
# include "../include/workque.h"
# include "../include/threadpool.h"

threadPool_t threadPool;
int PORT = 10001;                   //端口
int THREADNUM = 8;                  //线程数
const int MAX_EVENT_NUMBER = 10000; //最大事件数


void sigFunc(int sigNum)
{
    threadPool.que.overFlag = 1;

    pthread_cond_broadcast(&threadPool.que.cond);
    
    for (int i = 0; i < threadPool.threadNum; ++i) {
        pthread_join(threadPool.pthid[i], NULL);
    }
    printf("子线程全部退出\n");
    exit(0);
}

int main(int argc, char *argv[])
{
    signal(SIGUSR1, sigFunc);
    parse_arg(argc, argv);


    threadPoolInit(&threadPool, THREADNUM);
    threadPoolStart(&threadPool);

    int listenFd = tcpInit(PORT);

    int epfd = epoll_create(1);
    struct epoll_event events[MAX_EVENT_NUMBER];
    epoll_add(epfd,listenFd);

    //环形队列——用来存储已建立连接的客户端fd，以实现超时断开
    circular_que = (pQueNode_t*)calloc(30, sizeof(pQueNode_t));//设置超时时间为30S,所以循环队列的大小为30
    curPos = 0;

    int curtime;//当前的时间
    int lasttime;//上次返回的时间

    lasttime = time(NULL);
    while (1) {
        int readyNum = epoll_wait(epfd, events, 10, 1000);//超时时间为1S，如果超时则返回0

        //未超时返回,如果并发量很高，不停的有客户连接，仍然需要将curPos加一
        if (readyNum > 0) {
            for (int i = 0; i < readyNum; ++i) {
                //there is a new client
                if (events[i].data.fd == listenFd) {
                    int newFd = accept(listenFd, NULL, NULL);
                    printf("\n\n");
                    printf("new client\n");

                    //客户端发送一个标识指明是第一次连接还是第二次连接
                    char msg[1] = {0};
                    recv(newFd, msg, 1, 0);

                    //将客户端的fd插入任务队列
                    pQueNode_t pNew= (pQueNode_t)calloc(1, sizeof(queNode_t));//TODO:pNew的free
                    pNew->clientFd = newFd;
                    pNew->pos = curPos;
                    pthread_mutex_lock(&threadPool.que.que_mutex);
                    queInsert(&threadPool.que, pNew);
                    pthread_mutex_unlock(&threadPool.que.que_mutex);

                    //唤醒子线程
                    pthread_cond_signal(&threadPool.que.cond);
                    printf("客户端连接，curPos = %d\n", curPos);


                    //只将第一次登陆的客户端fd添加进循环队列(puts/gets的连接不加入)
                    if ('1' == msg[0]) {
                        pQueNode_t pNew2= (pQueNode_t)calloc(1, sizeof(queNode_t));
                        pNew2->clientFd = newFd;
                        pNew2->pos = curPos;
                        pNew2->pNext = circular_que[curPos];
                        circular_que[curPos] = pNew2;
                    }

                    curtime = time(NULL);
                    if (curtime - lasttime > 1) {
                        curPos = (curPos + 1) % 30;
                        lasttime = curtime;
                    }
                }
            }//end of for
        }
        //超时返回
        else {
            curPos = (curPos + 1) % 30;
            pQueNode_t ptemp = circular_que[curPos];
            while (ptemp) {
                printf("closing clientFd = %d\n",ptemp->clientFd);
                int ret = close(ptemp->clientFd);
                if (0 == ret) {
                    printf("超时断开\n");
                }
                circular_que[curPos] = ptemp->pNext;
                ptemp = circular_que[curPos];
            }
        }
    }//end of while
}
