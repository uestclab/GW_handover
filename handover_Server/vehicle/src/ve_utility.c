#include "ve_utility.h"

void postMsgWrapper(long int msg_type, char *buf, int buf_len, g_msg_queue_para* g_msg_queue){
	struct msg_st* data = (struct msg_st*)malloc(sizeof(struct msg_st));
	data->msg_type = msg_type;
	data->msg_number = msg_type;

	data->msg_len = buf_len;
	if(buf != NULL && buf_len != 0)
		memcpy(data->msg_json,buf,buf_len);
	
	int level = 0;
	postMsgQueue(data,level,g_msg_queue);
}

void postCheckSendSignal(retrans_air_t* tmp, g_msg_queue_para* g_msg_queue){
	int msg_len = sizeof(retrans_air_t);
	postMsgWrapper(MSG_CHECK_RECEIVED_LIST, (char*)tmp, msg_len, g_msg_queue);
}

void* retrans_air_process_thread(void* arg){
	retrans_air_t* tmp = (retrans_air_t*)arg;
	int32_t subtype = tmp->subtype;

	if(subtype == DISTANC_MEASURE_REQUEST){
		for(int i = 0;i<tmp->g_air->node->distance_measure_cnt_ms;i++){
			usleep(1000);
		}
	}
	postCheckSendSignal(tmp, tmp->g_msg_queue);
	free(tmp);
}

// extern interface
void postCheckWorkToThreadPool(int32_t subtype, int id, g_msg_queue_para* g_msg_queue, g_air_para* g_air, ThreadPool* g_threadpool){
	retrans_air_t* tmp = (retrans_air_t*)malloc(sizeof(retrans_air_t));
	tmp->subtype = subtype;
	tmp->id = id;
	tmp->g_msg_queue = g_msg_queue;
	tmp->g_air = g_air;
	AddWorker(retrans_air_process_thread,(void*)tmp,g_threadpool);
} 

void checkReceivedList(int32_t subtype, int id, system_info_para* g_system_info, g_msg_queue_para* g_msg_queue, 
					  g_air_para* g_air, ThreadPool* g_threadpool){
    if(subtype == DISTANC_MEASURE_REQUEST){
		if(g_system_info->ve_state == STATE_WORKING){
			char my_inital[6];
			memset(my_inital,0x0,6);
			memcpy(my_inital,(char*)(&(g_system_info->my_initial)), sizeof(uint32_t));
			send_airSignal(DISTANC_MEASURE_REQUEST, g_system_info->ve_mac, g_system_info->link_bs_mac, my_inital, g_air);
			postCheckWorkToThreadPool(DISTANC_MEASURE_REQUEST, id, g_msg_queue, g_air, g_threadpool);
		}else{
			zlog_info(g_air->log_handler,"stop send air frame DISTANC_MEASURE_REQUEST !! \n ");
		}
	}
}







