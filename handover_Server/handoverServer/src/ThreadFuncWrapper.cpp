#include "ThreadFuncWrapper.h"

void* timeoutInquire(void* args){
	Manager* pManager = (Manager*)args;
    sleep(1);
	pManager->post_msg(MSG_TIMEOUT,NULL,0);
}

void postTimeOutWorkToThreadPool(Manager* pManager, ThreadPool* g_threadpool){
	AddWorker(timeoutInquire,(void*)pManager,g_threadpool);
}