#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

#include <math.h>

#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include "gw_utility.h"
#include "gw_control.h"
#include "gw_frame.h"
#include "msg_queue.h"
#include "zlog.h"
#include "regdev_common.h"
#include "ThreadPool.h"

#include "cJSON.h"
#include "common.h"

global_var_t g_var_value;


zlog_category_t * serverLog(const char* path){
	int rc;
	zlog_category_t *zlog_handler = NULL;
	rc = zlog_init(path);

	if (rc) {
		return NULL;
	}

	zlog_handler = zlog_get_category("distance");

	if (!zlog_handler) {
		zlog_fini();
		return NULL;
	}

	return zlog_handler;
}

void closeServerLog(){
	zlog_fini();
}

/*  ----------------------------------------------------------------------------------  */
struct ConfigureNode* init_node(zlog_category_t* log_handler){
	struct ConfigureNode* clientConfigure = (struct ConfigureNode*)malloc(sizeof(struct ConfigureNode));

	clientConfigure->my_id = 0;
	clientConfigure->my_mac_str = (char*)malloc(32);
	clientConfigure->my_Ethernet = (char*)malloc(32);
	clientConfigure->measure_cnt_ms = 0;

//  init system global variable
	clientConfigure->system_info = (struct system_info_para*)malloc(sizeof(struct system_info_para));
	clientConfigure->system_info->have_ve_mac = 0;
	memset(clientConfigure->system_info->ve_mac,0,6);
	memset(clientConfigure->system_info->bs_mac,0,6);
	memset(clientConfigure->system_info->next_bs_mac,0,6); // for MSG_START_HANDOVER_THROUGH_AIR

	clientConfigure->system_info->send_id = 0;
	clientConfigure->system_info->rcv_id = 0;

	clientConfigure->system_info->my_initial    = 0;
	clientConfigure->system_info->other_initial = 0;
	clientConfigure->system_info->have_my_initial    = 0;
	clientConfigure->system_info->have_other_initial = 0;
	clientConfigure->system_info->system_state = 0;

	clientConfigure->system_info->received_air_state_list = (struct received_state_list*)malloc(sizeof(struct received_state_list));
	clientConfigure->system_info->received_air_state_list->received_delay_exchange_response = 0;


	const char* configure_path = "../conf/test_conf.json";
	char* pConfigure_file = readfile(configure_path);
	if(pConfigure_file == NULL){
		zlog_error(log_handler,"open file %s error.\n",configure_path);
		return clientConfigure;
	}
	cJSON * root = NULL;
    cJSON * item = NULL;
    root = cJSON_Parse(pConfigure_file);
	free(pConfigure_file);     
    if (!root){
        zlog_error(log_handler,"Error before: [%s]",cJSON_GetErrorPtr());
    }else{
		item = cJSON_GetObjectItem(root,"my_id");
		clientConfigure->my_id = item->valueint;
		item = cJSON_GetObjectItem(root, "my_Ethernet");
		memcpy(clientConfigure->my_Ethernet,item->valuestring,strlen(item->valuestring)+1);
		
		item = cJSON_GetObjectItem(root, "measure_cnt_ms");
		clientConfigure->measure_cnt_ms = item->valueint;

		cJSON_Delete(root);
    }

	//get mac addr
	// int	nRtn = get_mac(clientConfigure->my_mac_str, 32, clientConfigure->my_Ethernet);
    // if(nRtn > 0) // nRtn = 12
    // {	
    //     printf("nRtn = %d , MAC ADDR : %s\n", nRtn,clientConfigure->my_mac_str);
	// 	change_mac_buf(clientConfigure->my_mac_str,clientConfigure->system_info->bs_mac); // 0408 ---- bug
    // }else{
	// 	printf("get mac address failed!\n");
	// }

	zlog_info(log_handler," configure : measure_cnt_ms = %d  " , clientConfigure->measure_cnt_ms);

	return clientConfigure;
}

/* send main thread */

void print_subtype(int32_t subtype, g_air_para* g_air){
	if(subtype == 9)
		zlog_info(g_air->log_handler,"send_airSignal :  subtype = DISTANC_MEASURE_REQUEST , send successful \n");
	else if(subtype == 13)
		zlog_info(g_air->log_handler,"send_airSignal :  subtype = DELAY_EXCHANGE_RESPONSE , send successful \n");
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

void self_test_send(g_air_para* g_air){
	system_info_para* g_system_info = g_air->node->system_info;
	 // send DISTANC_MEASURE_REQUEST
	zlog_info(g_air->log_handler,"send DISTANC_MEASURE_REQUEST self_test_send \n");
	for(int i = 0; i < 10 ; i++){
		send_airSignal(DISTANC_MEASURE_REQUEST, g_system_info->bs_mac, g_system_info->ve_mac, g_system_info->bs_mac, g_air);
		usleep(100000);
	}	
}

// void* periodic_distance_measure_send(void* arg){
// 	g_air_para* g_air = (g_air_para*)arg;
// 	system_info_para* g_system_info = g_air->node->system_info;
// 	 // send DISTANC_MEASURE_REQUEST
// 	zlog_info(g_air->log_handler,"send DISTANC_MEASURE_REQUEST periodic \n");
// 	for(;;){
// 		send_airSignal(DISTANC_MEASURE_REQUEST, g_system_info->bs_mac, g_system_info->ve_mac, g_system_info->bs_mac, g_air);

// 		struct msg_st data;
// 		data.msg_len = 0;
// 		data.msg_type = MSG_CACULATE_DISTANCE;
// 		data.msg_number = MSG_CACULATE_DISTANCE;
// 		postMsgQueue(&data,g_air->g_msg_queue);

// 		//usleep(1000000);

// 		for(int i = 0;i<g_air->node->measure_cnt_ms;i++){
// 			usleep(1000);
// 		}
// 	}
// }

// void postDistanceMeasureWorkToThreadPool(g_air_para* g_air, ThreadPool* g_threadpool){
// 	AddWorker(periodic_distance_measure_send,(void*)g_air,g_threadpool);
// } 

/* receive thread */
struct air_data* bufTojson(management_frame_Info* Info, g_air_para* g_air){
	struct air_data* tmp_data = (struct air_data*)malloc(sizeof(struct air_data));

	tmp_data->seq_id = Info->seq_id;
	memcpy(tmp_data->source_mac_addr,Info->source_mac_addr,6);
	memcpy(tmp_data->dest_mac_addr,Info->dest_mac_addr,6);
	memcpy(tmp_data->Next_dest_mac_addr,Info->Next_dest_mac_addr,6);

	return tmp_data;
}

void process_air_signal(management_frame_Info* Info, g_air_para* g_air){
	struct msg_st data;
	struct air_data* json_buf = bufTojson(Info,g_air);
	data.msg_len = sizeof(struct air_data);
	memcpy(data.msg_json, (char*)json_buf, data.msg_len);
	switch(Info->subtype){
		case DISTANC_MEASURE_REQUEST:
		{
			data.msg_type = MSG_RECEIVED_DISTANC_MEASURE_REQUEST;
			data.msg_number = MSG_RECEIVED_DISTANC_MEASURE_REQUEST;
			postMsgQueue(&data,g_air->g_msg_queue);
			break;
		}
		case DELAY_EXCHANGE_REQUEST:
		{
			data.msg_type = MSG_RECEIVED_DELAY_EXCHANGE_REQUEST;
			data.msg_number = MSG_RECEIVED_DELAY_EXCHANGE_REQUEST;
			postMsgQueue(&data,g_air->g_msg_queue);
			break;
		}
		case DELAY_EXCHANGE_RESPONSE:
		{
			data.msg_type = MSG_RECEIVED_DELAY_EXCHANGE_RESPONSE;
			data.msg_number = MSG_RECEIVED_DELAY_EXCHANGE_RESPONSE;
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


void gw_poll_receive_test(g_air_para* g_air){
	zlog_info(g_air->log_handler,"start gw_poll_receive .......  \n");
	int status;
	management_frame_Info* temp_Info = new_air_frame(-1,0,NULL,NULL,NULL,0);
	while(1){ // wait for air_signal
		int stat = gw_monitor_poll(temp_Info, 2, g_air->log_handler); // receive 2 * 5ms
		if(stat == 26){
			zlog_info(g_air->log_handler,"receive new air frame \n");
			process_air_signal(temp_Info, g_air);
		}else if(stat < 26){
			//zlog_error(g_air->log_handler,"error ! --- poll timeout , stat = %d \n" , stat);
		}
	}
	free(temp_Info);
}

void* process_air_test_thread(void* args){
	g_air_para* g_air = (g_air_para*)args;

	pthread_mutex_lock(g_air->para_t->mutex_);
    while(1){
		while (g_air->systemIsReady == -1 )
		{
			zlog_info(g_air->log_handler,"process_air_thread() : wait for condition\n");
			pthread_cond_wait(g_air->para_t->cond_, g_air->para_t->mutex_);
		}
		pthread_mutex_unlock(g_air->para_t->mutex_);
    	gw_poll_receive_test(g_air); // 1. gw_poll_receive() 2. process_air()
    }
    zlog_info(g_air->log_handler,"Exit process_air_test_thread()");

}

void startReceviedAir(g_air_para* g_air){
	zlog_info(g_air->log_handler,"startReceviedAir\n");
	g_air->systemIsReady = 1;
	pthread_cond_signal(g_air->para_t->cond_);
}

int initTestAirThread(struct ConfigureNode* Node, g_air_para** g_air, g_msg_queue_para* g_msg_queue, zlog_category_t* handler){
	zlog_info(handler,"initTestAirThread()");
	*g_air = (g_air_para*)malloc(sizeof(struct g_air_para));
	(*g_air)->log_handler = handler;
	(*g_air)->para_t = newThreadPara();
	(*g_air)->send_para_t = newThreadPara();
	(*g_air)->systemIsReady = -1;
	(*g_air)->node = Node;
	(*g_air)->g_msg_queue = g_msg_queue;


	int ret = pthread_create((*g_air)->para_t->thread_pid, NULL, process_air_test_thread, (void*)(*g_air));
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

void Enter_self_test(g_air_para* g_air , g_RegDev_para* g_RegDev){
	system("sh ../conf/open_self.sh");

	zlog_info(g_air->log_handler,"open_self");

	self_test_send(g_air);

	g_air->node->system_info->my_initial = get_delay_tick(g_RegDev);
	g_air->node->system_info->have_my_initial = 1;

	system("sh ../conf/close_self.sh");

	zlog_info(g_air->log_handler,"close_self, my_initial = %u", g_air->node->system_info->my_initial);
}

/* ------- retransmit air signal ---------------- */
void postCheckSendSignal(int32_t subtype, g_msg_queue_para* g_msg_queue){
	struct msg_st data;
	data.msg_type = MSG_CHECK_RECEIVED_LIST;
	data.msg_number = MSG_CHECK_RECEIVED_LIST;
	data.msg_len = sizeof(int32_t);
	memcpy(data.msg_json, (char*)(&subtype), data.msg_len);
	postMsgQueue(&data,g_msg_queue);
}

void resetReceivedList(received_state_list* list){
	list->received_delay_exchange_response = 0;
}

void* retrans_air_process_thread(void* arg){
	retrans_air_t* tmp = (retrans_air_t*)arg;
	int32_t subtype = tmp->subtype;
	if(subtype == DELAY_EXCHANGE_REQUEST)
		sleep(2);
	else if(subtype == DISTANC_MEASURE_REQUEST){
		for(int i = 0;i<tmp->g_air->node->measure_cnt_ms;i++){
			usleep(1000);
		}
	}
	postCheckSendSignal(tmp->subtype, tmp->g_msg_queue);
	free(tmp);
}

void postCheckWorkToThreadPool(int32_t subtype, g_msg_queue_para* g_msg_queue, ThreadPool* g_threadpool, g_air_para* g_air){
	retrans_air_t* tmp = (retrans_air_t*)malloc(sizeof(retrans_air_t));
	tmp->subtype = subtype;
	tmp->g_msg_queue = g_msg_queue;
	tmp->g_air = g_air;
	AddWorker(retrans_air_process_thread,(void*)tmp,g_threadpool);
} 

void checkReceivedList(int32_t subtype, system_info_para* g_system_info, g_msg_queue_para* g_msg_queue, 
					  g_air_para* g_air, ThreadPool* g_threadpool){
	received_state_list* list = g_system_info->received_air_state_list;
	if(g_system_info->system_state == 0)
		return;
	if(subtype == DELAY_EXCHANGE_REQUEST){
		if(list->received_delay_exchange_response == 0){
			char my_inital[6];
			memset(my_inital,0x0,6);
			memcpy(my_inital,(char*)(&(g_system_info->my_initial)), sizeof(uint32_t));
			send_airSignal(DELAY_EXCHANGE_REQUEST, g_system_info->bs_mac, g_system_info->ve_mac, my_inital, g_air);
			postCheckWorkToThreadPool(subtype, g_msg_queue, g_threadpool, g_air);
		}
	}else if(subtype == DISTANC_MEASURE_REQUEST){
		if(g_system_info->have_my_initial == 1){
			send_airSignal(DISTANC_MEASURE_REQUEST, g_system_info->bs_mac, g_system_info->ve_mac, g_system_info->bs_mac, g_air);
			
			struct msg_st data;
			data.msg_len = 0;
			data.msg_type = MSG_CACULATE_DISTANCE;
			data.msg_number = MSG_CACULATE_DISTANCE;
			postMsgQueue(&data,g_air->g_msg_queue);
			
			postCheckWorkToThreadPool(subtype, g_msg_queue, g_threadpool, g_air);
		}
	}
}


/* ---------------------------------------------- */

struct air_data* parseAirMsg(char* msg_json){
	struct air_data* tmp_data = (struct air_data*)malloc(sizeof(struct air_data));
	memcpy(tmp_data,msg_json,sizeof(struct air_data));
	return tmp_data;
}

void process_event(struct msg_st* getData, g_air_para* g_air, 
                   g_RegDev_para* g_RegDev, g_msg_queue_para* g_msg_queue, 
				   ThreadPool* g_threadpool, zlog_category_t* zlog_handler)
{
	system_info_para* g_system_info = g_air->node->system_info;
	switch(getData->msg_type){
		case MSG_CHECK_RECEIVED_LIST:
		{
			int32_t subtype = -1;
			memcpy((char*)(&subtype), getData->msg_json, getData->msg_len);
			if(subtype == DELAY_EXCHANGE_REQUEST)
				zlog_info(zlog_handler," --- DELAY_EXCHANGE_REQUEST ------- EVENT : MSG_CHECK_RECEIVED_LIST: msg_number = %d", getData->msg_number);
			// check receive list according to subtype
			checkReceivedList(subtype, g_system_info, g_msg_queue, g_air, g_threadpool);
			break;
		}
		case MSG_RECEIVED_DISTANC_MEASURE_REQUEST:
		{
			//zlog_info(zlog_handler," ---------------- EVENT : MSG_RECEIVED_DISTANC_MEASURE_REQUEST: msg_number = %d ", getData->msg_number);
			break;
		}
		case MSG_RECEIVED_DELAY_EXCHANGE_RESPONSE: // my_inital and other_inital exchange completed 
		{
			zlog_info(zlog_handler," ---------------- EVENT : MSG_RECEIVED_DELAY_EXCHANGE_RESPONSE: msg_number = %d ", getData->msg_number);
			
			received_state_list* list = g_system_info->received_air_state_list;
			if(list->received_delay_exchange_response == 1)
				break;
			list->received_delay_exchange_response = 1;

			struct air_data* air_msg = parseAirMsg(getData->msg_json);
			uint32_t other_inital = 0;
			memcpy((char*)(&other_inital), air_msg->Next_dest_mac_addr, sizeof(uint32_t));
			g_system_info->other_initial = other_inital;
			g_system_info->have_other_initial = 1;
			printf("other_inital = %d \n", g_system_info->other_initial);
			free(air_msg);

			uint32_t delay =(g_system_info->my_initial + g_system_info->other_initial)/2+32;
			zlog_info(zlog_handler,"receive exchange response : other_inital = %d , new delay = %u \n", g_system_info->other_initial, delay);
			set_delay_tick(g_RegDev,delay);

			zlog_info(zlog_handler,"Start Send Distance Measure Air Frame .......................................... \n");

			/* test enterance !!!!!!! */
			//postDistanceMeasureWorkToThreadPool(g_air,g_threadpool); 
			send_airSignal(DISTANC_MEASURE_REQUEST, g_system_info->bs_mac, g_system_info->ve_mac, g_system_info->bs_mac, g_air);
			postCheckWorkToThreadPool(DISTANC_MEASURE_REQUEST, g_msg_queue, g_threadpool, g_air);
			break;
		}
		case MSG_RECEIVED_DELAY_EXCHANGE_REQUEST:
		{
			zlog_info(zlog_handler," ---------------- EVENT : MSG_RECEIVED_DELAY_EXCHANGE_REQUEST: msg_number = %d ", getData->msg_number);
			
			struct air_data* air_msg = parseAirMsg(getData->msg_json);
			uint32_t other_inital = 0;
			memcpy((char*)(&other_inital), air_msg->Next_dest_mac_addr, sizeof(uint32_t));
			g_system_info->other_initial = other_inital;
			g_system_info->have_other_initial = 1;
			uint32_t delay =(g_system_info->my_initial + g_system_info->other_initial)/2+32;
			zlog_info(zlog_handler,"receive exchange request : other_inital = %d , new delay = %u \n",g_system_info->other_initial, delay);
			set_delay_tick(g_RegDev,delay);
			
			char my_inital[6];
			memset(my_inital,0x0,6);
			memcpy(my_inital,(char*)(&(g_system_info->my_initial)), sizeof(uint32_t));
			send_airSignal(DELAY_EXCHANGE_RESPONSE, g_system_info->bs_mac, g_system_info->ve_mac, my_inital, g_air);
			break;
		}
		case MSG_CACULATE_DISTANCE:
		{
			//zlog_info(zlog_handler," ---------------- EVENT : MSG_CACULATE_DISTANCE: msg_number = %d ", getData->msg_number);

			usleep(10000);

			uint32_t delay =(g_system_info->my_initial +g_system_info->other_initial)/2+32;

			uint32_t current_tick = get_delay_tick(g_RegDev);

			double distance = (current_tick - delay) * 0.149896229;

			zlog_info(zlog_handler,"delay = %u, current_tick = %u , distance = %f " , delay, current_tick,distance);

			break;
		}
		case MSG_SYSTEM_EXCEPTION:
		{
			zlog_info(zlog_handler," ---------------- EVENT : MSG_SYSTEM_EXCEPTION: msg_number = %d ", getData->msg_number);

			if(g_system_info->system_state == 1){
				g_system_info->system_state = 0;
				g_system_info->have_my_initial = 0;
				zlog_info(zlog_handler," system state is set to 0 ! \n ");
			}else{
				zlog_info(zlog_handler," system state is already in 0 ! \n ");
			}

			break;
		}
		case MSG_SYSTEM_READY:
		{
			zlog_info(zlog_handler," ---------------- EVENT : MSG_SYSTEM_READY: msg_number = %d ", getData->msg_number);

			if(g_system_info->system_state == 0){
				Enter_self_test(g_air,g_RegDev);
				g_system_info->system_state = 1;

				received_state_list* list = g_system_info->received_air_state_list;
				if(list->received_delay_exchange_response == 1)
					list->received_delay_exchange_response = 0;

				char my_inital[6];
				memset(my_inital,0x0,6);
				memcpy(my_inital,(char*)(&(g_system_info->my_initial)), sizeof(uint32_t));
				send_airSignal(DELAY_EXCHANGE_REQUEST, g_system_info->bs_mac, g_system_info->ve_mac, my_inital, g_air);
				postCheckWorkToThreadPool(DELAY_EXCHANGE_REQUEST, g_msg_queue, g_threadpool, g_air);

				zlog_info(zlog_handler," system state is set to 1 ! \n ");

			}else{
				zlog_info(zlog_handler," system state is already in 1 ! \n");
			}

			break;
		}
		default:
			break;
	}
}


void distance_eventLoop(g_air_para* g_air, g_msg_queue_para* g_msg_queue, 
               g_RegDev_para* g_RegDev, ThreadPool* g_threadpool, zlog_category_t* zlog_handler)
{
	system_info_para* g_system_info = g_air->node->system_info;
	g_system_info->system_state = 1;
	char my_inital[6];
	memset(my_inital,0x0,6);
	memcpy(my_inital,(char*)(&(g_system_info->my_initial)), sizeof(uint32_t));
	send_airSignal(DELAY_EXCHANGE_REQUEST, g_system_info->bs_mac, g_system_info->ve_mac, my_inital, g_air);
	postCheckWorkToThreadPool(DELAY_EXCHANGE_REQUEST, g_msg_queue, g_threadpool, g_air);

	zlog_info(zlog_handler," ------------------------------  start distance_eventLoop ----------------------------- \n");

	while(g_var_value.program_run == 1){
		struct msg_st* getData = getMsgQueue(g_msg_queue);
		if(getData == NULL)
			continue;

		process_event(getData, g_air, g_RegDev, g_msg_queue, g_threadpool, zlog_handler);

		free(getData);
	}
}

// broker callback interface
int test_exception(char* buf, int buf_len, char *from, void* arg)
{
	int ret = 0;
	cJSON * root = NULL;
    cJSON * item = NULL;
    root = cJSON_Parse(buf);
    item = cJSON_GetObjectItem(root,"stat");
	if(strcmp(from,"mon/all/pub/system_stat") == 0){
		printf("system_stat is %s \n" , buf);
		if(strcmp(item->valuestring,"0x20") == 0 || strcmp(item->valuestring,"0x80000020") == 0){
			struct msg_st data;
			data.msg_len = 0;
			data.msg_type = MSG_SYSTEM_READY;
			data.msg_number = MSG_SYSTEM_READY;
			postMsgQueue(&data,g_var_value.g_msg_queue);

		}else{
			struct msg_st data;
			data.msg_len = 0;
			data.msg_type = MSG_SYSTEM_EXCEPTION;
			data.msg_number = MSG_SYSTEM_EXCEPTION;
			postMsgQueue(&data,g_var_value.g_msg_queue);
		}
	}
	return ret;
}

int init_program(){
	g_var_value.g_msg_queue = NULL;
	g_var_value.log_handler = NULL;
	g_var_value.program_run = 0;
	int value = gpio_read(973);
	while(value != 1){
		value = gpio_read(973);
	}
	return value;
}


int main(int argc, char *argv[]){

	zlog_category_t *zlog_handler = serverLog("../conf/zlog_default.conf");
	/* init distance measure request receive */

	fflush(stdout);
    setvbuf(stdout, NULL, _IONBF, 0);

	init_program();

	zlog_info(zlog_handler,"***************** continue start program ******************** \n ");

	struct ConfigureNode* Node = init_node(zlog_handler);

	/* reg dev */
	g_RegDev_para* g_RegDev = NULL;
	int state = initRegdev(&g_RegDev, zlog_handler);
	if(state != 0 ){
		zlog_info(zlog_handler,"initRegdev create failed !");
		return 0;
	}

	/* msg_queue */
	const char* pro_path = "/tmp/handover_test/";
	int proj_id = 'c';
	g_msg_queue_para* g_msg_queue = createMsgQueue(pro_path, proj_id, zlog_handler);
	if(g_msg_queue == NULL){
		zlog_info(zlog_handler,"No msg_queue created \n");
		return 0;
	}
	zlog_info(zlog_handler, "g_msg_queue->msgid = %d \n", g_msg_queue->msgid);

	state = clearMsgQueue(g_msg_queue);

	g_air_para* g_air = NULL;
	state = initTestAirThread(Node, &g_air, g_msg_queue, zlog_handler);
	if(state != 0){
		printf("initTestAirThread : state = %d \n", state);
		return 0;
	}

	/* broker gw_control */
	g_var_value.g_msg_queue = g_msg_queue;
	g_var_value.log_handler = zlog_handler;
	g_var_value.program_run = 1;
	int ret = initBroker(argv[0],test_exception);
	zlog_info(zlog_handler,"initBroker : ret = %d \n", ret);

	/* ThreadPool handler */
	ThreadPool* g_threadpool = NULL;
	createThreadPool(4096, 8, &g_threadpool); // 4096 , 8

	gw_sleep();

	startReceviedAir(g_air);

	Enter_self_test(g_air,g_RegDev);

	zlog_info(zlog_handler, "Enter distance event loop \n");

	distance_eventLoop(g_air, g_msg_queue, g_RegDev, g_threadpool, zlog_handler);

	zlog_info(zlog_handler,"Exit Distance Measure Process \n");

}











