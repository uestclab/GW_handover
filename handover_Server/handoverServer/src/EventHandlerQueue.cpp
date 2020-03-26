#include "EventHandlerQueue.h"
#include <string>

#include <glog/logging.h>

using namespace std;

EventHandlerQueue::EventHandlerQueue(Manager* pManager){
    pManager_ = pManager;
    pMsgQueue_ = createMsgQueue();
 	if(pMsgQueue_ == NULL){
		LOG(ERROR) << "EventHandlerQueue failed !";
	}
}

EventHandlerQueue::~EventHandlerQueue(void){
    if(pMsgQueue_ != NULL){
        delMsgQueue(pMsgQueue_);
    }
}

int EventHandlerQueue::start_event_process_thread(){
    if(pMsgQueue_ == NULL){
		LOG(ERROR) << "error ! pMsgQueue_ == NULL in start_event_process_thread() "; 
        return -1;
    }
    int ret = pthread_create(&thread_pid_, NULL, _thread_t<EventHandlerQueue,&EventHandlerQueue::_RunThread>, this);
    if(ret != 0){
        LOG(ERROR) << "error ! --- create monitor_thread error ! error_code = " << ret;
    }
	pthread_t              test_pid;
	ret = pthread_create(&test_pid, NULL, _test_thread_t<EventHandlerQueue,&EventHandlerQueue::_TestThread>, this);
    if(ret != 0){
        LOG(ERROR) << "error ! --- create test_thread error ! error_code = " << ret;
    }

    return ret;
}

int EventHandlerQueue::post_msg(long int msg_type, char *buf, int buf_len){
	struct msg_st* data = (struct msg_st*)malloc(sizeof(struct msg_st));
	data->msg_type = msg_type;
	data->msg_number = msg_type;

	data->msg_len = buf_len;
	if(buf != NULL && buf_len != 0)
		memcpy(data->msg_json,buf,buf_len);
	
	int level = 0;
	int ret = postMsgQueue(data,level,pMsgQueue_);
    if(ret != 0){
        LOG(ERROR) << "post_msg error !";
    }
    return ret;
}

void EventHandlerQueue::_RunThread(){
	LOG(INFO) << " ------------------------------ start EventHandlerQueue event loop ----------------------------- \n";
	while(1){
		struct msg_st* getData = getMsgQueue(pMsgQueue_);
		if(getData == NULL)
			continue;

		process_event(getData);
		// else if(getData->msg_type >= MSG_TIMEOUT)
		// 	process_self_event(getData);

		free(getData);
	}
}

void EventHandlerQueue::process_event(struct msg_st* getData){
	switch(getData->msg_type){
		case MSG_MONITOR:
		{
			LOG(INFO) << getData->msg_number << " -- MSG_MONITOR : msg_json = " << getData->msg_json << ",msg_len = " << getData->msg_len;
			break;
		}
		case MSG_NETWORK:
		{
			LOG(INFO) << getData->msg_number << " -- MSG_NETWORK : ";
			break;
		}
	}
}

// test code
void EventHandlerQueue::_TestThread(){
	for(;;){
		int ret = post_msg(MSG_NETWORK, NULL, 0);
		sleep(1);
	}

}