#ifndef SIMPLEQUEUE_H
#define SIMPLEQUEUE_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

typedef struct Job
{
	void *arg;
	void *(*process)(void *arg);
}Job;

typedef struct simple_queue{
	volatile uint32_t m_nInPos;   
	volatile uint32_t m_nOutPos;
	uint32_t m_nMaxSize;
	uint32_t m_nMaxSizeLess;
	Job ** m_vDataBuffer;
}simple_queue;

void SetMaxSize(uint32_t nMaxSize, simple_queue* g_queue);
  
void SimpleQueue(uint32_t nSize, simple_queue** g_queue); // uint32_t nSize=4096

int Put(struct Job* in, simple_queue* g_queue);

int Fetch(struct Job** out, simple_queue* g_queue);

uint32_t MaxSize(simple_queue* g_queue);

uint32_t Size(simple_queue* g_queue);

#ifdef __cplusplus
}
#endif

#endif//SIMPLEQUEUE_H




