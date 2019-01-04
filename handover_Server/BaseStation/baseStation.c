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
#include <pthread.h>
#include "BaseStation.h"
#include "cJSON.h"

para_thread para_t;

int sock_cli = 0;
int connected = 0;
deviceID Mydevice = 0;

char* sendMessage = NULL; // only in assembleFrame();
int32_t registerValue; // 
struct GW_buf* callback_buffer = NULL;

char* recvbuf = NULL;
int	 gMoreData_ = 0;

//callback function pointer
mydata_t * callback = NULL;

//log
zlog_category_t* log_handler = NULL;

int32_t myNtohl(const char* buf){
  int32_t be32 = 0;
  memcpy(&be32, buf, sizeof(be32)); // memcpy
  return ntohl(be32);
}

void user_wait()
{
	int c;

	#ifdef __cplusplus
		cout << WAIT_MSG << endl;
	#else
		printf("%s\n", WAIT_MSG);
	#endif

	do
	{
		c = getchar();
		if(c == 'g') break;
	} while(c != '\n');
}

int filelength(FILE *fp)
{
	int num;
	fseek(fp,0,SEEK_END);
	num=ftell(fp);
	fseek(fp,0,SEEK_SET);
	return num;
}

char* readfile(const char *path)
{
	FILE *fp;
	int length;
	char *ch;
	if((fp=fopen(path,"r"))==NULL)
	{
		zlog_error(log_handler,"open file %s error.\n",path);
		return NULL;
		//exit(0);
	}
	length=filelength(fp);
	ch=(char *)malloc(length);
	fread(ch,length,1,fp);
	*(ch+length-1)='\0';
	fclose(fp);
	return ch;
}

void *
receive_thread(void* args){
	int sock_cli = *((int*)args);

	pthread_mutex_lock(para_t.mutex_);
    while(1){
		while (connected == 0 )
		{
			pthread_cond_wait(para_t.cond_, para_t.mutex_);
		}
		pthread_mutex_unlock(para_t.mutex_);
    	receive();
    }
    zlog_info(log_handler,"Exit receive_thread()");

}

void receive(){ 
    int n;
    int size = 0;
    int totalByte = 0;
    int messageLength = 0;
    int typeNameLength = 0;
    
    char* temp_receBuffer = recvbuf + 20000; // hard code for offset = 100 
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
	
            if(totalByte < messageLength )
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
				proReceiveMessage(pStart,messageLength);

				// move to next message
                pStart = pStart + messageLength + MinHeaderLen;
                totalByte = totalByte - messageLength - MinHeaderLen;
                if(totalByte == 0){
                    gMoreData_ = 0;
					//printf("No more message !\n");
                    break;
                }
            }//else             
        }//(totalByte > MinHeaderLen * 2)
    } //while(1)	
}

frameInfo* parseFrame(const char* buf, int32_t length){
	
	frameInfo* receive_frame = clear_frameInfo();
	receive_frame->messageLength = length;
	receive_frame->frame_buf = buf;

	unsigned int temp_action = myNtohl(buf + sizeof(int32_t));
	unsigned int temp_device = myNtohl(buf + sizeof(int32_t) * 2);

	receive_frame->action = (actionID)(temp_action & 0xffff);

	receive_frame->source = (deviceID)(temp_device >> 16);
	receive_frame->dest = (deviceID)(temp_device & 0xffff);

	if(receive_frame->source == HTTP){
		receive_frame->id = (int32_t)(temp_action >> 16);
	}

	if(receive_frame->action == GW_EVENT){
		receive_frame->event = (eventID)myNtohl(buf + sizeof(int32_t) * 3);
	}
	return receive_frame;
}

void proReceiveMessage(const char* buf, int32_t length)
{
	zlog_info(log_handler,"proReceiveMessage()");
	frameInfo* receive_frame = parseFrame(buf,length);
	if(receive_frame->dest != Mydevice){
		zlog_info(log_handler,"This request is not match this device!");
		return;
	}

	if(receive_frame->action == GW_GET){
		zlog_info(log_handler,"receive_frame->action == GW_GET");
		const char* payload = receive_frame->frame_buf + sizeof(int32_t) * 3;
		// inform new cjon message is coming 
		// probability block when call callback function
		int bStatus = callback->frcb(payload, callback->rptr, callback_buffer);//&registerValue);
		if(bStatus == 0 && callback_buffer->buf_ != NULL && callback_buffer->length_buf > 0)
		{
			cJSON *temp_root = cJSON_CreateObject();
			cJSON_AddNumberToObject(temp_root, "register", *((int*)(callback_buffer->buf_)));
			//printf("registerValue = %d \n" , *((int*)(callback_buffer->buf_)));
			free(callback_buffer->buf_);
			callback_buffer->length_buf = 0;
			frameInfo*	send_frame = clear_frameInfo();
			send_frame->action = GW_INFO;
			if(receive_frame->source == HTTP){
				send_frame->id = receive_frame->id;
			}
			send_frame->source = Mydevice;
			send_frame->dest   = receive_frame->source;
			send_frame->send_buf = cJSON_Print(temp_root);
		
			assembleFrame(send_frame);
			cJSON_Delete(temp_root);
		}

	}else if(receive_frame->action == GW_WRITE){
		const char* payload = buf + sizeof(int32_t) * 3;
		//printf("WriteMessage jsonStr = %s\n", payload);
		int bStatus = callback->frcb(payload, callback->rptr,callback_buffer);
		if(bStatus == 0)
		{
			frameInfo*	send_frame = clear_frameInfo();
			send_frame->action = GW_EVENT;
			send_frame->event = WRITE_COMPLETED;
			if(receive_frame->source == HTTP){
				send_frame->id = receive_frame->id;
			}
			send_frame->source = Mydevice;
			send_frame->dest   = receive_frame->source;
		
			assembleFrame(send_frame);

			if(callback_buffer->buf_ != NULL && callback_buffer->length_buf > 0){
				free(callback_buffer->buf_);
				callback_buffer->length_buf = 0;
			}
		}
		
	}else if(receive_frame->action == GW_EVENT){
		if(receive_frame->event == TEST_1){
			zlog_info(log_handler,"receive event TEST_1 , cjson = %s", receive_frame->frame_buf + sizeof(int32_t) * 4);
		}else if(receive_frame->event == TEST_2){
			zlog_info(log_handler,"receive event TEST_2 , cjson = %s", receive_frame->frame_buf + sizeof(int32_t) * 4);
		}else if(receive_frame->event == TEST_3){
			zlog_info(log_handler,"receive event TEST_3 , cjson = %s", receive_frame->frame_buf + sizeof(int32_t) * 4);
		}else if(receive_frame->event == TEST_4){
			zlog_info(log_handler,"receive event TEST_4 , cjson = %s", receive_frame->frame_buf + sizeof(int32_t) * 4);
		}else if(receive_frame->event == TEST_5){
			zlog_info(log_handler,"receive event TEST_5 , cjson = %s", receive_frame->frame_buf + sizeof(int32_t) * 4);
		}else if(receive_frame->event == WRITE_COMPLETED){
			zlog_info(log_handler,"receive event WRITE_COMPLETED ");		
		}
	}else if(receive_frame->action == GW_INFO){
		zlog_info(log_handler,"receive_frame->action == GW_INFO");
		const char* payload = receive_frame->frame_buf + sizeof(int32_t) * 3;
		int bStatus = callback->frcb(payload, callback->rptr, callback_buffer);
	}
	free(receive_frame);
}

// add data to send_frame and call assembleFrame, send to server
void assembleFrame(frameInfo* send_frame){
	pthread_mutex_lock(para_t.mutex_);
	int32_t messLength = 0;
	int32_t action = (((int32_t)(send_frame->action) & 0x0000ffff) |  send_frame->id << 16 );   
	*((int32_t*)(sendMessage+4)) = htonl(action);
	int32_t device = ((int32_t)(send_frame->source) << 16 | ((int32_t)(send_frame->dest) & 0x0000ffff));
	*((int32_t*)(sendMessage+8)) = htonl(device);

	if(send_frame->action == GW_EVENT){
		int param_length = 0;
		if(send_frame->send_buf != NULL){
			char* param_payload = send_frame->send_buf;
			param_length = strlen(param_payload) + 1;
			memcpy(sendMessage + sizeof(int32_t) * 4,send_frame->send_buf,param_length);
			free(param_payload);
		}
		messLength = sizeof(int32_t) * 3 + param_length;
		*((int32_t*)sendMessage) = htonl(messLength);
		*((int32_t*)(sendMessage+12)) = htonl(send_frame->event);
	}else if(send_frame->action == GW_INFO || send_frame->action == GW_GET || send_frame->action == GW_WRITE){
		char* payload = send_frame->send_buf;
		int payloadLength = strlen(payload) + 1; 
		messLength = sizeof(int32_t) * 2 + payloadLength;
		*((int32_t*)sendMessage) = htonl(messLength);
		memcpy(sendMessage + sizeof(int32_t) * 3, payload, payloadLength);
		free(payload);
	}
	int status = send(sock_cli, sendMessage, messLength + sizeof(int32_t), 0); ///发送
	if(status != messLength + sizeof(int32_t))
		zlog_error(log_handler,"Error in client send socket: send length = %d , expected length = %d", status , messLength + sizeof(int32_t));
	free(send_frame);
	pthread_mutex_unlock(para_t.mutex_);
}


int  initThread(deviceID device, zlog_category_t* handler)
{
	if(handler != NULL)
		log_handler = handler;
	struct clientConfigureNode* clientConfigure = (struct clientConfigureNode*)malloc(sizeof(struct clientConfigureNode));
	const char* configure_ip = "192.168.1.11";
	clientConfigure->server_ip = (char*)malloc(16);
	memcpy(clientConfigure->server_ip, configure_ip, strlen(configure_ip)+1);
	clientConfigure->server_port = 44444;

	const char* configure_path = "../client_configure.json";
	char* pConfigure_file = readfile(configure_path);
	if(pConfigure_file == NULL)
		return 1;
	cJSON * root = NULL;
    cJSON * item = NULL;
    root = cJSON_Parse(pConfigure_file);
	free(pConfigure_file);     
    if (!root) 
    {
        zlog_error(log_handler,"Error before: [%s]",cJSON_GetErrorPtr());
    }
    else
    {
        item = cJSON_GetObjectItem(root, "server_ip");//
		memcpy(clientConfigure->server_ip,item->valuestring,strlen(item->valuestring)+1);
        item = cJSON_GetObjectItem(root, "server_port");
		clientConfigure->server_port = atoi(item->valuestring);
		cJSON_Delete(root);
    }
	
	zlog_info(log_handler,"clientConfigure: server_ip = %s, server_port = %d",
			clientConfigure->server_ip,clientConfigure->server_port);


	recvbuf = malloc(BUFFER_SIZE);
	sendMessage = malloc(BUFFER_SIZE);
	// create thread
	para_t.thread_ = (pthread_t*)malloc(sizeof(pthread_t));
    
    para_t.mutex_ = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
    int n = pthread_mutex_init(para_t.mutex_,NULL);
    if(n != 0)
        zlog_error(log_handler,"mutex init failed !");
    
    para_t.cond_ = (pthread_cond_t*)malloc(sizeof(pthread_cond_t));
    n = pthread_cond_init(para_t.cond_,NULL);
    if(n != 0)
        zlog_error(log_handler,"condition variable init failed !");

    ///sockfd
    sock_cli = socket(AF_INET,SOCK_STREAM, 0);
	
	// create thread
	int ret = pthread_create(para_t.thread_, NULL, receive_thread, (void*)&sock_cli);
    if(ret != 0){
        zlog_error(log_handler,"pthread_create error ! error_code = %d", ret);
    }
    

    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(clientConfigure->server_port);
    servaddr.sin_addr.s_addr = inet_addr(clientConfigure->server_ip);
    
    zlog_info(log_handler,"连接%s:%d",clientConfigure->server_ip,clientConfigure->server_port);
	
	free(clientConfigure->server_ip);
	free(clientConfigure);
    if (connect(sock_cli, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    {
        perror("connect");
		return 2;
        //exit(1);
    }
	
	connected = 1;
	pthread_cond_signal(para_t.cond_);

	//call_back buffer
	callback_buffer = (struct GW_buf*)malloc(sizeof(struct GW_buf));
	
	//init connection
	Mydevice = device;
	frameInfo* send_frame = clear_frameInfo();
	send_frame->action = GW_EVENT;
	send_frame->event = INIT_CONNECTION;

	send_frame->source = Mydevice;
	send_frame->dest   = SERVER;

	assembleFrame(send_frame);
 
	
	return 0;
}

int freeThread()
{
	close(sock_cli);
	pthread_cond_destroy(para_t.cond_);
    pthread_mutex_destroy(para_t.mutex_);
    free(para_t.cond_);
    free(para_t.mutex_);
    free(para_t.thread_);
	releaseCallbackFunc();
	if(recvbuf != NULL){	
		free(recvbuf);
	}
	if(sendMessage != NULL){
		free(sendMessage);
	}
	if(callback_buffer != NULL){
		free(callback_buffer);	
	}
	zlog_info(log_handler,"freeThread()");
	return 0;
}

int registerCallbackFunc(frcb_func read_fcb, void* rptr)
{
    callback = (mydata_t *)malloc(sizeof(mydata_t));
    callback->frcb = read_fcb;
	callback->jsonStr = NULL;
	callback->rptr = rptr;
    return 0;
}

void releaseCallbackFunc()
{
	free(callback);
}

frameInfo* clear_frameInfo(){
	frameInfo* input = (frameInfo *)malloc(sizeof(frameInfo));
	input->messageLength = 0;
	input->id = -1;
	input->action = 0;
	input->source = 0;
	input->dest = 0;
	input->event = 0;
	input->frame_buf = NULL;
	input->send_buf = NULL;
	return input;
}

// ----------------------------------- subscriber_Topic ----------------------------
int  subscriber_event(eventID event){
	frameInfo* send_frame = clear_frameInfo();
	send_frame->action = GW_EVENT;
	send_frame->event = SUBSCRIBE_TOPIC;
	send_frame->source = Mydevice;
	send_frame->dest   = SERVER;
	cJSON *temp_root = cJSON_CreateObject();
	cJSON_AddNumberToObject(temp_root, "topic_event", (int32_t)event); // decode by myself
	send_frame->send_buf = cJSON_Print(temp_root);
	assembleFrame(send_frame);
	cJSON_Delete(temp_root);
}

int  unsubscriber(eventID event){
	frameInfo* send_frame = clear_frameInfo();
	send_frame->action = GW_EVENT;
	send_frame->event = UNSUBSCRIBE_TOPIC;
	send_frame->source = Mydevice;
	send_frame->dest   = SERVER;
	cJSON *temp_root = cJSON_CreateObject();
	cJSON_AddNumberToObject(temp_root, "topic_event", (int32_t)event); // decode by myself
	send_frame->send_buf = cJSON_Print(temp_root);
	assembleFrame(send_frame);
	cJSON_Delete(temp_root);

}

void informServer(eventID event, char* param){
	frameInfo* send_frame = clear_frameInfo();
	send_frame->action = GW_EVENT;
	send_frame->event = event;
	send_frame->source = Mydevice;
	send_frame->dest   = SERVER;
	send_frame->send_buf = param;
	assembleFrame(send_frame);
}

void getFromDevice(deviceID dest, char* param){
	frameInfo* send_frame = clear_frameInfo();
	send_frame->action = GW_GET;
	send_frame->source = Mydevice;
	send_frame->dest   = dest;
	send_frame->send_buf = param;
	assembleFrame(send_frame);
}

void writeToDevice(deviceID dest, char* param){
	frameInfo* send_frame = clear_frameInfo();
	send_frame->action = GW_WRITE;
	send_frame->source = Mydevice;
	send_frame->dest   = dest;
	send_frame->send_buf = param;
	assembleFrame(send_frame);
}






