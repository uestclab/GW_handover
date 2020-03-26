#include "EventHandlerQueue.h"
#include <string>
#include "manager.h"
#include <glog/logging.h>
#include "ThreadFuncWrapper.h"
#include "vehicleInfo.h"
#include "signal_json.h"

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

int EventHandlerQueue::start_event_process_thread(ThreadPool* pThreadpool){
	pThreadpool_ = pThreadpool;
    if(pMsgQueue_ == NULL){
		LOG(ERROR) << "error ! pMsgQueue_ == NULL in start_event_process_thread() "; 
        return -1;
    }
    int ret = pthread_create(&thread_pid_, NULL, _thread_t<EventHandlerQueue,&EventHandlerQueue::_RunThread>, this);
    if(ret != 0){
        LOG(ERROR) << "error ! --- create monitor_thread error ! error_code = " << ret;
    }
    return ret;
}

int EventHandlerQueue::post_msg(long int msg_type, char *buf, int buf_len, int bs_id, int ve_id, double dist){
	struct msg_st* data = (struct msg_st*)malloc(sizeof(struct msg_st));
	data->msg_type = msg_type;
	data->msg_number = msg_type;

	data->msg_len = buf_len;
	if(buf != NULL && buf_len != 0)
		memcpy(data->msg_json,buf,buf_len);
	
	t_IDInfo* id_info = (t_IDInfo*)malloc(sizeof(t_IDInfo));
	id_info->bs_id = bs_id;
	id_info->ve_id = ve_id;
	id_info->dist = dist;
	data->tmp_data = (void*)id_info;

	int level = 0;
	int ret = postMsgQueue(data,level,pMsgQueue_);
    if(ret != 0){
        LOG(ERROR) << "post_msg error !";
    }
    return ret;
}

void freeMsg(struct msg_st* msgData){
	if(msgData->tmp_data != NULL){
		free(msgData->tmp_data);
		msgData->tmp_data = NULL;
	}
	free(msgData);
}

void EventHandlerQueue::_RunThread(){
	LOG(INFO) << " ------------------------------ start EventHandlerQueue event loop ----------------------------- \n";
	while(1){
		struct msg_st* getData = getMsgQueue(pMsgQueue_);
		if(getData == NULL)
			continue;

		process_event(getData);

		freeMsg(getData);
	}
}

void EventHandlerQueue::process_event(struct msg_st* getData){
	switch(getData->msg_type){
		case MSG_SYS_MONITOR:
		{
			LOG(INFO) << getData->msg_number << " -- MSG_SYS_MONITOR : ";
			break;
		}
		case MSG_TIMEOUT:
		{
			LOG(INFO) << getData->msg_number << " -- MSG_TIMEOUT : ";
			break;
		}
		case MSG_REVC_BEACON:
		{
			LOG(INFO) << getData->msg_number << " -- MSG_REVC_BEACON : ";
			t_IDInfo* id_info = (t_IDInfo*)getData->tmp_data;

			pManager_->transferToVe(id_info->ve_id, id_info->bs_id);

			break;
		}
		case MSG_INIT_DISTANCE_CHECK_TIMEOUT:
		{
			LOG(INFO) << getData->msg_number << " -- MSG_INIT_DISTANCE_CHECK_TIMEOUT : ";
			t_IDInfo* id_info = (t_IDInfo*)getData->tmp_data;
			pManager_->informInitDistance(id_info->ve_id); // call bs distance
			break;
		}
		case MSG_INIT_DISTANCE_OVER:
		{
			LOG(INFO) << getData->msg_number << " -- MSG_INIT_DISTANCE_OVER : ";
			
			t_IDInfo* id_info = (t_IDInfo*)getData->tmp_data;
			pManager_->saveBsDistance(id_info->ve_id, id_info->bs_id, id_info->dist);
			pManager_->informInitDistance(id_info->ve_id);// call bs distance

			break;
		}
		case MSG_READY_HANDOVER:
		{
			LOG(INFO) << getData->msg_number << " -- MSG_READY_HANDOVER : ";
			break;
		}
		case MSG_INIT_COMPLETED:
		{
			LOG(INFO) << getData->msg_number << " -- MSG_INIT_COMPLETED : ";
			t_IDInfo* id_info = (t_IDInfo*)getData->tmp_data;
			BaseStation* bs = pManager_->findBsById(id_info->bs_id);
            pManager_->establishLink(bs, id_info->ve_id);
			// pManager_->updateExpectId(this);
			// pManager_->recall_other_bs(this);
			break;
		}
		case MSG_LINK_CLOSED:
		{
			LOG(INFO) << getData->msg_number << " -- MSG_LINK_CLOSED : ";
			t_IDInfo* id_info = (t_IDInfo*)getData->tmp_data;
			pManager_->incChangeLink(id_info->bs_id,id_info->ve_id,0);
			// send_server_recall_monitor_signal(this, this->getBaseStationID());
			break;
		}
		case MSG_LINK_OPEN:
		{
			LOG(INFO) << getData->msg_number << " -- MSG_LINK_OPEN : ";
			t_IDInfo* id_info = (t_IDInfo*)getData->tmp_data;
			pManager_->incChangeLink(id_info->bs_id,id_info->ve_id,1);
			// pManager_->updateExpectId(this);
			break;
		}
		case MSG_CHANGE_TUNNEL:
		{
			LOG(INFO) << getData->msg_number << " -- MSG_CHANGE_TUNNEL : ";
			t_IDInfo* id_info = (t_IDInfo*)getData->tmp_data;
			pManager_->change_tunnel_Link(id_info->ve_id);
			BaseStation* bs = pManager_->findBsById(id_info->bs_id);
			send_change_tunnel_ack_signal(bs, bs->getBaseStationID());
			break;
		}
	}
}
