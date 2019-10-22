#ifndef AIR_PROCESS_H
#define AIR_PROCESS_H
#include "gw_frame.h"
#include "gw_utility.h"
#include "ipc_server_stub.h"
#include "ThreadPool.h"
#include "gw_macros_util.h"

typedef struct g_air_frame_para{
	g_args_frame*      g_frame;
	g_ipc_server_para  g_ipc_server; // use container_of ----- ! only one copy in all this program
	ThreadPool*        g_threadpool;
	uint16_t           send_id;
	int32_t            running;
	para_thread*       para_t;
	para_thread*       send_para_t;
	zlog_category_t*   log_handler;
}g_air_frame_para;

typedef struct g_transfer_para{
	management_frame_Info* tranfer_buf;
	g_air_frame_para*      g_air_frame;
}g_transfer_para;


int initProcessAirThread(g_air_frame_para** g_air_frame, ThreadPool* g_threadpool, zlog_category_t* handler);

void startProcessAir(g_air_frame_para* g_air_frame, int running_step);

int send_airFrame(char* buf, int buf_len, g_ipc_server_para* g_ipc_server_var);

#endif//AIR_PROCESS_H
