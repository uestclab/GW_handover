#include <pthread.h>
#include "air_process.h"
#include "gw_frame.h"
#include "cJSON.h"
#include <assert.h>


/* thread process , call ipc_send interface */
void* call_ipc_send_task(void* args){
	g_transfer_para* g_transfer = (g_transfer_para*)args;
	management_frame_Info* tranfer_buf = g_transfer->tranfer_buf;
	g_air_frame_para* g_air_frame = g_transfer->g_air_frame;
	// call ipc send , transfer management_frame_info data to other programe

	zlog_info(g_air_frame->log_handler, "call_ipc_send_task .... -- receive_air :  seq_id = %u, subtype =%d\n", 
	tranfer_buf->seq_id, tranfer_buf->subtype);
	switch(tranfer_buf->subtype){
		/* bs receive */
		case BEACON:
		case ASSOCIATION_RESPONSE:
		case HANDOVER_START_RESPONSE:
		case REASSOCIATION:
		{
			uint16_t send_id = 0;
			char mac_buf[6] = {0};
			char mac_buf_dest[6] = {0};
			char mac_buf_next[6] = {0};


			management_frame_Info* frame_Info = new_air_frame(ASSOCIATION_REQUEST, 0,mac_buf,mac_buf_dest,mac_buf_next,send_id);   
			int ret = send_airFrame((char*)frame_Info, sizeof(management_frame_Info), &(g_air_frame->g_ipc_server));
			free(frame_Info);


			send_to_client((char*)tranfer_buf, sizeof(management_frame_Info), 
					"baseStation", &(g_air_frame->g_ipc_server)); // for test
			break;
		}
		/* ve receive */
		case ASSOCIATION_REQUEST:
		case DEASSOCIATION:
		case HANDOVER_START_REQUEST:
		{
			send_to_client((char*)tranfer_buf, sizeof(management_frame_Info), 
					"vehicle", &(g_air_frame->g_ipc_server)); // for test
			break;
		}
		/* distance receive */
		case DISTANC_MEASURE_REQUEST:
		case DELAY_EXCHANGE_REQUEST:
		case DELAY_EXCHANGE_RESPONSE:
		{
			break;
		}
		default:
		{
			break;
		}
	}
	free(g_transfer);
}

void postCallSendWorkToThreadPool(management_frame_Info* tranfer_buf, g_air_frame_para* g_air_frame)
{
	struct g_transfer_para* g_transfer = (g_transfer_para*)malloc(sizeof(g_transfer_para));
	g_transfer->tranfer_buf = tranfer_buf;
	g_transfer->g_air_frame = g_air_frame;
	AddWorker(call_ipc_send_task,(void*)g_transfer,g_air_frame->g_threadpool);
}

/* call icp send */
void dispatch_recived_air_frame(management_frame_Info* Info, g_air_frame_para* g_air_frame){
	management_frame_Info* tranfer_buf = (management_frame_Info*)malloc(sizeof(management_frame_Info));
	memcpy(tranfer_buf,Info,sizeof(management_frame_Info));
	if(Info->subtype > 13 || Info->subtype == 7 || Info->subtype == 8 || 
		Info->subtype == 11 || Info->subtype == 12){
		zlog_error(g_air_frame->log_handler,"error ! --- error subtype : Info->subtype = %d \n", Info->subtype);
	}else{
		postCallSendWorkToThreadPool(tranfer_buf, g_air_frame);
	}
}


void gw_poll_receive(g_air_frame_para* g_air_frame){
	int status;
	management_frame_Info* temp_Info = new_air_frame(-1,0,NULL,NULL,NULL,0);
	while(1){ // wait for air_signal
		int stat = gw_monitor_poll(temp_Info, 2, g_air_frame->g_frame,  g_air_frame->log_handler); // receive 2 * 5ms
		if(stat == 26){
			zlog_info(g_air_frame->log_handler,"receive new air frame \n");
			dispatch_recived_air_frame(temp_Info, g_air_frame);
		}else if(stat < 26){
			;//zlog_error(g_air_frame->log_handler,"error ! --- poll timeout , stat = %d \n" , stat);
		}
	}
	free(temp_Info);
}

void* process_air_thread(void* args){
	g_air_frame_para* g_air_frame = (g_air_frame_para*)args;

	pthread_mutex_lock(g_air_frame->para_t->mutex_);
    while(1){
		while (g_air_frame->running == -1)
		{
			zlog_info(g_air_frame->log_handler,"process_air_thread() : wait for condition\n");
			pthread_cond_wait(g_air_frame->para_t->cond_, g_air_frame->para_t->mutex_);
		}
		pthread_mutex_unlock(g_air_frame->para_t->mutex_);
    	gw_poll_receive(g_air_frame);
    }
    zlog_info(g_air_frame->log_handler,"Exit process_air_thread()");

}

int initProcessAirThread(g_air_frame_para** g_air_frame, ThreadPool* g_threadpool, zlog_category_t* handler){
	*g_air_frame = (g_air_frame_para*)malloc(sizeof(struct g_air_frame_para));
	(*g_air_frame)->log_handler = handler;
	(*g_air_frame)->para_t = newThreadPara();
	(*g_air_frame)->send_para_t = newThreadPara();
	(*g_air_frame)->send_id = 0;
	(*g_air_frame)->running = -1;
	(*g_air_frame)->g_frame = (g_args_frame*)malloc(sizeof(struct g_args_frame));
	(*g_air_frame)->g_threadpool = g_threadpool;

	int ret = pthread_create((*g_air_frame)->para_t->thread_pid, NULL, process_air_thread, (void*)(*g_air_frame));
    if(ret != 0){
        zlog_error(handler,"error ! --- create monitor_thread error ! error_code = %d", ret);
		return -1;
    }
	
	int fd = ManagementFrame_create_monitor_interface((*g_air_frame)->g_frame);
	if(fd < 0){
		zlog_error(handler,"error ! --- ManagementFrame_create_monitor_interface : fd = %d \n" , fd);
		return fd;
	}

	return 0;
}

int freeProcessAirThread(g_air_frame_para* g_air_frame){
	zlog_info(g_air_frame->log_handler,"freeProcessAirThread()");
	free(g_air_frame);
}

void startProcessAir(g_air_frame_para* g_air_frame, int running_step){
	g_air_frame->running = running_step;
	pthread_cond_signal(g_air_frame->para_t->cond_);
}

void print_subtype(int32_t subtype, g_air_frame_para* g_air_frame){
	if(subtype == 0)
		zlog_info(g_air_frame->log_handler,"send_airSignal :  subtype = BEACON , send successful \n");
	else if(subtype == 1)
		zlog_info(g_air_frame->log_handler,"send_airSignal :  subtype = ASSOCIATION_REQUEST , send successful \n");
	else if(subtype == 2)
		zlog_info(g_air_frame->log_handler,"send_airSignal :  subtype = ASSOCIATION_RESPONSE , send successful \n");
	else if(subtype == 3)
		zlog_info(g_air_frame->log_handler,"send_airSignal :  subtype = REASSOCIATION , send successful \n");
	else if(subtype == 4)
		zlog_info(g_air_frame->log_handler,"send_airSignal :  subtype = DEASSOCIATION , send successful \n");
	else if(subtype == 5)
		zlog_info(g_air_frame->log_handler,"send_airSignal :  subtype = HANDOVER_START_REQUEST , send successful \n");
	else if(subtype == 6)
		zlog_info(g_air_frame->log_handler,"send_airSignal :  subtype = HANDOVER_START_RESPONSE , send successful \n");
	else if(subtype == DISTANC_MEASURE_REQUEST)
		zlog_info(g_air_frame->log_handler,"send_airSignal :  subtype = DISTANC_MEASURE_REQUEST , send successful \n");
	else if(subtype == DELAY_EXCHANGE_REQUEST)
		zlog_info(g_air_frame->log_handler,"send_airSignal :  subtype = DELAY_EXCHANGE_REQUEST , send successful \n");
	else if(subtype == DELAY_EXCHANGE_RESPONSE)
		zlog_info(g_air_frame->log_handler,"send_airSignal :  subtype = DELAY_EXCHANGE_RESPONSE , send successful \n");
}

/* ipc receive call --- thread safe?*/ 
int send_airFrame(char* buf, int buf_len, g_ipc_server_para* g_ipc_server_var){

	struct g_air_frame_para* g_air_frame = NULL;	
	g_air_frame = container_of(g_ipc_server_var, g_air_frame_para, g_ipc_server);

	pthread_mutex_lock(g_air_frame->send_para_t->mutex_);
	g_air_frame->send_id = g_air_frame->send_id + 1;
	
	int status;
	assert(buf_len == sizeof(management_frame_Info));
	management_frame_Info* frame_Info = (management_frame_Info*)buf;
	int subtype = frame_Info->subtype;
	status = handle_air_tx(frame_Info,g_air_frame->g_frame, g_air_frame->log_handler);

	pthread_mutex_unlock(g_air_frame->send_para_t->mutex_);

	if(status == 26){ // send success
		print_subtype(subtype,g_air_frame);
	}else if(status < 26){
		zlog_error(g_air_frame->log_handler,"error ! --- air_tx,status = %d , subtype = %d \n" , status, subtype);
	}
	return status;
}