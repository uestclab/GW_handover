#include "bs_utility.h"

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

void timer_cb(void* in_data, g_msg_queue_para* g_msg_queue){
	postMsgWrapper(MSG_TIMEOUT, NULL, 0, g_msg_queue);
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

void printMsgType(long int type, uint16_t seq_id){
	if(type == MSG_RECEIVED_BEACON)
		printf("receive MSG_RECEIVED_BEACON : seq_id = %d \n", seq_id);
	else if(type == MSG_RECEIVED_ASSOCIATION_RESPONSE)
		printf("receive MSG_RECEIVED_ASSOCIATION_RESPONSE : seq_id = %d \n", seq_id);
	else if(type == MSG_RECEIVED_HANDOVER_START_RESPONSE)
		printf("receive MSG_RECEIVED_HANDOVER_START_RESPONSE : seq_id = %d \n", seq_id);
	else if(type == MSG_RECEIVED_REASSOCIATION)
		printf("receive MSG_RECEIVED_REASSOCIATION : seq_id = %d \n", seq_id);
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
	postMsgWrapper(MSG_CHECK_RECEIVED_LIST, (char*)(&subtype), sizeof(int32_t), g_msg_queue); // note 0319 --- _lq_
}

void resetReceivedList(received_state_list* list){
	list->received_association_response = 0;
	list->received_handover_start_response = 0;
	list->received_reassociation = 0;
}

void* retrans_air_process_thread(void* arg){
	retrans_air_t* tmp = (retrans_air_t*)arg;
	int32_t subtype = tmp->subtype;
	if(subtype == ASSOCIATION_REQUEST)
		usleep(2000);
	else if(subtype == HANDOVER_START_REQUEST)
		usleep(4000);
	else if(subtype == DEASSOCIATION)
		usleep(2000);
	else if(subtype == DISTANC_MEASURE_REQUEST){
		for(int i = 0;i<tmp->g_air->node->distance_measure_cnt_ms;i++){
			usleep(1000);
		}
	}
	postCheckSendSignal(tmp->subtype, tmp->g_msg_queue);
	free(tmp);
}

void postCheckWorkToThreadPool(int32_t subtype, g_msg_queue_para* g_msg_queue, g_air_para* g_air, ThreadPool* g_threadpool){
	retrans_air_t* tmp = (retrans_air_t*)malloc(sizeof(retrans_air_t));
	tmp->subtype = subtype;
	tmp->g_msg_queue = g_msg_queue;
	tmp->g_air = g_air;
	AddWorker(retrans_air_process_thread,(void*)tmp,g_threadpool);
} 

void checkReceivedList(int32_t subtype, system_info_para* g_system_info, g_msg_queue_para* g_msg_queue, 
					  g_air_para* g_air, ThreadPool* g_threadpool){
	received_state_list* list = g_system_info->received_air_state_list;
	if(subtype == ASSOCIATION_REQUEST){
		if(list->received_association_response == 0){
			char my_inital[6];
			memset(my_inital,0x0,6);
			memcpy(my_inital,(char*)(&(g_system_info->my_initial)), sizeof(uint32_t)); //20191024
			send_airSignal(ASSOCIATION_REQUEST, g_system_info->bs_mac, g_system_info->ve_mac, my_inital, g_air);
			postCheckWorkToThreadPool(subtype, g_msg_queue, g_air, g_threadpool);
		}
	}else if(subtype == HANDOVER_START_REQUEST){
		if(list->received_handover_start_response == 0){
			send_airSignal(HANDOVER_START_REQUEST, g_system_info->bs_mac, g_system_info->ve_mac, g_system_info->next_bs_mac, g_air);
			postCheckWorkToThreadPool(subtype, g_msg_queue, g_air, g_threadpool);
		}
	}else if(subtype == DEASSOCIATION){
		if(list->received_reassociation == 0){
			send_airSignal(DEASSOCIATION, g_system_info->bs_mac, g_system_info->ve_mac, g_system_info->bs_mac, g_air);
			postCheckWorkToThreadPool(subtype, g_msg_queue, g_air, g_threadpool);
		}else{
			zlog_info(g_air->log_handler, "list : %d ,%d ,%d \n", 
			list->received_association_response, list->received_handover_start_response, list->received_reassociation);
			resetReceivedList(list);
			zlog_info(g_air->log_handler,"resetReceivedList when received reassociation after send deassociation ");
		}
	}else if(subtype == DISTANC_MEASURE_REQUEST){
		if(g_system_info->bs_state == STATE_WORKING){
			char my_inital[6];
			memset(my_inital,0x0,6);
			memcpy(my_inital,(char*)(&(g_system_info->my_initial)), sizeof(uint32_t));
			send_airSignal(DISTANC_MEASURE_REQUEST, g_system_info->bs_mac, g_system_info->ve_mac, my_inital, g_air);
			postCheckWorkToThreadPool(DISTANC_MEASURE_REQUEST, g_msg_queue, g_air, g_threadpool);
		}else{
			zlog_info(g_air->log_handler,"stop send air frame DISTANC_MEASURE_REQUEST !! \n ");
		}
	}
}

/* ------------------------------------- */

void* check_air_tx_data_statistics(void* args){
	struct g_test_para* g_test_all = (g_test_para*)args;
	g_RegDev_para*         g_RegDev = g_test_all->g_RegDev;
	zlog_category_t*    log_handler = g_RegDev->log_handler; 
	
	zlog_info(log_handler,"rx_byte_filter_ether_low32() = %x \n", rx_byte_filter_ether_low32(g_RegDev));
	zlog_info(log_handler,"rx_byte_filter_ether_high32() = %x \n", rx_byte_filter_ether_high32(g_RegDev));
	int wait_cnt = 0;
	while(1)
	{
		usleep(500);
		zlog_info(log_handler,"rx_byte_filter_ether_low32() = %x \n", rx_byte_filter_ether_low32(g_RegDev));
		zlog_info(log_handler,"rx_byte_filter_ether_high32() = %x \n", rx_byte_filter_ether_high32(g_RegDev));
		if(wait_cnt == g_test_all->node->check_eth_rx_cnt) // move to configure node parameter : parameter = 2
			break;
		wait_cnt = wait_cnt + 1;		
	}

	postMsgWrapper(MSG_START_HANDOVER_THROUGH_AIR, NULL, 0, g_test_all->g_msg_queue);

	free(g_test_all);
}

void postCheckTxBufferWorkToThreadPool(struct ConfigureNode* Node, g_msg_queue_para* g_msg_queue, 
									   g_RegDev_para* g_RegDev, ThreadPool* g_threadpool)
{
	struct g_test_para* g_test_all = (g_test_para*)malloc(sizeof(g_test_para));
	g_test_all->g_msg_queue = g_msg_queue;
	g_test_all->g_RegDev = g_RegDev;
	g_test_all->node = Node;
	AddWorker(check_air_tx_data_statistics,(void*)g_test_all,g_threadpool);
}



/* -------------------------------------------------------------------------- */
void resetReceivedNetworkList(received_network_list* list){
	list->received_dac_closed_x2 = 0;
	list->received_dac_closed_x2_ack = 0;
}

void postCheckNetworkSignal(int32_t signal, g_msg_queue_para* g_msg_queue){
	int msg_len = sizeof(int32_t);
	postMsgWrapper(MSG_CHECK_RECEIVED_NETWORK_LIST, (char*)(&signal), msg_len, g_msg_queue); // note 0319 --- _lq_
}

void* retrans_network_process_thread(void* arg){
	retrans_network_t* tmp = (retrans_network_t*)arg;
	int32_t signal = tmp->signal;
	if(signal == MSG_RECEIVED_DAC_CLOSED_ACK)
		usleep(2000);
	else if(signal == MSG_SOURCE_BS_DAC_CLOSED)
		usleep(2000);
	postCheckNetworkSignal(tmp->signal, tmp->g_msg_queue);
	free(tmp);
}

void postCheckNetworkSignalWorkToThreadPool(int32_t signal, g_msg_queue_para* g_msg_queue, ThreadPool* g_threadpool){
	retrans_network_t* tmp = (retrans_network_t*)malloc(sizeof(retrans_network_t));
	tmp->signal = signal;
	tmp->g_msg_queue = g_msg_queue;
	AddWorker(retrans_network_process_thread,(void*)tmp,g_threadpool);
}

void checkNetWorkReceivedList(int32_t signal, system_info_para* g_system_info, g_msg_queue_para* g_msg_queue, 
					  g_x2_para* g_x2, ThreadPool* g_threadpool){
	received_network_list* list = g_system_info->received_network_state_list;
	if(signal == MSG_RECEIVED_DAC_CLOSED_ACK){
		if(list->received_dac_closed_x2_ack == 0){
			send_dac_closed_x2_signal(g_x2->node->my_id, g_x2->node->udp_server_ip, g_x2);
			postCheckNetworkSignalWorkToThreadPool(signal, g_msg_queue, g_threadpool);
		}else{
			zlog_info(g_x2->log_handler, "network list : %d ,%d \n", 
			list->received_dac_closed_x2, list->received_dac_closed_x2_ack);
			resetReceivedNetworkList(list);
			zlog_info(g_x2->log_handler,"resetReceivedNetworkList when received dac close ack ");
		}
	}
}





