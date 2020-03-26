#include "ThreadPool.h"
#include <stdlib.h>

static void * WorkProcess(void *arg);
void JoinFollower(ThreadPool* g_threadpool);
void PromoteNewLeader(ThreadPool* g_threadpool);

void createThreadPool(uint32_t nQueueSize,uint32_t nThreadNum, ThreadPool** g_threadpool_p){
	*g_threadpool_p = (ThreadPool*)malloc(sizeof(struct ThreadPool));
	ThreadPool* g_threadpool = *g_threadpool_p;
	SimpleQueue(nQueueSize, &(g_threadpool->m_oQueue)); 
	g_threadpool->m_oLeaderID = NO_CURRENT_LEADER;
	g_threadpool->m_nThreadNum=nThreadNum;
	g_threadpool->m_bQueueClose=0;
	g_threadpool->m_bPoolClose=0;
	g_threadpool->m_nMaxTaskNum=MaxSize(g_threadpool->m_oQueue);
	pthread_cond_init(&(g_threadpool->m_pQueueNotEmpty),NULL);
	pthread_cond_init(&(g_threadpool->m_pQueueNotFull),NULL);
	pthread_cond_init(&(g_threadpool->m_pQueueEmpty),NULL);
	pthread_cond_init(&(g_threadpool->m_pNoLeader),NULL);
	pthread_mutex_init(&(g_threadpool->m_pLeaderMutex),NULL);
	pthread_mutex_init(&(g_threadpool->m_pQueueHeadMutex),NULL);
	pthread_mutex_init(&(g_threadpool->m_pQueueTailMutex),NULL);
	pthread_attr_t attr;
	pthread_attr_init (&attr);
	pthread_attr_setdetachstate (&attr, PTHREAD_CREATE_DETACHED);
	g_threadpool->m_vThreadID=(pthread_t *)malloc(sizeof(pthread_t)*g_threadpool->m_nThreadNum);
	for(size_t i=0;i<nThreadNum;++i){
		pthread_create(&(g_threadpool->m_vThreadID[i]),&attr,WorkProcess,(void*)g_threadpool);
	}
}

void destoryThreadPool(ThreadPool* g_threadpool)
{
	Destroy(g_threadpool);
	pthread_cond_destroy(&(g_threadpool->m_pQueueNotEmpty));
	pthread_cond_destroy(&(g_threadpool->m_pQueueNotFull));
	pthread_cond_destroy(&(g_threadpool->m_pQueueEmpty));
	pthread_cond_destroy(&(g_threadpool->m_pNoLeader));
	pthread_mutex_destroy(&(g_threadpool->m_pQueueHeadMutex));
	pthread_mutex_destroy(&(g_threadpool->m_pQueueTailMutex));
	pthread_mutex_destroy(&(g_threadpool->m_pLeaderMutex));
}

void Destroy(ThreadPool* g_threadpool)
{
	if (g_threadpool->m_bPoolClose) 
		return;
	//关闭队列，不在接受新的任务
	g_threadpool->m_bQueueClose=1;

	pthread_mutex_lock(&(g_threadpool->m_pQueueTailMutex));
	while (Size(g_threadpool->m_oQueue)!=0){
		pthread_cond_wait(&(g_threadpool->m_pQueueEmpty), &(g_threadpool->m_pQueueTailMutex));
	}
	g_threadpool->m_bPoolClose=1;
	pthread_mutex_unlock(&(g_threadpool->m_pQueueTailMutex));
	//唤醒所有线程，准备退出
	pthread_cond_broadcast(&(g_threadpool->m_pNoLeader));
	pthread_cond_broadcast(&(g_threadpool->m_pQueueNotEmpty));

	//delete [] m_vThreadID;
	free(g_threadpool->m_vThreadID);
}

int AddWorker(void *(*process)(void *arg),void* arg, ThreadPool* g_threadpool)
{
	if(g_threadpool->m_bQueueClose)
		return 0;
	//Job *pNewJob=new Job;
	Job *pNewJob = (Job*)malloc(sizeof(Job));
	pNewJob->arg=arg;
	pNewJob->process=process;

	pthread_mutex_lock(&(g_threadpool->m_pQueueHeadMutex));

	while(Size(g_threadpool->m_oQueue)>=g_threadpool->m_nMaxTaskNum&&!g_threadpool->m_bQueueClose){
		pthread_cond_wait(&(g_threadpool->m_pQueueNotFull), &(g_threadpool->m_pQueueHeadMutex));
	}

	if(g_threadpool->m_bQueueClose){
		//delete pNewJob;
		free(pNewJob);
		pthread_mutex_unlock(&(g_threadpool->m_pQueueHeadMutex));
		return 0;
	}
	Put(pNewJob,g_threadpool->m_oQueue);
	pthread_mutex_unlock(&(g_threadpool->m_pQueueHeadMutex));
	pthread_cond_signal(&(g_threadpool->m_pQueueNotEmpty));
	return 1;
}
 
void * WorkProcess(void *arg)
{

	ThreadPool *pThreadPool=(ThreadPool*)arg;
	JoinFollower(pThreadPool);
	while(1)
	{
		pthread_mutex_lock(&(pThreadPool->m_pQueueTailMutex));
		while(Size(pThreadPool->m_oQueue)==0&&!pThreadPool->m_bPoolClose){
			pthread_cond_wait(&(pThreadPool->m_pQueueNotEmpty),&(pThreadPool->m_pQueueTailMutex));
		}
		pthread_mutex_unlock(&(pThreadPool->m_pQueueTailMutex));
		if(pThreadPool->m_bPoolClose){
			pthread_exit(NULL);
		}

		Job *pJob;

		//pThreadPool->m_oQueue.Fetch(pJob);
		Fetch(&pJob, pThreadPool->m_oQueue);

		if(pThreadPool->m_bQueueClose&&Size(pThreadPool->m_oQueue)==0){
			pthread_cond_signal(&(pThreadPool->m_pQueueEmpty));
		}
		pthread_cond_signal(&(pThreadPool->m_pQueueNotFull));

		//pThreadPool->PromoteNewLeader();
		PromoteNewLeader(pThreadPool);
		pJob->process(pJob->arg);
		//delete pJob;    
		free(pJob);
		JoinFollower(pThreadPool);
	}
	return NULL;
}

void JoinFollower(ThreadPool* g_threadpool)
{
	pthread_mutex_lock(&(g_threadpool->m_pLeaderMutex));
	while(g_threadpool->m_oLeaderID!=NO_CURRENT_LEADER&&!g_threadpool->m_bPoolClose){
		pthread_cond_wait(&(g_threadpool->m_pNoLeader),&(g_threadpool->m_pLeaderMutex));
	}

	if(g_threadpool->m_bPoolClose){
		pthread_mutex_unlock(&(g_threadpool->m_pLeaderMutex));
		pthread_exit(NULL);
	}
	g_threadpool->m_oLeaderID=pthread_self();

	pthread_mutex_unlock(&(g_threadpool->m_pLeaderMutex));  

}

void PromoteNewLeader(ThreadPool* g_threadpool)
{
	g_threadpool->m_oLeaderID=NO_CURRENT_LEADER;
	pthread_cond_signal(&(g_threadpool->m_pNoLeader));
}





