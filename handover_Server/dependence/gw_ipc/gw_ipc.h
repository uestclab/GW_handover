#ifndef GW_IPC_H
#define GW_IPC_H

#include "zlog.h"
#include <stdint.h>
#include <poll.h>
#include <sys/ioctl.h>

typedef enum ipc_type{
	SOURCE_PROCESS = 1, // client to server , distinguish different process
    CJSON, // 
    RAW_FRAME_DATA,
	HEART_BEAT, // server send to client
	HEART_BEAT_RESPONSE,
}ipc_type;

typedef int (*recv_cb)(char* buf, int buf_len, void* from, int type);

typedef int (*accept_cb)(int connfd, void* from, void* out);

typedef struct ipc_cb_para{
	recv_cb      cb_recv; // callback in receive
	accept_cb    cb_accept; // callback in accept
	void*        cb_para;
}ipc_cb_para;

typedef struct g_IPC_para{
	char*        client_path;
	char*        server_path;
	int          listenfd; // ipc_server listen use
	int          num_process;
	int          sockfd;   // ipc_server or ipc_client transfer data use
	struct pollfd poll_fd;
	int          gMoreData_;
	char*        recvbuf;
	char*        send_buf;
	ipc_cb_para*     cb_info;
	zlog_category_t*   log_handler;
}g_IPC_para;

void register_cb_para(recv_cb cb_recv, accept_cb cb_accept, void* cb_para, g_IPC_para* g_IPC);

int init_ipc(char* client_path, char* server_path, g_IPC_para** g_IPC, zlog_category_t* handler);

g_IPC_para* init_ipc_broker(int connfd, zlog_category_t* handler);

int connect_server(g_IPC_para* g_IPC);

int build_server(g_IPC_para* g_IPC);

int wait_accept_loop(g_IPC_para* g_IPC);

void close_ipc(g_IPC_para* g_IPC);

// instead of gw_monitor_poll and add callback function
int ipc_poll_receive(g_IPC_para* g_IPC, int time_cnt);

int ipc_receive(g_IPC_para* g_IPC);

// instead of handle_air_tx
int ipc_send(char* buf, int buf_len, int type, int connfd, g_IPC_para* g_IPC); 

#endif//GW_IPC_H
