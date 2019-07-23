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
	struct ConfigureNode* clientConfigure = (struct ConfigureNode*)malloc(sizeof(struct ConfigureNode));

	clientConfigure->vehicle_id = 0;
	clientConfigure->my_mac_str = (char*)malloc(32);
	clientConfigure->my_Ethernet = (char*)malloc(32);


//  init system global variable
	clientConfigure->system_info = (struct system_info_para*)malloc(sizeof(struct system_info_para));
	clientConfigure->system_info->ve_state = STATE_STARTUP;
	clientConfigure->system_info->isLinked = 0;
	memset(clientConfigure->system_info->ve_mac,0,6);
	memset(clientConfigure->system_info->link_bs_mac,0,6);
	memset(clientConfigure->system_info->next_bs_mac,0,6);

	clientConfigure->system_info->send_id = 0;
	clientConfigure->system_info->rcv_id = 0;

	clientConfigure->system_info->received_air_state_list = (struct received_state_list*)malloc(sizeof(struct received_state_list));
	clientConfigure->system_info->received_air_state_list->received_association_request = 0;
	clientConfigure->system_info->received_air_state_list->received_handover_start_request = 0;
	clientConfigure->system_info->received_air_state_list->received_deassociation = 0;
// 
	const char* configure_path = "../conf/ve_conf.json";
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
		item = cJSON_GetObjectItem(root,"vehicle_id");
		clientConfigure->vehicle_id = item->valueint;
		item = cJSON_GetObjectItem(root, "my_Ethernet");
		memcpy(clientConfigure->my_Ethernet,item->valuestring,strlen(item->valuestring)+1);
		cJSON_Delete(root);
    }

	//get mac addr
	int	nRtn = get_mac(clientConfigure->my_mac_str, 32, clientConfigure->my_Ethernet);
    if(nRtn > 0) // nRtn = 12
    {	
        printf("nRtn = %d , MAC ADDR : %s\n", nRtn,clientConfigure->my_mac_str);
		change_mac_buf(clientConfigure->my_mac_str,clientConfigure->system_info->ve_mac); // // 0408 ---- bug
    }else{
		printf("get mac address failed!\n");
	}

	return clientConfigure;
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

	//zlog_category_t *zlog_handler = serverLog("/run/media/mmcblk1p1/etc/zlog_default.conf"); // on board
	zlog_category_t *zlog_handler = serverLog("../conf/zlog_default.conf");

	struct ConfigureNode* configureNode_ = configure(zlog_handler);
	if(configureNode_ == NULL){
		printf("configureNode_ == NULL \n");
		return 0;
	}

	zlog_info(zlog_handler," +++++++++++++++++++++++++++++ start vehicle ++++++++++++++++++++++++++++++++++++++++++++++ \n");

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
	const char* pro_path = "../vehicle_main.c";
	g_msg_queue_para* g_msg_queue = createMsgQueue(pro_path, zlog_handler);
	if(g_msg_queue == NULL){
		zlog_info(zlog_handler,"No msg_queue created \n");
		return 0;
	}
	zlog_info(zlog_handler, "g_msg_queue->msgid = %d \n", g_msg_queue->msgid);

	state = clearMsgQueue(g_msg_queue);

	/* air process thread */
	g_air_para* g_air = NULL;
	state = initProcessAirThread(configureNode_, &g_air, g_msg_queue, zlog_handler);

	/* send periodic thread */
	g_periodic_para* g_periodic = NULL;
    state = initPeriodicThread(configureNode_, &g_periodic, g_air, g_msg_queue, zlog_handler);

	gw_sleep(); // need count down wait all thread in startup 

// ------------------------

	/* msg loop */ /* state machine */
	eventLoop(g_air, g_periodic, g_msg_queue, g_RegDev, zlog_handler);

	closeServerLog();
	
	//pthread_join(*(g_air->para_t->thread_pid),NULL);

    return 0;
}




