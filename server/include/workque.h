//队列使用的结构体及相关操作
# ifndef __WORKQUE_H__
# define __WORKQUE_H__

# include "head.h"
typedef struct queNode {
    int clientFd;
    struct queNode * pNext;
    int pos;//该结点在循环队列中的位置
}queNode_t, *pQueNode_t;

typedef struct que {
    int size;
    pQueNode_t phead, ptail;
    pthread_cond_t cond;
    pthread_mutex_t que_mutex;
    int overFlag;//当overFlag==1时表示开始执行退出操作
}que_t, *pque_t;

int queInit(pque_t pq);
int queInsert(pque_t pq, queNode_t * pNew);
int queGet(pque_t pq, pQueNode_t * pGet);

extern pQueNode_t *circular_que;
extern int curPos;           

# endif


