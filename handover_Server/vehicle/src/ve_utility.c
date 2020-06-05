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
	}else if(subtype == KEEP_ALIVE){
		usleep(50000);
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
	}else if(subtype == KEEP_ALIVE){
		received_state_list* list = g_system_info->received_air_state_list;
		if(list->received_keepAlive_ack == 0){// not received 
			g_system_info->miss_response_cnt++;
			if(g_system_info->miss_response_cnt == 5){
				// start relocalization process and reset system
				;
			}else{
				sendAndCheck_keepAlive(g_system_info,g_air,g_threadpool);
			}
		}else{ // received response
			sendAndCheck_keepAlive(g_system_info,g_air,g_threadpool);
			list->received_keepAlive_ack = 0;
			g_system_info->miss_response_cnt = 0;
		}
	}
}

void sendAndCheck_keepAlive(system_info_para* g_system_info, g_air_para* g_air, ThreadPool* g_threadpool){
	char my_inital[6];
	memset(my_inital,0x0,6);
	uint8_t ve_id = 0;
	ve_id = (uint8_t)(g_system_info->ve_id);
	memcpy(my_inital+sizeof(uint32_t),(char*)(&(ve_id)), sizeof(uint8_t));

	send_airSignal(KEEP_ALIVE, g_system_info->ve_mac, g_system_info->link_bs_mac, my_inital, g_air);
	postCheckWorkToThreadPool(KEEP_ALIVE, 0, g_air->g_msg_queue, g_air, g_threadpool);
}
