#include "SimpleQueue.h"
#include <stdlib.h>


void SetMaxSize(uint32_t nMaxSize, simple_queue* g_queue)
{
	g_queue->m_nMaxSize=1;
	while(nMaxSize>>=1){
		g_queue->m_nMaxSize<<=1;
	} 
	g_queue->m_nMaxSizeLess = g_queue->m_nMaxSize - 1;
	g_queue->m_vDataBuffer = (Job**)malloc(g_queue->m_nMaxSize * sizeof(struct Job*));
}
  
void SimpleQueue(uint32_t nSize, simple_queue** g_queue) // uint32_t nSize=4096
{
	*g_queue = (simple_queue*)malloc(sizeof(struct simple_queue));
	SetMaxSize(nSize,*g_queue);
	(*g_queue)->m_nInPos=0;
	(*g_queue)->m_nOutPos=0;
}

int Put(struct Job* in, simple_queue* g_queue)
{
	if(g_queue->m_nInPos-g_queue->m_nOutPos>=g_queue->m_nMaxSize){
		return 0;
	}
	uint32_t nPos=g_queue->m_nInPos&g_queue->m_nMaxSizeLess;
	g_queue->m_vDataBuffer[nPos]=in;
	++g_queue->m_nInPos;
	return 1;
}

int Fetch(struct Job** out, simple_queue* g_queue)
{
	if(g_queue->m_nInPos-g_queue->m_nOutPos==0){
		return 0;
	}
	uint32_t nPos=g_queue->m_nOutPos&g_queue->m_nMaxSizeLess;
	*out=g_queue->m_vDataBuffer[nPos];
	++g_queue->m_nOutPos;
	return 1;
}

uint32_t MaxSize(simple_queue* g_queue)
{
	return g_queue->m_nMaxSize;
}

uint32_t Size(simple_queue* g_queue)
{
	return g_queue->m_nInPos-g_queue->m_nOutPos;
}