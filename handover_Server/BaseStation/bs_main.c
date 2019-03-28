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

#include "bs_network.h"
#include "bs_monitor.h"
#include "define_common.h"
#include "zlog.h"

#include "cJSON.h"
#include "gw_utility.h"


zlog_category_t *zlog_handler = NULL;

int serverLog(const char* path){
	int rc;

	rc = zlog_init(path);

	if (rc) {
		//printf("init failed\n");
		return -1;
	}

	zlog_handler = zlog_get_category("baseStation");

	if (!zlog_handler) {
		//printf("get cat fail\n");

		zlog_fini();
		return -2;
	}

	return 0;
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
	clientConfigure->my_mac = (char*)malloc(32);
	clientConfigure->my_Ethernet = (char*)malloc(32);

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
	int	nRtn = get_mac(clientConfigure->my_mac, 32, clientConfigure->my_Ethernet);
    if(nRtn > 0) // nRtn = 12
    {	
        printf("nRtn = %d , MAC ADDR : %s\n", nRtn,clientConfigure->my_mac);
    }else{
		printf("get mac address failed!\n");
	}

	return clientConfigure;
}

int main() // main thread
{
	int status = serverLog("../zlog.conf");

	struct ConfigureNode* configureNode_ = configure(zlog_handler);
	if(configureNode_ == NULL){
		printf("configureNode_ == NULL \n");
		return 0;
	}

	int state = initThread(configureNode_, zlog_handler);
	if(state == 1 || state == 2){
		//return 0;
	}
	
	// test 0328
	g_monitor_para* g_monitor = NULL;
	int stat = initMonitorThread(configureNode_, &g_monitor, zlog_handler);
	user_wait();
	zlog_info(zlog_handler,"startMonitor() , g_monitor = %d \n",g_monitor);
	startMonitor(g_monitor);

	pthread_join(*(g_monitor->para_t->thread_pid),NULL);

	return 0;
	

	//user_wait();

	user_wait();
	signal_json* json = clear_json();
	json->bsId_ = configureNode_->my_id;
	memcpy(json->bsMacAddr_, configureNode_->my_mac, strlen(configureNode_->my_mac)+1);
	sendSignal(ID_PAIR,json);

	user_wait(); // 1.wait init_location_signal, 2.get and caculate rssi, then make decision   
	json = clear_json();
	json->bsId_ = configureNode_->my_id;
	json->rssi_ = 10;
	sendSignal(READY_HANDOVER,json);

	user_wait(); // 1.my bs is choosed as link one after init_link_signal, process air interface , then send completed when air interface prepared
	json = clear_json();
	json->bsId_ = configureNode_->my_id;
	json->rssi_ = 10;
	sendSignal(INIT_COMPLETED,json);

	user_wait(); // 1.send when running? (notification)
	json = clear_json();
	json->bsId_ = configureNode_->my_id;
	json->rssi_ = 20;
	sendSignal(READY_HANDOVER,json);

	user_wait();
	freeThread();
	closeServerLog();
    return 0;
}
