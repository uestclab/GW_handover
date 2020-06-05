#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <fcntl.h>
#include <sys/shm.h>

#include "bs_event_process.h"
#include "bs_network.h"
#include "bs_monitor.h"
#include "bs_air.h"
#include "bs_x2.h"
#include "msg_queue.h"
#include "define_common.h"
#include "zlog.h"

#include "cJSON.h"
#include "gw_utility.h"
#include "gw_control.h"
#include "ThreadPool.h"


zlog_category_t * serverLog(const char* path){
	int rc;
	zlog_category_t *zlog_handler = NULL;
	rc = zlog_init(path);

	if (rc) {
		return NULL;
	}

	zlog_handler = zlog_get_category("baseStation");

	if (!zlog_handler) {
		zlog_fini();
		return NULL;
	}

	return zlog_handler;
}

void closeServerLog(){
	zlog_fini();
}

struct ConfigureNode* configure(zlog_category_t* log_handler){
	struct ConfigureNode* Node = (struct ConfigureNode*)malloc(sizeof(struct ConfigureNode));
	const char* configure_ip = "127.0.0.1";
	Node->server_ip = (char*)malloc(16);
	memcpy(Node->server_ip, configure_ip, strlen(configure_ip)+1);
	Node->server_port = 44445;
	Node->my_id = 0;
	Node->my_mac_str = (char*)malloc(32);
	Node->my_Ethernet = (char*)malloc(32);
	Node->enable_user_wait = 0; //
	Node->sleep_cnt_second = 0;
	Node->check_eth_rx_cnt = 0;
	Node->delay_mon_cnt_second = 0;
	// x2 interface 
	Node->udp_server_ip    = (char*)malloc(32);
	Node->udp_server_port  = 60000;
	// thread pool
	Node->task_queue_size = 0;
	Node->threads_num = 0;
	

//  init system global variable
	Node->system_info = (struct system_info_para*)malloc(sizeof(struct system_info_para));
	Node->system_info->bs_state = STATE_STARTUP;
	Node->system_info->have_ve_mac = 0;
	Node->system_info->ve_id = 0;
	memset(Node->system_info->ve_mac,0,6);
	memset(Node->system_info->bs_mac,0,6);
	memset(Node->system_info->next_bs_mac,0,6); // for MSG_START_HANDOVER_THROUGH_AIR
	Node->system_info->monitored = 0;
	Node->system_info->handover_cnt = 0;
	Node->system_info->sourceBs_dac_disabled = 0;
	Node->system_info->received_reassociation = 0;

	Node->system_info->send_id = 0;
	Node->system_info->rcv_id = 0;

	Node->system_info->received_air_state_list = (struct received_state_list*)malloc(sizeof(struct received_state_list));
	Node->system_info->received_air_state_list->received_association_response = 0;
	Node->system_info->received_air_state_list->received_handover_start_response = 0;
	Node->system_info->received_air_state_list->received_reassociation = 0;
	Node->system_info->received_air_state_list->received_keepAlive = 0;

	Node->system_info->received_network_state_list = (struct received_network_list*)malloc(sizeof(struct received_network_list));
	Node->system_info->received_network_state_list->received_dac_closed_x2_ack = 0;
	Node->system_info->received_network_state_list->received_dac_closed_x2 = 0;

	// // -------- init tick
	Node->system_info->my_initial = 0;
	Node->system_info->other_initial = 0;
	Node->system_info->have_my_initial = 0;
	Node->system_info->have_other_initial = 0;
	Node->distance_measure_cnt_ms = 0;
	Node->distance_threshold = 0;
	Node->snr_threshold = 0;

	// -------- handover start confirm
	Node->system_info->distance_near_confirm = 0;
	Node->system_info->received_handover_start_confirm = 0;

// 
	const char* configure_path = "../conf/bs_conf.json";
	char* pConfigure_file = readfile(configure_path);
	if(pConfigure_file == NULL){
		zlog_error(log_handler,"open file %s error.\n",configure_path);
		return Node;
	}
	cJSON * root = NULL;
    cJSON * item = NULL;
    root = cJSON_Parse(pConfigure_file);
	free(pConfigure_file);     
    if (!root){
        zlog_error(log_handler,"Error before: [%s]",cJSON_GetErrorPtr());
    }else{
        item = cJSON_GetObjectItem(root, "server_ip");
		memcpy(Node->server_ip,item->valuestring,strlen(item->valuestring)+1);
        item = cJSON_GetObjectItem(root, "server_port");
		Node->server_port = item->valueint;
		item = cJSON_GetObjectItem(root,"my_id");
		Node->my_id = item->valueint;
		item = cJSON_GetObjectItem(root, "my_Ethernet");
		memcpy(Node->my_Ethernet,item->valuestring,strlen(item->valuestring)+1);
		
		item = cJSON_GetObjectItem(root, "enable_user_wait");
		Node->enable_user_wait = item->valueint;
		item = cJSON_GetObjectItem(root, "sleep_cnt_second");
		Node->sleep_cnt_second = item->valueint;
        item = cJSON_GetObjectItem(root, "check_eth_rx_cnt");
		Node->check_eth_rx_cnt = item->valueint;

		item = cJSON_GetObjectItem(root, "delay_mon_cnt_second");
		Node->delay_mon_cnt_second = item->valueint;

        item = cJSON_GetObjectItem(root, "udp_server_port");
		Node->udp_server_port = item->valueint;

		item = cJSON_GetObjectItem(root, "task_queue_size");
		Node->task_queue_size = item->valueint;

        item = cJSON_GetObjectItem(root, "threads_num");
		Node->threads_num = item->valueint;

		item = cJSON_GetObjectItem(root, "distance_measure_cnt_ms");
		Node->distance_measure_cnt_ms = item->valueint;

		item = cJSON_GetObjectItem(root, "distance_threshold");
		Node->distance_threshold = item->valueint;

		item = cJSON_GetObjectItem(root, "snr_threshold");
		Node->snr_threshold = item->valueint;

		cJSON_Delete(root);
    }

	//get mac addr
	int	nRtn = get_mac(Node->my_mac_str, 32, Node->my_Ethernet);
    if(nRtn > 0) // nRtn = 12
    {	
        printf("nRtn = %d , MAC ADDR : %s\n", nRtn,Node->my_mac_str);
		change_mac_buf(Node->my_mac_str,Node->system_info->bs_mac); // 0408 ---- bug
    }else{
		printf("get mac address failed!\n");
	}

	int ret = get_ip(Node->udp_server_ip, Node->my_Ethernet);
	if(ret == 0)
    {	
		printf("x2 interface : udp_server_ip = %s , udp_server_port = %d \n", Node->udp_server_ip, Node->udp_server_port);
    }else{
		printf("get mac address failed!\n");
	}


	zlog_info(log_handler," configure : enable_user_wait = %d , sleep_cnt_second = %d , check_eth_rx_cnt = %d " , Node->enable_user_wait , Node->sleep_cnt_second , Node->check_eth_rx_cnt);

	return Node;
}

int init_program(){
	int value = gpio_read(973);
	while(value != 1){
		sleep(2);
		value = gpio_read(973);
	}
	return value;
}

// broker callback interface
int process_exception(char* buf, int buf_len, char *from, void* arg)
{
	int ret = 0;
	if(strcmp(from,"mon/all/pub/system_stat") == 0){
		printf("system_stat is %s \n" , buf);
	}
	return ret;
}

int main(int argc, char *argv[]) // main thread
{
	if(argc == 2){
		if(argv[1] != NULL){
			if(strcmp(argv[1],"-v") == 0)
				printf("version : [%s %s] \n",__DATE__,__TIME__);
			else
				printf("error parameter\n");
			return 0;
		}
	}

	fflush(stdout);
    setvbuf(stdout, NULL, _IONBF, 0);
	
	zlog_category_t *zlog_handler = serverLog("../conf/zlog_default.conf");

	//init_program();
	zlog_info(zlog_handler," +++++++++++++++++++++++++++++ start baseStation ++++++++++++++++++++++++++++++++++++++++++++++ \n");


	struct ConfigureNode* Node = configure(zlog_handler);
	if(Node == NULL){
		printf("Node == NULL \n");
		return 0;
	}

	/* reg dev */
	g_RegDev_para* g_RegDev = NULL;
	int state = initRegdev(&g_RegDev, zlog_handler);
	if(state != 0 ){
		zlog_info(zlog_handler,"initRegdev create failed !");
		return 0;
	}

	/* broker gw_control */
	int ret = initBroker(argv[0],process_exception);
	zlog_info(zlog_handler,"initBroker : ret = %d \n", ret);
	
	/* msg_queue */
	g_msg_queue_para* g_msg_queue = createMsgQueue();
	if(g_msg_queue == NULL){
		zlog_info(zlog_handler,"No msg_queue created \n");
		return 0;
	}
	zlog_info(zlog_handler, "g_msg_queue->msgid = %d \n", g_msg_queue->msgid);
	
	/* network thread */
	g_network_para* g_network = NULL;
	state = initNetworkThread(Node, &g_network, g_msg_queue, zlog_handler);
	if(state == 1 || state == 2){
		return 0;
	}

	/* air process thread */
	g_air_para* g_air = NULL;
	state = initProcessAirThread(Node, &g_air, g_msg_queue, zlog_handler);
	if(state != 0){
		printf("initProcessAirThread : state = %d \n", state);
		return 0;
	}

	/* x2 interface thread */
	g_x2_para* g_x2 = NULL;
	state = initX2Thread(Node, &g_x2, g_msg_queue, zlog_handler);
	if(state < 0){
		printf("initX2Thread : state = %d \n", state);
		return 0;
	}

	/* ThreadPool handler */
	ThreadPool* g_threadpool = NULL;
	printf("Thread pool parameter : task_queue_size = %d , threads_num = %d \n", Node->task_queue_size, Node->threads_num);
	createThreadPool(Node->task_queue_size, Node->threads_num, &g_threadpool); // 4096 , 8

	gw_sleep();

// ------------------------

	/* msg loop */ /* state machine */
	eventLoop(g_network, g_air, g_x2, g_msg_queue, g_RegDev, g_threadpool, zlog_handler);


	freeNetworkThread(g_network);
	closeServerLog();
	
	//pthread_join(*(g_monitor->para_t->thread_pid),NULL);
	pthread_join(*(g_network->para_t->thread_pid),NULL);
	pthread_join(*(g_air->para_t->thread_pid),NULL);
	pthread_join(*(g_x2->para_t->thread_pid),NULL);

    return 0;
}




