#include <pthread.h>
#include "bs_air.h"
#include "gw_frame.h"
#include "cJSON.h"

struct air_data* bufTojson(management_frame_Info* Info, g_air_para* g_air){
/*
typedef struct air_data{
	char source_mac_addr[6];
	char dest_mac_addr[6];
	char Next_dest_mac_addr[6];
	uint16_t seq_id;
}air_data;
*/
	struct air_data* tmp_data = (struct air_data*)malloc(sizeof(struct air_data));

	tmp_data->seq_id = Info->seq_id;
	memcpy(tmp_data->source_mac_addr,Info->source_mac_addr,6);
	memcpy(tmp_data->dest_mac_addr,Info->dest_mac_addr,6);
	memcpy(tmp_data->Next_dest_mac_addr,Info->Next_dest_mac_addr,6);

	return tmp_data;
}


void process_recived_signal(management_frame_Info* Info, g_air_para* g_air){ // add parse seq_id ---- continue
	struct msg_st data;
	struct air_data* json_buf = bufTojson(Info,g_air);
	data.msg_len = sizeof(struct air_data);
	memcpy(data.msg_json, (char*)json_buf, data.msg_len);
	switch(Info->subtype){
		case BEACON:
		{
			data.msg_type = MSG_RECEIVED_BEACON;
			data.msg_number = MSG_RECEIVED_BEACON;
			postMsgQueue(&data,g_air->g_msg_queue);
			break;
		}
		case ASSOCIATION_RESPONSE:
		{
			data.msg_type = MSG_RECEIVED_ASSOCIATION_RESPONSE;
			data.msg_number = MSG_RECEIVED_ASSOCIATION_RESPONSE;
			postMsgQueue(&data,g_air->g_msg_queue);
			break;
		}
		case HANDOVER_START_RESPONSE:
		{
			data.msg_type = MSG_RECEIVED_HANDOVER_START_RESPONSE;
			data.msg_number = MSG_RECEIVED_HANDOVER_START_RESPONSE;
			postMsgQueue(&data,g_air->g_msg_queue);
			break;
		}
		case REASSOCIATION:
		{
			data.msg_type = MSG_RECEIVED_REASSOCIATION;
			data.msg_number = MSG_RECEIVED_REASSOCIATION;
			postMsgQueue(&data,g_air->g_msg_queue);
			break;
		}
		default:
		{
			zlog_error(g_air->log_handler,"error ! --- error subtype : Info->subtype = %d \n", Info->subtype);
			break;
		}
	}
	free(json_buf);
}


void gw_poll_receive(g_air_para* g_air){
	zlog_info(g_air->log_handler,"start gw_poll_receive .......  \n");
	int status;
	management_frame_Info* temp_Info = new_air_frame(-1,0,NULL,NULL,NULL,0);
	while(1){ // wait for air_signal
		int stat = gw_monitor_poll(temp_Info, 2, g_air->log_handler); // receive 2 * 5ms
		if(stat == 26){
			zlog_info(g_air->log_handler,"receive new air frame \n");
			process_recived_signal(temp_Info, g_air);
		}else if(stat < 26){
			//zlog_error(g_air->log_handler,"error ! --- poll timeout , stat = %d \n" , stat);
		}
	}
	free(temp_Info);
}

void* process_air_thread(void* args){
	g_air_para* g_air = (g_air_para*)args;

	pthread_mutex_lock(g_air->para_t->mutex_);
    while(1){
		while (g_air->running == -1 ) // for beacon test --- 20190401
		{
			zlog_info(g_air->log_handler,"process_air_thread() : wait for condition\n");
			pthread_cond_wait(g_air->para_t->cond_, g_air->para_t->mutex_);
		}
		pthread_mutex_unlock(g_air->para_t->mutex_);
    	gw_poll_receive(g_air); // 1. gw_poll_receive() 2. process_air()
    }
    zlog_info(g_air->log_handler,"Exit process_air_thread()");

}

int initProcessAirThread(struct ConfigureNode* Node, g_air_para** g_air, 
		g_msg_queue_para*  g_msg_queue, zlog_category_t* handler){
	//zlog_info(handler,"initProcessAirThread()");
	*g_air = (g_air_para*)malloc(sizeof(struct g_air_para));
	(*g_air)->log_handler = handler;
	(*g_air)->para_t = newThreadPara();
	(*g_air)->send_para_t = newThreadPara();
	(*g_air)->running = -1;
	(*g_air)->node = Node;
	(*g_air)->g_msg_queue = g_msg_queue;

	//zlog_info(handler,"g_msg_queue->msgid = %d \n" , (*g_air)->g_msg_queue->msgid);
	int ret = pthread_create((*g_air)->para_t->thread_pid, NULL, process_air_thread, (void*)(*g_air));
    if(ret != 0){
        zlog_error(handler,"error ! --- create monitor_thread error ! error_code = %d", ret);
		return -1;
    }
	
	int fd = ManagementFrame_create_monitor_interface();
	if(fd < 0){
		zlog_error(handler,"error ! --- ManagementFrame_create_monitor_interface : fd = %d \n" , fd);
		return fd;
	}

	return 0;
}

int  freeProcessAirThread(g_air_para* g_air){
	zlog_info(g_air->log_handler,"freeProcessAirThread()");
	free(g_air);
}

void startProcessAir(g_air_para* g_air, int running_step){
	zlog_info(g_air->log_handler,"startProcessAir : running_step = %d \n" , running_step);
	g_air->running = running_step;
	pthread_cond_signal(g_air->para_t->cond_);
}

void print_subtype(int32_t subtype, g_air_para* g_air){
	if(subtype == 0)
		zlog_info(g_air->log_handler,"send_airSignal :  subtype = BEACON , send successful \n");
	else if(subtype == 1)
		zlog_info(g_air->log_handler,"send_airSignal :  subtype = ASSOCIATION_REQUEST , send successful \n");
	else if(subtype == 2)
		zlog_info(g_air->log_handler,"send_airSignal :  subtype = ASSOCIATION_RESPONSE , send successful \n");
	else if(subtype == 3)
		zlog_info(g_air->log_handler,"send_airSignal :  subtype = REASSOCIATION , send successful \n");
	else if(subtype == 4)
		zlog_info(g_air->log_handler,"send_airSignal :  subtype = DEASSOCIATION , send successful \n");
	else if(subtype == 5)
		zlog_info(g_air->log_handler,"send_airSignal :  subtype = HANDOVER_START_REQUEST , send successful \n");
	else if(subtype == 6)
		zlog_info(g_air->log_handler,"send_airSignal :  subtype = HANDOVER_START_RESPONSE , send successful \n");
}


int send_airSignal(int32_t subtype, char* mac_buf, char* mac_buf_dest, char* mac_buf_next, g_air_para* g_air){
	pthread_mutex_lock(g_air->send_para_t->mutex_);
	system_info_para* g_system_info = g_air->node->system_info;
	g_system_info->send_id = g_system_info->send_id  + 1;
	int status;
	management_frame_Info* frame_Info = new_air_frame(subtype, 0,mac_buf,mac_buf_dest,mac_buf_next,g_system_info->send_id);
	status = handle_air_tx(frame_Info,g_air->log_handler);
	free(frame_Info);
	pthread_mutex_unlock(g_air->send_para_t->mutex_);

	if(status == 26){ // send success
		print_subtype(subtype,g_air);
	}else if(status < 26){
		zlog_error(g_air->log_handler,"error ! --- air_tx,status = %d , subtype = %d \n" , status, subtype);
	}
	return status;
}




