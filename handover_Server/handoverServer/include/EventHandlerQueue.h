#ifndef GLORY_EVENT_H
#define GLORY_EVENT_H

#include "msg_queue.h"
#include <pthread.h> 
#include "ThreadPool.h"

typedef struct t_IDInfo{
    int bs_id;
    int ve_id;
    double dist;
}t_IDInfo;


template <typename TYPE, void(TYPE::*_RunThread)() >
void* _thread_t(void* param)
{

   TYPE* This = (TYPE*)param;

   This->_RunThread();

   return NULL;

}

typedef enum msg_event{
	MSG_SYS_MONITOR = 1,
    MSG_TIMEOUT,

    MSG_REVC_BEACON,
    MSG_INIT_DISTANCE_CHECK_TIMEOUT,
    MSG_INIT_DISTANCE_OVER,
    MSG_READY_HANDOVER,
    MSG_INIT_COMPLETED,
    MSG_LINK_CLOSED,
    MSG_LINK_OPEN,
    MSG_CHANGE_TUNNEL,

}msg_event;

class Manager;
class EventHandlerQueue
{
public:
    EventHandlerQueue(Manager* pManager);
    ~EventHandlerQueue(void);
    
    int post_msg(long int msg_type, char *buf, int buf_len, int bs_id, int ve_id, double dist=0);
    int start_event_process_thread(ThreadPool* pThreadpool);

    void _RunThread(); // eventLoop

private:
    void process_event(struct msg_st* getData);

private:
    pthread_t              thread_pid_;
    Manager*               pManager_;
    g_msg_queue_para*      pMsgQueue_;
    ThreadPool*            pThreadpool_;

};




#endif  /* ndef GLORY_EVENT_H */
