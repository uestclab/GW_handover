#include "ThreadFuncWrapper.h"



void* timeoutInquire(void* args){
	threadpool_data_t* tmp = (threadpool_data_t*)args;
	Manager* pManager = (Manager*)tmp->pTemp;
    sleep(3);
	pManager->post_msg(MSG_INIT_DISTANCE_CHECK_TIMEOUT,NULL,0, tmp->bs_id, tmp->ve_id);
}

void postTimeOutWorkToThreadPool(int ve_id, Manager* pManager, ThreadPool* g_threadpool){
	threadpool_data_t* tmp = (threadpool_data_t*)malloc(sizeof(threadpool_data_t));
	tmp->bs_id = -1;
	tmp->ve_id = ve_id;
	tmp->pTemp = pManager;
	AddWorker(timeoutInquire,(void*)tmp,g_threadpool);
}
