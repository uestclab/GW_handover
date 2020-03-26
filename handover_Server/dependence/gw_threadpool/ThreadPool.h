#ifndef __THREADPOOL_H
#define __THREADPOOL_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <pthread.h>
#include <stdio.h>
#include "SimpleQueue.h"
#define NO_CURRENT_LEADER 0


typedef struct ThreadPool{
	simple_queue*   m_oQueue;
	pthread_cond_t  m_pQueueNotEmpty;
	pthread_cond_t  m_pQueueNotFull;
	pthread_cond_t  m_pQueueEmpty;
	pthread_cond_t  m_pNoLeader;
	pthread_mutex_t m_pLeaderMutex;
	pthread_mutex_t m_pQueueHeadMutex;
	pthread_mutex_t m_pQueueTailMutex;
	int m_bQueueClose;
	int m_bPoolClose;
	pthread_t *m_vThreadID;
	pthread_t m_oLeaderID;
	uint32_t m_nThreadNum;
	uint32_t m_nMaxTaskNum;
}ThreadPool;

void createThreadPool(uint32_t nQueueSize, uint32_t nThreadNum, ThreadPool** g_threadpool); // uint32_t nQueueSize=4096,uint32_t nThreadNum=8
void destoryThreadPool(ThreadPool* g_threadpool);
int AddWorker(void *(*process)(void *arg),void* arg, ThreadPool* g_threadpool);
void Destroy(ThreadPool* g_threadpool);

#ifdef __cplusplus
}
#endif

#endif//__THREADPOOL_H
