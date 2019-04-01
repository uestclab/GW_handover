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
#include "msg_queue.h"
#include "timer.h"
#include "define_common.h"
#include "zlog.h"

#include "cJSON.h"
#include "gw_utility.h"


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
	struct ConfigureNode* clientConfigure = (struct ConfigureNode*)malloc(sizeof(struct ConfigureNode));
	const char* configure_ip = "127.0.0.1";
	clientConfigure->server_ip = (char*)malloc(16);
	memcpy(clientConfigure->server_ip, configure_ip, strlen(configure_ip)+1);
	clientConfigure->server_port = 44445;
	clientConfigure->my_id = 0;
	clientConfigure->my_mac_str = (char*)malloc(32);
	clientConfigure->my_Ethernet = (char*)malloc(32);

//  init system global variable
	clientConfigure->system_info = (struct system_info_para*)malloc(sizeof(struct system_info_para));
	clientConfigure->system_info->bs_state = STATE_STARTUP;
	clientConfigure->system_info->have_ve_mac = 0;
	memset(clientConfigure->system_info->ve_mac,0,6);
	memset(clientConfigure->system_info->bs_mac,0,6);
	clientConfigure->system_info->received_start_handover_response = 0;

// 
	const char* configure_path = "../configuration_files/client_configure.json";
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
        item = cJSON_GetObjectItem(root, "server_ip");
		memcpy(clientConfigure->server_ip,item->valuestring,strlen(item->valuestring)+1);
        item = cJSON_GetObjectItem(root, "server_port");
		clientConfigure->server_port = item->valueint;
		item = cJSON_GetObjectItem(root,"my_id");
		clientConfigure->my_id = item->valueint;
		item = cJSON_GetObjectItem(root, "my_Ethernet");
		memcpy(clientConfigure->my_Ethernet,item->valuestring,strlen(item->valuestring)+1);
		cJSON_Delete(root);
    }

	//get mac addr
	int	nRtn = get_mac(clientConfigure->my_mac_str, 32, clientConfigure->my_Ethernet);
    if(nRtn > 0) // nRtn = 12
    {	
        printf("nRtn = %d , MAC ADDR : %s\n", nRtn,clientConfigure->my_mac_str);
		change_mac_buf(clientConfigure->my_mac_str,clientConfigure->system_info->bs_mac);
    }else{
		printf("get mac address failed!\n");
	}

	return clientConfigure;
}

int main() // main thread
{
	zlog_category_t *zlog_handler = serverLog("../zlog.conf");

	struct ConfigureNode* configureNode_ = configure(zlog_handler);
	if(configureNode_ == NULL){
		printf("configureNode_ == NULL \n");
		return 0;
	}
	
	/* msg_queue */
	g_msg_queue_para* g_msg_queue = createMsgQueue(configureNode_, zlog_handler);
	if(g_msg_queue == NULL){
		zlog_info(zlog_handler,"No msg_queue created \n");
		return 0;
	}
	zlog_info(zlog_handler, "g_msg_queue->msgid = %d \n", g_msg_queue->msgid);

	/* timer thread */
	g_timer_para* g_timer = NULL;
	int state = InitTimerThread(&g_timer, g_msg_queue, zlog_handler);
	if(state == -1){
		return 0;
	}
	
	/* network thread */
	g_network_para* g_network = NULL;
	state = initNetworkThread(configureNode_, &g_network, g_msg_queue, zlog_handler);
	if(state == 1 || state == 2){
		return 0;
	}

	/* air process thread */
	g_air_para* g_air = NULL;
	state = initProcessAirThread(configureNode_, &g_air, g_msg_queue, g_timer, zlog_handler);


	/* monitor thread */
	g_monitor_para* g_monitor = NULL;
	state = initMonitorThread(configureNode_, &g_monitor, g_msg_queue, g_network, zlog_handler);

	gw_sleep(); // need count down wait 3 thread all in startup !!!! -- 20190329

// ------------------------

	/* msg loop */ /* state machine */
	eventLoop(g_network, g_monitor, g_air, g_msg_queue, zlog_handler);


	freeNetworkThread(g_network);
	closeServerLog();
	
	pthread_join(*(g_monitor->para_t->thread_pid),NULL);
	pthread_join(*(g_network->para_t->thread_pid),NULL);
	pthread_join(*(g_air->para_t->thread_pid),NULL);

    return 0;
}




