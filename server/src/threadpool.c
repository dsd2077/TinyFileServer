# include "../include/head.h"
# include "../include/workque.h"
# include "../include/threadpool.h"


pQueNode_t *circular_que;
int curPos;       


int threadPoolInit(pThreadPool_t pPool, int threadNum)
{
    pPool->threadNum = threadNum;
    pPool->pthid = (pthread_t *)calloc(threadNum, sizeof(pthread_t));//TODO:free
    queInit(&pPool->que);

    return 0;
}

//线程处理函数的格式是固定的：void *(*start_routine) (void *)
void * threadFunc(void *p)
{
    pque_t pq = (pque_t)p;
    pQueNode_t pGet = NULL;//用来存储从队列中获取到的结点

    while (1) {
        pthread_mutex_lock(&pq->que_mutex);
        if (0 == pq->size) {
            pthread_cond_wait(&pq->cond,&pq->que_mutex);
        }
        queGet(pq, &pGet);
        pthread_mutex_unlock(&pq->que_mutex);

        //取到结点
        if (pGet) {
            /* transFile(pGet->clientFd);//TODO:将其改为childHandle */
            child_handle(pGet);
        }

        //收到退出信号，准备退出
        if (1 == pq->overFlag) {
            pthread_exit(NULL);
        }
    }                                                 
}

int threadPoolStart(pThreadPool_t pPool)
{
    //create child thread 
    for (int i = 0; i < pPool->threadNum; ++i) {
        pthread_create(&pPool->pthid[i], NULL, threadFunc, &pPool->que);
    }

    return 0;
}


