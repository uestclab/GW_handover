#define _GNU_SOURCE
// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <stdint.h>
// #include <assert.h>

// #include <math.h>

// #include <unistd.h>
// #include <signal.h>
// #include <sys/time.h>

// #include "zlog.h"

// #include <sched.h>
// #include <pthread.h>
// #include <sys/syscall.h>

// #include <stddef.h>  
// #include <sys/socket.h>  
// #include <sys/un.h>  
// #include <errno.h> 
// #include <ctype.h>

#include <stdlib.h>  
#include <stdio.h>  
#include <stddef.h>  
#include <sys/socket.h>  
#include <sys/un.h>  
#include <errno.h>  
#include <string.h>  
#include <unistd.h>
#include <pthread.h>
#include <assert.h>
#include "zlog.h"
#include "gw_ipc.h"
#include "gw_frame.h"
#include "gw_utility.h"
#include "msg_queue.h"

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

// char *client_path = "client.socket";  
// char *server_path = "/tmp/air_test/server/bin/server.socket";


// char proc_name_g[32];

// /* ------------  ipc_client  ----------------  */

// void* ipc_receive_thread(void* args){
// 	g_IPC_para* g_ipc_client = (g_IPC_para*)args;
// 	int ret;
// 	while(1){  
// 		ret = ipc_receive(g_ipc_client);  
// 		if (ret < 0) {
// 			break;  
// 		} else if(ret == 0) {  
// 			;//zlog_info(g_ipc_client->log_handler,"decode successful \n");
// 		}
// 	}
// }

// // receive air frame
// int process_ipc_msg(char* buf, int buf_len, void* input, int type){
// 	g_IPC_para* g_ipc_client = (g_IPC_para*)input;

// 	assert(buf_len == sizeof(management_frame_Info));
// 	management_frame_Info* frame_Info = (management_frame_Info*)buf;

// 	zlog_info(g_ipc_client->log_handler,"receive air frame : subtype = %d , buf_len = %d , type = %d , t_id = %lu\n", 
// 	frame_Info->subtype, buf_len, type, pthread_self());
// 	// hexdump(frame_Info->source_mac_addr, 6);
// 	// hexdump(frame_Info->dest_mac_addr, 6);
// 	// hexdump(frame_Info->Next_dest_mac_addr, 6);

// 	if(strcmp(proc_name_g,"baseStation") == 0){
// 		char mac_buf[6] = {0};
// 		char mac_buf_dest[6] = {0};
// 		char mac_buf_next[6] = {0};
// 		management_frame_Info* frame_Info = new_air_frame(DEASSOCIATION, 0,mac_buf,mac_buf_dest,mac_buf_next,1);   
// 		int ret = ipc_send((char*)frame_Info, sizeof(management_frame_Info), RAW_FRAME_DATA, g_ipc_client->sockfd, g_ipc_client);
// 		if(ret != sizeof(management_frame_Info) + 8){
// 			zlog_error(g_ipc_client->log_handler,"ipc_send error \n");
// 		}
// 		free(frame_Info);
// 	}

// 	// post msg to queue

// 	return 0;
// }

// /* notify process type to server */
// int notify_to_server(char* proc_name, g_IPC_para* g_IPC){
// 	int name_len = strlen(proc_name) + 1;
// 	int ret = ipc_send(proc_name, name_len, SOURCE_PROCESS, g_IPC->sockfd, g_IPC);
// 	if(ret != name_len + 8){
// 		zlog_error(g_IPC->log_handler,"notify_to_server error \n");
// 		return -1;
// 	}else{
// 		return 0;
// 	}
// }

// int main(int argc, char *argv[]) // main thread
// {
// 	zlog_category_t *zlog_handler = serverLog("../conf/zlog_default.conf");

// 	zlog_info(zlog_handler,"start ipc client --- file : %s , proc_name : %s , main_tid = %lu \n", 
// 	argv[1], argv[2], pthread_self());

// 	g_IPC_para* g_ipc_client = NULL;
// 	int ret = init_ipc(argv[1], server_path, &g_ipc_client, zlog_handler);
// 	register_cb_para(process_ipc_msg, NULL, (void*)(g_ipc_client), g_ipc_client);

// 	ret = connect_server(g_ipc_client);
// 	if(ret != 0){
// 		zlog_error(zlog_handler,"connect server error\n");
// 		return 0;
// 	}


// 	char* proc_name = (char*)malloc(32); 
// 	memcpy(proc_name,argv[2],strlen(argv[2])+1);//"baseStation";
// 	memcpy(proc_name_g,proc_name,strlen(proc_name)+1);
// 	ret = notify_to_server(proc_name, g_ipc_client);
// 	if(ret != 0){
// 		printf("... exit after notify_to_server \n");
// 		return 0;
// 	}

// 	zlog_info(zlog_handler,"start to work ..... \n");

// 	pthread_t thread_id;
// 	ret = pthread_create(&thread_id, NULL, ipc_receive_thread, (void*)g_ipc_client);
// 	uint16_t send_id = 0;
// 	char mac_buf[6];
// 	char mac_buf_dest[6];
// 	char mac_buf_next[6];
// 	memset(mac_buf,0,6);
// 	memset(mac_buf_dest,0,6);
// 	memset(mac_buf_next,0,6);
// 	if(strcmp(proc_name,"baseStation") == 0){
// 		mac_buf_next[2] = 14;
// 	}else if(strcmp(proc_name,"vehicle") == 0){
// 		mac_buf_next[2] = 22;
// 	}
// 	while(1){
// 		// thread safe interface to call ipc_send
// 		if(strcmp(proc_name,"baseStation") == 0){
// 			sleep(10);
// 		}else if(strcmp(proc_name,"vehicle") == 0){
// 			management_frame_Info* frame_Info = new_air_frame(BEACON, 0,mac_buf,mac_buf_dest,mac_buf_next,send_id);   
// 			ret = ipc_send((char*)frame_Info, sizeof(management_frame_Info), RAW_FRAME_DATA, g_ipc_client->sockfd, g_ipc_client);
// 			if(ret != sizeof(management_frame_Info) + 8){
// 				zlog_error(zlog_handler,"ipc_send error \n");
// 			}
// 			sleep(5);
// 		}
// 		send_id = send_id + 1;
//     }

//     return 0;  
// }


int main(int argc, char *argv[])
{
	zlog_category_t *zlog_handler = serverLog("../conf/zlog_default.conf");

	/* msg_queue 0*/
	const char* pro_path = "/tmp/handover_test/";
	int proj_id = 'a';
	g_msg_queue_para* g_msg_queue = createMsgQueue(pro_path, proj_id, zlog_handler);
	if(g_msg_queue == NULL){
		zlog_info(zlog_handler,"No msg_queue created \n");
		return 0;
	}
	zlog_info(zlog_handler, "g_msg_queue->msgid = %d \n", g_msg_queue->msgid);

	int state = clearMsgQueue(g_msg_queue);

	/* 1 */
	const char* pro_path_1 = "/tmp/handover_test/";
	int proj_id_1 = 'b';
	g_msg_queue_para* g_msg_queue_1 = createMsgQueue(pro_path_1, proj_id_1, zlog_handler);
	if(g_msg_queue_1 == NULL){
		zlog_info(zlog_handler,"No msg_queue created \n");
		return 0;
	}
	zlog_info(zlog_handler, "g_msg_queue->msgid = %d \n", g_msg_queue_1->msgid);

	state = clearMsgQueue(g_msg_queue_1);

	/* 2 */
	const char* pro_path_2 = "/tmp/handover_test/";
	int proj_id_2 = 'c';
	g_msg_queue_para* g_msg_queue_2 = createMsgQueue(pro_path_2, proj_id_2, zlog_handler);
	if(g_msg_queue_2 == NULL){
		zlog_info(zlog_handler,"No msg_queue created \n");
		return 0;
	}
	zlog_info(zlog_handler, "g_msg_queue->msgid = %d \n", g_msg_queue_2->msgid);

	state = clearMsgQueue(g_msg_queue_2);


}