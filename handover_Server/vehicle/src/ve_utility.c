#include "ve_utility.h"


void postCheckSendSignal(int32_t subtype, g_msg_queue_para* g_msg_queue){
	struct msg_st data;
	data.msg_type = MSG_CHECK_RECEIVED_LIST;
	data.msg_number = MSG_CHECK_RECEIVED_LIST;
	data.msg_len = sizeof(int32_t);
	memcpy(data.msg_json, (char*)(&subtype), data.msg_len);
	postMsgQueue(&data,g_msg_queue);
}

void* retrans_air_process_thread(void* arg){
	retrans_air_t* tmp = (retrans_air_t*)arg;
	int32_t subtype = tmp->subtype;

	if(subtype == DISTANC_MEASURE_REQUEST){
		for(int i = 0;i<tmp->g_air->node->distance_measure_cnt_ms;i++){
			usleep(1000);
		}
	}
	postCheckSendSignal(tmp->subtype, tmp->g_msg_queue);
	free(tmp);
}

// extern interface
void postCheckWorkToThreadPool(int32_t subtype, g_msg_queue_para* g_msg_queue, g_air_para* g_air, ThreadPool* g_threadpool){
	retrans_air_t* tmp = (retrans_air_t*)malloc(sizeof(retrans_air_t));
	tmp->subtype = subtype;
	tmp->g_msg_queue = g_msg_queue;
	tmp->g_air = g_air;
	AddWorker(retrans_air_process_thread,(void*)tmp,g_threadpool);
} 

void checkReceivedList(int32_t subtype, system_info_para* g_system_info, g_msg_queue_para* g_msg_queue, 
					  g_air_para* g_air, ThreadPool* g_threadpool){
    if(subtype == DISTANC_MEASURE_REQUEST){
		if(g_system_info->ve_state == STATE_WORKING){
			char my_inital[6];
			memset(my_inital,0x0,6);
			memcpy(my_inital,(char*)(&(g_system_info->my_initial)), sizeof(uint32_t));
			send_airSignal(DISTANC_MEASURE_REQUEST, g_system_info->ve_mac, g_system_info->link_bs_mac, my_inital, g_air);
			postCheckWorkToThreadPool(DISTANC_MEASURE_REQUEST, g_msg_queue, g_air, g_threadpool);
		}else{
			zlog_info(g_air->log_handler,"stop send air frame DISTANC_MEASURE_REQUEST !! \n ");
		}
	}
}







