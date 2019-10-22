#ifndef IPC_SERVER_STUB_H
#define IPC_SERVER_STUB_H

#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/socket.h>  
#include <sys/un.h>
#include "zlog.h"
#include "ThreadPool.h"
#include "gw_ipc.h"
#include "hash_tbl.h"


typedef struct g_ipc_server_para{
    char* server_path;
    g_IPC_para* ipc_server;
    hash_tbl* g_tbl; // hash table for different process to index transfer path depends on air msg type
    int num_process;
    g_IPC_para* g_ipc_broker[32]; // manage at least 32 different process
    //ThreadPool*        g_threadpool;
}g_ipc_server_para;

typedef struct accept_para{
    g_ipc_server_para* g_ipc_server;
    int ipc_broker_idx;
}accept_para;

g_ipc_server_para* start_ipc_server_stub(char* server_path, g_ipc_server_para* g_ipc_server, zlog_category_t* handler);

int block_accept_loop(g_ipc_server_para* g_ipc_server);

int send_to_client(char* buf, int buf_len, char* proc_name, g_ipc_server_para* g_ipc_server);

#endif//IPC_SERVER_STUB_H
