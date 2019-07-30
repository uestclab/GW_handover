#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <pthread.h>

#include "bs_x2.h"
#include "bs_network_json.h"

#include "cJSON.h"

void printcx2json(char* json, g_x2_para* g_x2){
	zlog_info(g_x2->log_handler,"receive x2 : json = %s\n",json);
	zlog_info(g_x2->log_handler,"--------------------------------------\n");
}

void processX2Signal(char* buf, int32_t length, struct sockaddr_in remote_addr, g_x2_para* g_x2){ // continue

	cJSON * root = NULL;
    cJSON * item = NULL;
    root = cJSON_Parse(buf);

    item = cJSON_GetObjectItem(root, "x2signal");
	if(strcmp(item->valuestring,"dac_closed") == 0){
		printcx2json(buf,g_x2);
		struct msg_st data;
		data.msg_type = MSG_SOURCE_BS_DAC_CLOSED;
		data.msg_number = MSG_SOURCE_BS_DAC_CLOSED;
		data.msg_len = 0;
		postMsgQueue(&data,g_x2->g_msg_queue);
		item = cJSON_GetObjectItem(root,"own_ip");
		memcpy(g_x2->remote_ip, item->valuestring, strlen(item->valuestring)+1);
		send_dac_closed_x2_ack_signal(g_x2->node->my_id, g_x2);
	}else if(strcmp(item->valuestring,"dac_closed_ack") == 0){
		printcx2json(buf,g_x2);
		struct msg_st data;
		data.msg_type = MSG_RECEIVED_DAC_CLOSED_ACK;
		data.msg_number = MSG_RECEIVED_DAC_CLOSED_ACK;
		data.msg_len = 0;
		postMsgQueue(&data,g_x2->g_msg_queue);
	}
	
	cJSON_Delete(root);
}

void x2_receive(g_x2_para* g_x2){
	int ret = 0;
    struct sockaddr_in remote_addr;
    int size_addr = sizeof(struct sockaddr);
	while(1){
		if(ret = recvfrom(g_x2->sock_server, g_x2->receive_buf, UDP_BUFFER_SIZE, 0, (struct sockaddr *)&remote_addr, &size_addr)<0){
			zlog_info(g_x2->log_handler, "recvfrom() error!\n");
			continue;
		}
		//printf("%s",g_x2->receive_buf);
		processX2Signal(g_x2->receive_buf, ret, remote_addr, g_x2);
	}
}


void* x2_receive_thread(void* args){
	g_x2_para* g_x2 = (g_x2_para*)args;
	zlog_info(g_x2->log_handler,"Enter receive_thread()");
	pthread_mutex_lock(g_x2->para_t->mutex_);
    while(1){
		while (g_x2->connected == 0 )
		{
			pthread_cond_wait(g_x2->para_t->cond_, g_x2->para_t->mutex_);
		}
		pthread_mutex_unlock(g_x2->para_t->mutex_);

    	x2_receive(g_x2);
		if(g_x2->connected == 0)
			break;
    }
    zlog_info(g_x2->log_handler,"Exit receive_thread()");
}

int initX2Thread(struct ConfigureNode* Node, g_x2_para** g_x2, g_msg_queue_para* g_msg_queue, zlog_category_t* handler){
	zlog_info(handler,"initX2Thread()");

	*g_x2 = (g_x2_para*)malloc(sizeof(struct g_x2_para));

	(*g_x2)->log_handler = handler;
	(*g_x2)->para_t = newThreadPara();
	(*g_x2)->receive_buf = malloc(UDP_BUFFER_SIZE);
    (*g_x2)->sock_server = socket(AF_INET,SOCK_DGRAM, 0);
	(*g_x2)->sock_client = -1;
	(*g_x2)->g_msg_queue = g_msg_queue;
	(*g_x2)->connected   = 0;
	(*g_x2)->remote_ip   = (char*)malloc(32);
	(*g_x2)->node = Node;

	zlog_info(handler,"Configure: udp_server_ip = %s, udp_server_port = %d",
		Node->udp_server_ip,Node->udp_server_port);

	// create thread
	int ret = pthread_create((*g_x2)->para_t->thread_pid, NULL, x2_receive_thread, (void*)(*g_x2));
    if(ret != 0){
        zlog_error(handler,"pthread_create error ! error_code = %d", ret);
		return -2;
    }
    
    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(Node->udp_server_port);
    servaddr.sin_addr.s_addr = inet_addr(Node->udp_server_ip);

	if(bind((*g_x2)->sock_server,(struct sockaddr*)&servaddr,sizeof(struct sockaddr_in))<0){
        zlog_info(handler,"bind() error!\n");
		return -1;
	}
    
    zlog_info(handler,"udp x2 server %s:%d",Node->udp_server_ip,Node->udp_server_port);


	(*g_x2)->connected   = 1;
	pthread_cond_signal((*g_x2)->para_t->cond_);
	
	return 0;
}

int freeX2Thread(g_x2_para* g_x2)
{
	close(g_x2->sock_server);
	destoryThreadPara(g_x2->para_t);
	if(g_x2->receive_buf != NULL){	
		free(g_x2->receive_buf);
	}

	zlog_info(g_x2->log_handler,"freeX2Thread()");
	free(g_x2);
	return 0;
}

// --------------------------------------
void sendX2Signal(char* json, g_x2_para* g_x2){
	zlog_info(g_x2->log_handler,"bs x2 send : %s \n" , json);
	//configure_target_ip(NULL, g_x2); // temp for test process 
	g_x2->sock_client = socket(AF_INET,SOCK_DGRAM,0);
	struct sockaddr_in saddr;
    memset(&saddr,0,sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(g_x2->node->udp_server_port);
    saddr.sin_addr.s_addr = inet_addr(g_x2->remote_ip);

	int status = sendto(g_x2->sock_client, json, strlen(json) + 1, 0, (struct sockaddr *)&saddr, sizeof(struct sockaddr));
	if(status < 0)
		zlog_info(g_x2->log_handler, "sendX2Signal() error!\n");
	close(g_x2->sock_client);
	g_x2->sock_client = -1;
}

void configure_target_ip(char* ip, g_x2_para* g_x2){
	memcpy(g_x2->remote_ip,ip,strlen(ip)+1);
	if(g_x2->node->my_id == 11){
		char* ip_66 = "192.168.10.66";
		//memcpy(g_x2->remote_ip,ip_66,strlen(ip_66)+1);
	}else if(g_x2->node->my_id == 22){
		char* ip_33 = "192.168.10.33";
		//memcpy(g_x2->remote_ip,ip_33,strlen(ip_33)+1);
	}
}







