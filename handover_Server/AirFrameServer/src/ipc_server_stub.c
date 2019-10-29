#include "ipc_server_stub.h"
#include <pthread.h>
#include "air_process.h"

/* ------------  ipc_server  ----------------  */

int send_to_client(char* buf, int buf_len, char* proc_name, g_ipc_server_para* g_ipc_server){	
	int ipc_broker_idx = -1;
	if(search_hash_tbl(g_ipc_server->g_tbl, proc_name)){
		ipc_broker_idx = *((int*)search_hash_tbl(g_ipc_server->g_tbl,proc_name));
		zlog_info(g_ipc_server->ipc_server->log_handler,"%s-%d\n", proc_name, ipc_broker_idx);
		g_IPC_para* g_ipc_broker = g_ipc_server->g_ipc_broker[ipc_broker_idx];
		int ret = ipc_send(buf, buf_len, CJSON, g_ipc_broker->sockfd, g_ipc_broker);
		if(ret != buf_len + 8){
			zlog_error(g_ipc_server->ipc_server->log_handler, "ipc_send error , ret = %d \n", ret);
		}
		return ret;
	}else{
		zlog_error(g_ipc_server->ipc_server->log_handler,"this proc_name is not valid");
		return -1;
	}
}


void* ipc_receive_thread(void* args){
	g_IPC_para* g_ipc_client = (g_IPC_para*)args;
	int ret;
	while(1){  
		ret = ipc_poll_receive(g_ipc_client,2); // 2 * 5ms  
		if (ret < 0) {
			zlog_info(g_ipc_client->log_handler,"ipc poll time out : ret = %d \n", ret); 
		} else if(ret == 0) {  
			zlog_info(g_ipc_client->log_handler,"ipc_poll_receive : ret = 0 \n");
		}
	}
}

void* process_thread(void* args){
	g_IPC_para* g_ipc_broker = (g_IPC_para*)args;
	int ret;
	while(1){  
		ret = ipc_poll_receive(g_ipc_broker,2);// 2 * 5ms  
		if (ret < 0) {  
			//zlog_info(g_ipc_broker->log_handler,"ipc poll time out : ret = %d \n", ret); 
		} else if(ret == 0) {  
			zlog_info(g_ipc_broker->log_handler,"ipc_poll_receive : ret = 0 \n");
		}
	}  
}

int process_ipc_msg(char* buf, int buf_len, void* input, int type){
	accept_para* accept_parameter = (accept_para*)input;
	g_ipc_server_para* g_ipc_server = accept_parameter->g_ipc_server;
	int ipc_broker_idx = accept_parameter->ipc_broker_idx;
	g_IPC_para* g_ipc_broker = accept_parameter->g_ipc_server->g_ipc_broker[ipc_broker_idx];

	zlog_info(g_ipc_server->ipc_server->log_handler,"client call : process_ipc_msg : buf_len = %d , type = %d \n", buf_len, type);
    if( type == RAW_FRAME_DATA || type == CJSON){
		/* call air send interface --- thread safe */
		int ret = send_airFrame(buf, buf_len, g_ipc_server);
		// ......

	}else if(type == HEART_BEAT){
		;
	}else if(type == HEART_BEAT_RESPONSE){
		;
	}else if(type == SOURCE_PROCESS){
		char key[128];
		sprintf(key, "%s", buf);
		insert_hash_tbl(g_ipc_server->g_tbl, key, (void*)(&ipc_broker_idx), sizeof(int));

		if(search_hash_tbl(g_ipc_server->g_tbl, key)){
		    int* tmp_val = (int*)search_hash_tbl(g_ipc_server->g_tbl,key);
		}
	}else{
		zlog_info(g_ipc_server->ipc_server->log_handler," type is not valid:  %d ", type);
	}

	return 0;
}

int new_process_accept(int connfd, void* input, void* out){
	g_ipc_server_para* g_ipc_server = (g_ipc_server_para*)input;

	zlog_info(g_ipc_server->ipc_server->log_handler,"new_process_accept : connfd = %d \n", connfd);

	g_IPC_para* g_ipc_broker = init_ipc_broker(connfd,g_ipc_server->ipc_server->log_handler);

	g_ipc_server->g_ipc_broker[g_ipc_server->num_process] = g_ipc_broker;
	accept_para* accept_parameter = (accept_para*)malloc(sizeof(accept_para));
	accept_parameter->g_ipc_server = g_ipc_server;
	accept_parameter->ipc_broker_idx = g_ipc_server->num_process;
	register_cb_para(process_ipc_msg, NULL, (void*)accept_parameter, g_ipc_broker);
	g_ipc_server->num_process = g_ipc_server->num_process + 1;
	pthread_t thread_id;
	int ret = pthread_create(&thread_id, NULL, process_thread, (void*)g_ipc_broker);
	return 0;
}

/* accept loop , block function */
g_ipc_server_para* start_ipc_server_stub(char* server_path, g_ipc_server_para* g_ipc_server, zlog_category_t* handler){
	int ret = init_ipc(NULL, server_path, &(g_ipc_server->ipc_server), handler);
	register_cb_para(NULL, new_process_accept, (void*)g_ipc_server, g_ipc_server->ipc_server);
	ret = build_server(g_ipc_server->ipc_server);
	if(ret != 0){
		zlog_error(handler,"build server error\n");
		return NULL;
	}

	g_ipc_server->num_process = 0;
	g_ipc_server->g_tbl = (hash_tbl*)malloc(sizeof(hash_tbl));
    init_hash_tbl(g_ipc_server->g_tbl);

	return g_ipc_server;
}

int block_accept_loop(g_ipc_server_para* g_ipc_server){
	int ret = wait_accept_loop(g_ipc_server->ipc_server);
	return ret;
}

