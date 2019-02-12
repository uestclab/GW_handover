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
#include "baseStationTR_arm.h"
#include "cJSON.h"
#include "gw_utility.h"

para_thread* para_t = NULL;
int sock_cli = 0;
int connected = 0;

char* sendMessage = NULL; // only in send
char* recvbuf = NULL;
int	 gMoreData_ = 0;

//log
zlog_category_t* log_handler = NULL;

void *
receive_thread(void* args){
	int sock_cli = *((int*)args);

	pthread_mutex_lock(para_t->mutex_);
    while(1){
		while (connected == 0 )
		{
			pthread_cond_wait(para_t->cond_, para_t->mutex_);
		}
		pthread_mutex_unlock(para_t->mutex_);
    	receive();
    }
    zlog_info(log_handler,"Exit receive_thread()");

}

void receive(){ 
    int n;
    int size = 0;
    int totalByte = 0;
    int messageLength = 0;
    
    char* temp_receBuffer = recvbuf + 1000; //
    char* pStart = NULL;
    char* pCopy = NULL;

    n = recv(sock_cli, temp_receBuffer, BUFFER_SIZE,0);
    if(n<=0){
		return;
    }
    size = n;
    zlog_info(log_handler,"-------------------- size = %d", size);

    pStart = temp_receBuffer - gMoreData_;
    totalByte = size + gMoreData_;
    
    const int MinHeaderLen = sizeof(int32_t);

    while(1){
        if(totalByte <= MinHeaderLen)
        {
            gMoreData_ = totalByte;
            pCopy = pStart;
            if(gMoreData_ > 0)
            {
                memcpy(temp_receBuffer - gMoreData_, pCopy, gMoreData_);
            }
            break;
        }
        if(totalByte > MinHeaderLen)
        {
            messageLength= myNtohl(pStart);
	
            if(totalByte < messageLength + MinHeaderLen )
            {
                gMoreData_ = totalByte;
                pCopy = pStart;
                if(gMoreData_ > 0){
                    memcpy(temp_receBuffer - gMoreData_, pCopy, gMoreData_);
                }
                break;
            } 
            else// at least one message 
            {
				processMessage(pStart,messageLength);
				// move to next message
                pStart = pStart + messageLength + MinHeaderLen;
                totalByte = totalByte - messageLength - MinHeaderLen;
                if(totalByte == 0){
                    gMoreData_ = 0;
					//printf("No more message !\n");
                    break;
                }
            }//else             
        }//(totalByte > MinHeaderLen)
    } //while(1)	
}

int  initThread(struct ConfigureNode* Node, zlog_category_t* handler)
{
	log_handler = handler;
	zlog_info(log_handler,"Configure: server_ip = %s, server_port = %d",
			Node->server_ip,Node->server_port);

	recvbuf = malloc(BUFFER_SIZE);
	sendMessage = malloc(BUFFER_SIZE);
	// create thread
	para_t = newThreadPara();

    ///sockfd
    sock_cli = socket(AF_INET,SOCK_STREAM, 0);
	
	// create thread
	int ret = pthread_create(para_t->thread_pid, NULL, receive_thread, (void*)&sock_cli);
    if(ret != 0){
        zlog_error(log_handler,"pthread_create error ! error_code = %d", ret);
		return 1;
    }
    
    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(Node->server_port);
    servaddr.sin_addr.s_addr = inet_addr(Node->server_ip);
    
    zlog_info(log_handler,"link %s:%d",Node->server_ip,Node->server_port);
    if (connect(sock_cli, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0){
        perror("connect");
		return 2;
    }
	free(Node->server_ip);
	connected = 1;
	pthread_cond_signal(para_t->cond_);
	
	return 0;
}

int freeThread()
{
	close(sock_cli);
	destoryThreadPara(para_t);
	if(recvbuf != NULL){	
		free(recvbuf);
	}
	if(sendMessage != NULL){
		free(sendMessage);
	}
	zlog_info(log_handler,"freeThread()");
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

void printcjson(cJSON* root){
	cJSON * item = NULL;
	printf("------- receive : \n");
	item = cJSON_GetObjectItem(root, "signal");
	printf("signal : %s \n", item->valuestring);
	item = cJSON_GetObjectItem(root, "bs_id");
	printf("id = %d \n", item->valueint);
	item = cJSON_GetObjectItem(root, "rssi");
	printf("rssi = %f \n", item->valuedouble);
	item = cJSON_GetObjectItem(root, "bs_mac_addr");
	printf("bs_mac_addr = %s \n" , item->valuestring);
	item = cJSON_GetObjectItem(root, "train_mac_addr");
	printf("train_mac_addr = %s \n" , item->valuestring);
	printf("---------------\n");
}

void processMessage(char* buf, int32_t length){
    messageInfo* message = parseMessage(buf,length);
    cJSON * root = NULL;
    cJSON * item = NULL;
    root = cJSON_Parse(message->buf);
    switch(message->signal){
        case ID_RECEIVED:
        {
			// reserver train mac address 
			printcjson(root);
            break;
        }
        case INIT_LOCATION:
        {
			printcjson(root);
            break;
        }
        case INIT_LINK:
        {
			// communicate with air interface immediatatly , and send INIT_COMPLETED signal to server
			printcjson(root);
            break;
        }
        case START_HANDOVER:
        {
			printcjson(root);
            break;
        }
        default:
        {
            break;
        }
    }
    free(message);
}

// ---- send section : sendSignal thread safety ----

signal_json* clear_json(){
    signal_json* input = (signal_json*)malloc(sizeof(signal_json));
    input->bsId_ = -1;
    input->rssi_ = -99.9;
    memset(input->bsMacAddr_,0x00,32);
	input->bsMacAddr_[31] = '\0';
    memset(input->trainMacAddr_,0x00,32);
	input->trainMacAddr_[31] = '\0';
    return input;
}

void sendSignal(signalType type, signal_json* json){
	pthread_mutex_lock(para_t->mutex_); // called by 2 thread
    messageInfo* message = (messageInfo*)malloc(sizeof(messageInfo));
    message->length = 0;
    message->signal = type;
    cJSON* root = cJSON_CreateObject();
    if(type == ID_PAIR)
		cJSON_AddStringToObject(root, "signal", "id_pair_signal");
    else if(type == READY_HANDOVER)
        cJSON_AddStringToObject(root, "signal", "ready_handover_signal");
    else if(type == INIT_COMPLETED)
        cJSON_AddStringToObject(root, "signal", "initcompleted_signal");
    else if(type == LINK_CLOSED)
        cJSON_AddStringToObject(root, "signal", "link_closed_signal");
    else if(type == LINK_OPEN)
        cJSON_AddStringToObject(root, "signal", "link_open_signal");
    cJSON_AddNumberToObject(root, "bs_id", json->bsId_);
    cJSON_AddNumberToObject(root, "rssi", json->rssi_);
    cJSON_AddStringToObject(root, "bs_mac_addr", json->bsMacAddr_);
    cJSON_AddStringToObject(root, "train_mac_addr", json->trainMacAddr_);
    message->buf = cJSON_Print(root);
	printf("bs send : %s \n" , message->buf);
	// ---- sendMessage send ---- 
	*((int32_t*)(sendMessage+sizeof(int32_t))) = htonl((int32_t)(message->signal)); 
	int32_t buf_length = strlen(message->buf) + 1;
	message->length = sizeof(int32_t) * 2 + buf_length;
	*((int32_t*)sendMessage) = htonl(message->length);
	memcpy(sendMessage + sizeof(int32_t) * 2, message->buf, buf_length);
	int status = send(sock_cli, sendMessage, message->length + sizeof(int32_t), 0);
	if(status != message->length + sizeof(int32_t))
		zlog_error(log_handler,"Error in client send socket: send length = %d , expected length = %d", status , message->length + sizeof(int32_t));

    free(message->buf);
    free(message);
    free(json);
    cJSON_Delete(root);
	pthread_mutex_unlock(para_t->mutex_);
}

//cJSON_AddNumberToObject(temp_root, "register", *((int*)(callback_buffer->buf_)));
//cJSON_AddNumberToObject(temp_root, "send_event", (int32_t)1);
//cJSON_AddStringToObject(temp_root, "string", "TEST_1");



