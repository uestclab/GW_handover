#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <stddef.h>  
#include <sys/socket.h>  
#include <sys/un.h>  
#include <errno.h>
#include <netinet/in.h>

#include "gw_utility.h"
#include "gw_ipc.h"

// -------------------------------------- 
/* callback interface */
void NotifyMessageComing(char* start, int32_t length, g_IPC_para* g_IPC){
	int32_t msg_type = myNtohl(start+sizeof(int32_t));
	int buf_len = length - sizeof(int32_t);
	char* buf = start + sizeof(int32_t)*2;
	zlog_info(g_IPC->log_handler,"NotifyMessageComing : msg_type = %d , buf_len = %d \n", msg_type, buf_len);
	g_IPC->cb_info->cb_recv(buf,buf_len,g_IPC->cb_info->cb_para,msg_type);
    if( msg_type == RAW_FRAME_DATA || msg_type == CJSON){
		;
	}else if(msg_type == HEART_BEAT){
		;
	}else if(msg_type == HEART_BEAT_RESPONSE){
		;
	}else if(msg_type == SOURCE_PROCESS){
		;
	}else{
		zlog_info(g_IPC->log_handler," msg_type is not valid:  %d ", msg_type);
	}
}

/* receive interface */
int ipc_poll_receive(g_IPC_para* g_IPC, int time_cnt){
	int loop = 0;
	int rc = 0;
	int ret = -1;
	for(loop = 0; loop <time_cnt; loop++){
		rc = poll(&(g_IPC->poll_fd),1,5); // ms
		if(rc > 0){
			if(POLLIN == g_IPC->poll_fd.revents){
				rc = ipc_receive(g_IPC);
				break;
			}else{
				ret = -2;
			}
		}else{
			ret = -3;
		}
	}// for
	if(loop == time_cnt)
		return ret;
	else
		return rc;
}



int ipc_receive(g_IPC_para* g_IPC){
	int n;
	int size = 0;
	int totalByte = 0;
	int messageLength = 0;
	
	char* temp_receBuffer = g_IPC->recvbuf + 100; //
	char* pStart = NULL;
	char* pCopy = NULL;

	n = read(g_IPC->sockfd, temp_receBuffer, 1024); // block 
	if(n<=0){
		return n;
	}
	size = n;
	// if(g_IPC->gMoreData_ != 0){
	// 	printf("size = %d , g_IPC->gMoreData_ = %d \n", size, g_IPC->gMoreData_);
	// }
	pStart = temp_receBuffer - g_IPC->gMoreData_;
	totalByte = size + g_IPC->gMoreData_;
	
	const int MinHeaderLen = sizeof(int32_t);

	while(1){
		if(totalByte <= MinHeaderLen)
		{
			g_IPC->gMoreData_ = totalByte;
			pCopy = pStart;
			if(g_IPC->gMoreData_ > 0)
			{
				memcpy(temp_receBuffer - g_IPC->gMoreData_, pCopy, g_IPC->gMoreData_);
			}
			break;
		}
		if(totalByte > MinHeaderLen)
		{
			messageLength= myNtohl(pStart);

			if(totalByte < messageLength + MinHeaderLen )
			{
				g_IPC->gMoreData_ = totalByte;
				pCopy = pStart;
				if(g_IPC->gMoreData_ > 0){
					memcpy(temp_receBuffer - g_IPC->gMoreData_, pCopy, g_IPC->gMoreData_);
				}
				break;
			} 
			else// at least one message 
			{
				NotifyMessageComing(pStart, messageLength, g_IPC);
				// move to next message
				pStart = pStart + messageLength + MinHeaderLen;
				totalByte = totalByte - messageLength - MinHeaderLen;
				if(totalByte == 0){
					g_IPC->gMoreData_ = 0;
					break;
				}
			}         
		}
	}
	return 0;
}

void register_cb_para(recv_cb cb_recv, accept_cb cb_accept, void* cb_para, g_IPC_para* g_IPC){
	ipc_cb_para* cb_info = (ipc_cb_para*)malloc(sizeof(ipc_cb_para));
	cb_info->cb_accept = cb_accept;
	cb_info->cb_recv = cb_recv;
	cb_info->cb_para = cb_para; //// notice : NULL pointer use

	g_IPC->cb_info = cb_info;
}

int init_ipc(char* client_path, char* server_path, g_IPC_para** g_IPC, zlog_category_t* handler){
	*g_IPC = (g_IPC_para*)malloc(sizeof(g_IPC_para));
	(*g_IPC)->client_path = client_path;
	(*g_IPC)->server_path = server_path;
	(*g_IPC)->sockfd = -1;
	(*g_IPC)->listenfd = -1;
	(*g_IPC)->num_process = -1;
	(*g_IPC)->recvbuf = (char*)malloc(1024);
	(*g_IPC)->send_buf = (char*)malloc(1024);
	(*g_IPC)->log_handler = handler;
	(*g_IPC)->gMoreData_ = 0;
	(*g_IPC)->cb_info = NULL;

	return 0;
}

g_IPC_para* init_ipc_broker(int connfd, zlog_category_t* handler){
	g_IPC_para* g_IPC = NULL;
	init_ipc(NULL,NULL,&g_IPC,handler);
	g_IPC->sockfd = connfd;

	g_IPC->poll_fd.fd = connfd;
	g_IPC->poll_fd.events=POLLIN;

	return g_IPC;
}

int connect_server(g_IPC_para* g_IPC){
	struct  sockaddr_un cliun, serun;  
    int len;
 
    if ((g_IPC->sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0){  
        zlog_error(g_IPC->log_handler,"client socket error");  
        return -1; 
    }  
      
    memset(&cliun, 0, sizeof(cliun));  
    cliun.sun_family = AF_UNIX;  
    strcpy(cliun.sun_path, g_IPC->client_path);  
    len = offsetof(struct sockaddr_un, sun_path) + strlen(cliun.sun_path);  
    unlink(cliun.sun_path);  
    if (bind(g_IPC->sockfd, (struct sockaddr*)&cliun, len) < 0) {  
        zlog_error(g_IPC->log_handler,"bind error");  
        return -2;  
    }  
 
    memset(&serun, 0, sizeof(serun));  
    serun.sun_family = AF_UNIX;  
    strcpy(serun.sun_path, g_IPC->server_path);  
    len = offsetof(struct sockaddr_un, sun_path) + strlen(serun.sun_path);

	int connect_stat = connect(g_IPC->sockfd, (struct sockaddr*)&serun, len);  
    while (connect_stat < 0){  
        perror("ipc server connect failed"); 
		gw_sleep();
		connect_stat = connect(g_IPC->sockfd, (struct sockaddr*)&serun, len);
		zlog_error(g_IPC->log_handler,"No ipc server , connect is failure");
    }

	// connect success , next start to receive and init link info

	g_IPC->poll_fd.fd = g_IPC->sockfd;
	g_IPC->poll_fd.events = POLLIN;


	return 0;
}
  
int build_server(g_IPC_para* g_IPC){
    struct sockaddr_un serun, cliun;
    int size;  
 
    if ((g_IPC->listenfd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {  
        zlog_error(g_IPC->log_handler, "socket error");  
		return -1;
    }  
 
    memset(&serun, 0, sizeof(serun));  
    serun.sun_family = AF_UNIX;  
    strcpy(serun.sun_path, g_IPC->server_path);  
    size = offsetof(struct sockaddr_un, sun_path) + strlen(serun.sun_path);  
    unlink(g_IPC->server_path);  
    if (bind(g_IPC->listenfd, (struct sockaddr *)&serun, size) < 0) {  
        zlog_error(g_IPC->log_handler,"bind error");  
        return -2;  
    }  
    zlog_info(g_IPC->log_handler,"UNIX domain socket bound\n");  
      
    if (listen(g_IPC->listenfd, 20) < 0) {  
        zlog_error(g_IPC->log_handler,"listen error");  
        return -3;          
    }
	g_IPC->num_process = 0; // init number of process in ipc server
    zlog_info(g_IPC->log_handler, "Accepting connections ...\n");  
 
	return 0;
}

int wait_accept_loop(g_IPC_para* g_IPC){
	int connfd;
	struct sockaddr_un cliun;
	socklen_t cliun_len;
    while(1){
        cliun_len = sizeof(cliun);
		connfd = accept(g_IPC->listenfd, (struct sockaddr *)&cliun, &cliun_len);         
        if( connfd < 0){  
            zlog_error(g_IPC->log_handler, "accept error");  
            continue;
        }else{
			g_IPC->num_process = g_IPC->num_process + 1;
			//printf("new process coming .. : num_process = %d \n",g_IPC->num_process);
			g_IPC->cb_info->cb_accept(connfd,g_IPC->cb_info->cb_para,NULL);
		}	
	}
    return 0;   
}

void close_ipc(g_IPC_para* g_IPC){
	free(g_IPC->recvbuf);
	free(g_IPC->send_buf);
	close(g_IPC->sockfd);
	g_IPC->gMoreData_ = 0;
}

int ipc_send(char* buf, int buf_len, int msg_type, int connfd, g_IPC_para* g_IPC){
	*((int32_t*)(g_IPC->send_buf+sizeof(int32_t))) = htonl(msg_type);
	int32_t messagelength = sizeof(int32_t) + buf_len;
	*((int32_t*)(g_IPC->send_buf)) = htonl(messagelength);
	memcpy(g_IPC->send_buf + sizeof(int32_t) * 2, buf, buf_len);
	//free(buf);
	zlog_info(g_IPC->log_handler,"ipc_send: msglen = %d , msg_type = %d \n", messagelength,msg_type);
	int status = write(connfd, g_IPC->send_buf , messagelength + sizeof(int32_t));
	if(status != messagelength + sizeof(int32_t)){
		zlog_error(g_IPC->log_handler,"Error in ipc socket: send length = %d ", status);
	}
	return status;
}









