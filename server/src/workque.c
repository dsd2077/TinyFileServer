// # include "../include/head.h"
// # include "../include/workque.h"
# include "../include/threadpool.h"

int queInit(pque_t pq)
{
    pq->size = 0;
    pq->phead = NULL;
    pq->ptail = NULL;
    pq->overFlag = 0;
    pthread_cond_init(&pq->cond, NULL);
    pthread_mutex_init(&pq->que_mutex, NULL);

    return 0;
}

//在调用queInsert()之前，已经加锁
int queInsert(pque_t pq, pQueNode_t pNew)
{
    if (NULL == pq->phead) {
        pq->phead = pNew;
        pq->ptail = pNew;
    }
    else {
        pq->ptail->pNext = pNew;
        pq->ptail = pNew;
    }
    ++pq->size;

    return 0;
    
}

//在调用queGet()之前，已经加锁，所以在queGet()内部不需要再加锁
int queGet(pque_t pq, pQueNode_t * pGet)
{
    if (0 == pq->size) {
        return -1;
    }
    *pGet = pq->phead;
    pq->phead = pq->phead->pNext;
    --pq->size;
    
    //only one node 
    if (0 == pq->size) {
        pq->ptail = NULL;
    }
    
    return 0;
}


