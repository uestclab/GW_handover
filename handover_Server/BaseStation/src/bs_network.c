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

#include "bs_network.h"
#include "bs_network_json.h"

#include "cJSON.h"


void* receive_thread(void* args){
	g_network_para* g_network = (g_network_para*)args;
	zlog_info(g_network->log_handler,"Enter receive_thread()");
	pthread_mutex_lock(g_network->para_t->mutex_);
	while (g_network->connected == 0 )
	{
		pthread_cond_wait(g_network->para_t->cond_, g_network->para_t->mutex_);
	}
	pthread_mutex_unlock(g_network->para_t->mutex_);

	if(g_network->startup == 0){ // start event link ---------------------------------------------------- bs first action !!!!
		zlog_info(g_network->log_handler,"send_id_pair_signal -------- first action to handover server");
		send_id_pair_signal(g_network->node->my_id, g_network->node->my_mac_str, g_network);
		g_network->startup = 1;
	}
	receive(g_network);

    zlog_info(g_network->log_handler,"Exit receive_thread()");
}

void receive(g_network_para* g_network){
	while(1){ 
		int n;
		int size = 0;
		int totalByte = 0;
		int messageLength = 0;
		
		char* temp_receBuffer = g_network->recvbuf + 1000; //
		char* pStart = NULL;
		char* pCopy = NULL;

		n = recv(g_network->sock_cli, temp_receBuffer, BUFFER_SIZE,0); // block 
		if(n<=0){
			//return;
			continue;
		}
		size = n;
		//zlog_info(g_network->log_handler,"-------------------- size = %d", size);

		pStart = temp_receBuffer - g_network->gMoreData_;
		totalByte = size + g_network->gMoreData_;
		
		const int MinHeaderLen = sizeof(int32_t);

		while(1){
		    if(totalByte <= MinHeaderLen)
		    {
		        g_network->gMoreData_ = totalByte;
		        pCopy = pStart;
		        if(g_network->gMoreData_ > 0)
		        {
		            memcpy(temp_receBuffer - g_network->gMoreData_, pCopy, g_network->gMoreData_);
		        }
		        break;
		    }
		    if(totalByte > MinHeaderLen)
		    {
		        messageLength= myNtohl(pStart);
	
		        if(totalByte < messageLength + MinHeaderLen )
		        {
		            g_network->gMoreData_ = totalByte;
		            pCopy = pStart;
		            if(g_network->gMoreData_ > 0){
		                memcpy(temp_receBuffer - g_network->gMoreData_, pCopy, g_network->gMoreData_);
		            }
		            break;
		        } 
		        else// at least one message 
		        {
					processMessage(pStart,messageLength,g_network);
					// move to next message
		            pStart = pStart + messageLength + MinHeaderLen;
		            totalByte = totalByte - messageLength - MinHeaderLen;
		            if(totalByte == 0){
		                g_network->gMoreData_ = 0;
		                break;
		            }
		        }         
		    }
		}

		if(g_network->connected == 0)
			break;
	}	
}

int initNetworkThread(struct ConfigureNode* Node, g_network_para** g_network, g_msg_queue_para* g_msg_queue, zlog_category_t* handler){
	zlog_info(handler,"initNetworkThread()");

	*g_network = (g_network_para*)malloc(sizeof(struct g_network_para));

	(*g_network)->log_handler = handler;
	(*g_network)->para_t = newThreadPara();
	(*g_network)->connected = 0;
	(*g_network)->recvbuf = malloc(BUFFER_SIZE);
	(*g_network)->sendMessage = malloc(BUFFER_SIZE);
	(*g_network)->gMoreData_ = 0;
    (*g_network)->sock_cli = socket(AF_INET,SOCK_STREAM, 0);
	(*g_network)->g_msg_queue = g_msg_queue;
	(*g_network)->node = Node;
	(*g_network)->startup = 0;

	zlog_info(handler,"Configure: server_ip = %s, server_port = %d",
		Node->server_ip,Node->server_port);

	// create thread
	int ret = pthread_create((*g_network)->para_t->thread_pid, NULL, receive_thread, (void*)(*g_network));
    if(ret != 0){
        zlog_error(handler,"pthread_create error ! error_code = %d", ret);
		return 1;
    }
    
    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(Node->server_port);
    servaddr.sin_addr.s_addr = inet_addr(Node->server_ip);
    
    zlog_info(handler,"link %s:%d",Node->server_ip,Node->server_port);

	int connect_stat = connect((*g_network)->sock_cli, (struct sockaddr *)&servaddr, sizeof(servaddr));

    while (connect_stat < 0){
        perror("connect");

		// add 0603
		
		gw_sleep();
		
		connect_stat = connect((*g_network)->sock_cli, (struct sockaddr *)&servaddr, sizeof(servaddr));
		
		zlog_info(handler,"No server , connect is failure");
		// -------------------- end

		//return 2;
    }
	//free(Node->server_ip); // 0603 modify
	(*g_network)->connected = 1;
	pthread_cond_signal((*g_network)->para_t->cond_);
	
	return 0;
}

int freeNetworkThread(g_network_para* g_network)
{
	close(g_network->sock_cli);
	destoryThreadPara(g_network->para_t);
	if(g_network->recvbuf != NULL){	
		free(g_network->recvbuf);
	}
	if(g_network->sendMessage != NULL){
		free(g_network->sendMessage);
	}
	g_network->gMoreData_ = 0;
	g_network->connected  = 0;
	g_network->startup = 0;
	zlog_info(g_network->log_handler,"freeThread()");
	free(g_network);
	return 0;
}

// -------------------------------------- 
messageInfo* parseMessage(char* buf, int32_t length){
    messageInfo* message = (messageInfo*)malloc(sizeof(messageInfo));
    message->length = length;
    message->signal = (signalType)(myNtohl(buf+sizeof(int32_t)));
    message->buf = buf+sizeof(int32_t)*2;
    return message;
}

void printcjson(char* json, g_network_para* g_network){
	zlog_info(g_network->log_handler,"receive : json = %s\n",json);
	zlog_info(g_network->log_handler,"--------------------------------------\n");
}

void processMessage(char* buf, int32_t length, g_network_para* g_network){
    messageInfo* message = parseMessage(buf,length);
	//cJSON * root = NULL;
    //cJSON * item = NULL;
    //root = cJSON_Parse(message->buf);
    switch(message->signal){
        case ID_RECEIVED:
        {
			printcjson(message->buf,g_network);
            break;
        }
        case INIT_LOCATION:
        {
			printcjson(message->buf,g_network);

			struct msg_st data;
			data.msg_type = MSG_START_MONITOR;
			data.msg_number = MSG_START_MONITOR;
			data.msg_len = 0;
			postMsgQueue(&data,g_network->g_msg_queue);

            break;
        }
        case INIT_LINK:
        {
			printcjson(message->buf,g_network);
			
			struct msg_st data;
			data.msg_type = MSG_INIT_SELECTED;
			data.msg_number = MSG_INIT_SELECTED;
			data.msg_len = 0;
			postMsgQueue(&data,g_network->g_msg_queue);

            break;
        }
        case START_HANDOVER:
        {
			printcjson(message->buf,g_network);

			//item = cJSON_GetObjectItem(root, "target_bs_mac");
			//char target_mac_buf[6];
			//change_mac_buf(item->valuestring,target_mac_buf);

			

			struct msg_st data;
			data.msg_type = MSG_START_HANDOVER;
			data.msg_number = MSG_START_HANDOVER;
			data.msg_len = strlen(message->buf) + 1;
			memcpy(data.msg_json,message->buf,data.msg_len);
			//memcpy(data.msg_json,target_mac_buf,6); // Note : !! 
			
			postMsgQueue(&data,g_network->g_msg_queue);

            break;
        }
		case CHANGE_TUNNEL_ACK:
		{
			printcjson(message->buf,g_network);
			break;
		}
		case SERVER_RECALL_MONITOR: // server control with source bs distance monitor msg -- 20191023
		{
			printcjson(message->buf,g_network);
			struct msg_st data;
			data.msg_type = MSG_SERVER_RECALL_MONITOR;
			data.msg_number = MSG_SERVER_RECALL_MONITOR;
			data.msg_len = 0;
			postMsgQueue(&data,g_network->g_msg_queue);
			break;
		}
        default:
        {
            break;
        }
    }
    free(message);
	//cJSON_Delete(root); // detect mem leak -- 0611
}

// ---- send section : sendSignal thread safety ----

void sendSignal(signalType type, char* json, g_network_para* g_network){
	pthread_mutex_lock(g_network->para_t->mutex_); // called by 2 thread
    messageInfo* message = (messageInfo*)malloc(sizeof(messageInfo));
    message->length = 0;
    message->signal = type;
    message->buf = json;
	zlog_info(g_network->log_handler,"bs send : %s \n" , message->buf);
	// ---- sendMessage send ---- 
	*((int32_t*)(g_network->sendMessage+sizeof(int32_t))) = htonl((int32_t)(message->signal)); 
	int32_t buf_length = strlen(message->buf) + 1;
	message->length = sizeof(int32_t) * 2 + buf_length; // error?
	*((int32_t*)(g_network->sendMessage)) = htonl(message->length);
	memcpy(g_network->sendMessage + sizeof(int32_t) * 2, message->buf, buf_length);
	int status = send(g_network->sock_cli, g_network->sendMessage, message->length + sizeof(int32_t), 0);
	if(status != message->length + sizeof(int32_t))
		zlog_error(g_network->log_handler,"Error in client send socket: send length = %d , expected length = %d", status , message->length + sizeof(int32_t));

    free(message);
    free(json);
	pthread_mutex_unlock(g_network->para_t->mutex_);
}



