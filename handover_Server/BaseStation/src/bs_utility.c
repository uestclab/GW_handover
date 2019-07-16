#include "bs_utility.h"

void timer_cb(void* in_data, g_msg_queue_para* g_msg_queue){
	struct msg_st data;
	data.msg_type = MSG_TIMEOUT;
	data.msg_number = MSG_TIMEOUT;
	data.msg_len = 0;
	postMsgQueue(&data,g_msg_queue);
}

int is_my_air_frame(char* src, char* dest){
	int i = 0;
	for(i=0;i<6;i++){
		if(src[i] != dest[i])
			return -1;
	}
	return 0;
}

void configureDstMacToBB(char* dst_buf, g_RegDev_para* g_RegDev, zlog_category_t* zlog_handler){
	set_dst_mac_fast(g_RegDev, dst_buf);
}

struct net_data* parseNetMsg(struct msg_st* getData){

	char* msg_json = getData->msg_json;
	if(getData->msg_len == 0)
		return NULL;
	cJSON * root = NULL;
	cJSON * item = NULL;
	root = cJSON_Parse(msg_json);

	struct net_data* tmp_data = (struct net_data*)malloc(sizeof(struct net_data));
	item = cJSON_GetObjectItem(root, "target_bs_mac");
	char target_mac_buf[6];
	change_mac_buf(item->valuestring,target_mac_buf);
	memcpy(tmp_data->target_bs_mac,target_mac_buf,6);
	item = cJSON_GetObjectItem(root, "target_bs_ip");
	memcpy(tmp_data->target_bs_ip,item->valuestring,strlen(item->valuestring)+1);

	cJSON_Delete(root);
	return tmp_data;
}

void printMsgType(long int type){
	if(type == MSG_RECEIVED_BEACON)
		printf("receive MSG_RECEIVED_BEACON \n");
	else if(type == MSG_RECEIVED_ASSOCIATION_RESPONSE)
		printf("receive MSG_RECEIVED_ASSOCIATION_RESPONSE \n");
	else if(type == MSG_RECEIVED_HANDOVER_START_RESPONSE)
		printf("receive MSG_RECEIVED_HANDOVER_START_RESPONSE \n");
	else if(type == MSG_RECEIVED_REASSOCIATION)
		printf("receive MSG_RECEIVED_REASSOCIATION \n");
}

struct air_data* parseAirMsg(char* msg_json){
	struct air_data* tmp_data = (struct air_data*)malloc(sizeof(struct air_data));
	memcpy(tmp_data,msg_json,sizeof(struct air_data));
	return tmp_data;
}

/* duplicate receive and retransmit air frame */

int checkAirFrameDuplicate(msg_event receivedAirEvent, system_info_para* g_system_info){
	int flag = 0;
	received_state_list* list = g_system_info->received_air_state_list;
	switch(receivedAirEvent){
		case MSG_RECEIVED_ASSOCIATION_RESPONSE:
		{
			if(list->received_association_response == 0){
				list->received_association_response = 1;
			}else{
				flag = 1;
			}			
			break;
		}
		case MSG_RECEIVED_HANDOVER_START_RESPONSE:
		{
			if(list->received_handover_start_response == 0){
				list->received_handover_start_response = 1;
			}else{
				flag = 1;
			}
			break;
		}
		case MSG_RECEIVED_REASSOCIATION:
		{
			if(list->received_reassociation == 0){
				list->received_reassociation = 1;
			}else{
				flag =1;
			}
			break;
		}
	}
	return flag;
}

void postCheckSendSignal(int32_t subtype, g_msg_queue_para* g_msg_queue){
	struct msg_st data;
	data.msg_type = MSG_CHECK_RECEIVED_LIST;
	data.msg_number = MSG_CHECK_RECEIVED_LIST;
	data.msg_len = sizeof(int32_t);
	memcpy(data.msg_json, (char*)(&subtype), data.msg_len);
	postMsgQueue(&data,g_msg_queue);
}

void resetReceivedList(received_state_list* list){
	list->received_association_response = 0;
	list->received_handover_start_response = 0;
	list->received_reassociation = 0;
}

void* retrans_air_process_thread(void* arg){
	retrans_air_t* tmp = (retrans_air_t*)arg;
	//zlog_info(tmp->g_msg_queue->log_handler,"retrans_air_process_thread : thread_Id = %lu \n", pthread_self());
	int32_t subtype = tmp->subtype;
	if(subtype == ASSOCIATION_REQUEST)
		usleep(2000);
	else if(subtype == HANDOVER_START_REQUEST)
		usleep(4000);
	else if(subtype == DEASSOCIATION)
		usleep(2000);
	postCheckSendSignal(tmp->subtype, tmp->g_msg_queue);
	free(tmp);
}

void postCheckWorkToThreadPool(int32_t subtype, g_msg_queue_para* g_msg_queue, ThreadPool* g_threadpool){
	retrans_air_t* tmp = (retrans_air_t*)malloc(sizeof(retrans_air_t));
	tmp->subtype = subtype;
	tmp->g_msg_queue = g_msg_queue;
	AddWorker(retrans_air_process_thread,(void*)tmp,g_threadpool);
} 

void checkReceivedList(int32_t subtype, system_info_para* g_system_info, g_msg_queue_para* g_msg_queue, 
					  g_air_para* g_air, ThreadPool* g_threadpool){
	received_state_list* list = g_system_info->received_air_state_list;
	if(subtype == ASSOCIATION_REQUEST){
		if(list->received_association_response == 0){
			send_airSignal(ASSOCIATION_REQUEST, g_system_info->bs_mac, g_system_info->ve_mac, g_system_info->bs_mac, g_air);
			postCheckWorkToThreadPool(subtype, g_msg_queue, g_threadpool);
		}	
	}else if(subtype == HANDOVER_START_REQUEST){
		if(list->received_handover_start_response == 0){
			send_airSignal(HANDOVER_START_REQUEST, g_system_info->bs_mac, g_system_info->ve_mac, g_system_info->next_bs_mac, g_air);
			postCheckWorkToThreadPool(subtype, g_msg_queue, g_threadpool);
		}
	}else if(subtype == DEASSOCIATION){
		if(list->received_reassociation == 0){
			send_airSignal(DEASSOCIATION, g_system_info->bs_mac, g_system_info->ve_mac, g_system_info->bs_mac, g_air);
			postCheckWorkToThreadPool(subtype, g_msg_queue, g_threadpool);;
		}else{
			zlog_info(g_air->log_handler, "list : %d ,%d ,%d \n", 
			list->received_association_response, list->received_handover_start_response, list->received_reassociation);
			resetReceivedList(list);
			zlog_info(g_air->log_handler,"resetReceivedList when received reassociation after send deassociation ");
		}
	}
}
