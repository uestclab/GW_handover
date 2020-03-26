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

#include "ve_air.h"
#include "ve_event_process.h"
#include "msg_queue.h"
#include "common.h"
#include "zlog.h"

#include "cJSON.h"
#include "gw_utility.h"
#include "gw_frame.h"
#include "gw_control.h"
#include "ThreadPool.h"

zlog_category_t * serverLog(const char* path){
	int rc;
	zlog_category_t *zlog_handler = NULL;
	rc = zlog_init(path);

	if (rc) {
		return NULL;
	}

	zlog_handler = zlog_get_category("vehicle");

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

	Node->vehicle_id = 0;
	Node->my_mac_str = (char*)malloc(32);
	Node->my_Ethernet = (char*)malloc(32);


//  init system global variable
	Node->system_info = (struct system_info_para*)malloc(sizeof(struct system_info_para));
	Node->system_info->ve_state = STATE_STARTUP;
	Node->system_info->isLinked = 0;
	memset(Node->system_info->ve_mac,0,6);
	memset(Node->system_info->link_bs_mac,0,6);
	memset(Node->system_info->next_bs_mac,0,6);

	Node->system_info->send_id = 0;
	Node->system_info->rcv_id = 0;

	Node->system_info->received_air_state_list = (struct received_state_list*)malloc(sizeof(struct received_state_list));
	Node->system_info->received_air_state_list->received_association_request = 0;
	Node->system_info->received_air_state_list->received_handover_start_request = 0;
	Node->system_info->received_air_state_list->received_deassociation = 0;

	// // -------- init tick
	Node->system_info->my_initial = 0;
	Node->system_info->other_initial = 0;
	Node->system_info->have_my_initial = 0;
	Node->system_info->have_other_initial = 0;
	Node->system_info->new_distance_test_id = 0;
	Node->distance_measure_cnt_ms = 0;
	Node->distance_threshold = 0;


// 
	const char* configure_path = "../conf/ve_conf.json";
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
		item = cJSON_GetObjectItem(root,"vehicle_id");
		Node->vehicle_id = item->valueint;
		item = cJSON_GetObjectItem(root, "my_Ethernet");
		memcpy(Node->my_Ethernet,item->valuestring,strlen(item->valuestring)+1);

		item = cJSON_GetObjectItem(root, "distance_measure_cnt_ms");
		Node->distance_measure_cnt_ms = item->valueint;

		item = cJSON_GetObjectItem(root, "distance_threshold");
		Node->distance_threshold = item->valueint;

		cJSON_Delete(root);
    }

	//get mac addr
	int	nRtn = get_mac(Node->my_mac_str, 32, Node->my_Ethernet);
    if(nRtn > 0) // nRtn = 12
    {	
        printf("nRtn = %d , MAC ADDR : %s\n", nRtn,Node->my_mac_str);
		change_mac_buf(Node->my_mac_str,Node->system_info->ve_mac); // // 0408 ---- bug
    }else{
		printf("get mac address failed!\n");
	}

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

	//zlog_category_t *zlog_handler = serverLog("/run/media/mmcblk1p1/etc/zlog_default.conf"); // on board
	zlog_category_t *zlog_handler = serverLog("../conf/zlog_default.conf");

	//init_program();
	zlog_info(zlog_handler," +++++++++++++++++++++++++++++ start vehicle ++++++++++++++++++++++++++++++++++++++++++++++ \n");

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


	/* air process thread */
	g_air_para* g_air = NULL;
	state = initProcessAirThread(Node, &g_air, g_msg_queue, zlog_handler);

	/* send periodic thread */
	g_periodic_para* g_periodic = NULL;
    state = initPeriodicThread(Node, &g_periodic, g_air, g_msg_queue, zlog_handler);

	gw_sleep(); // need count down wait all thread in startup

	/* ThreadPool handler */
	ThreadPool* g_threadpool = NULL;
	createThreadPool(64, 4, &g_threadpool); // 4096 , 8 

// ------------------------

	/* msg loop */ /* state machine */
	eventLoop(g_air, g_periodic, g_msg_queue, g_RegDev, g_threadpool,zlog_handler);

	closeServerLog();
	
	//pthread_join(*(g_air->para_t->thread_pid),NULL);

    return 0;
}




