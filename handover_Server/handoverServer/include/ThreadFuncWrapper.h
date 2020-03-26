#include "ThreadPool.h"
#include "EventHandlerQueue.h"
#include "manager.h"

typedef struct threadpool_data_t{
    void* pTemp;
    int bs_id;
    int ve_id;
}threadpool_data_t;

void postTimeOutWorkToThreadPool(int ve_id, Manager* pManager, ThreadPool* g_threadpool);
