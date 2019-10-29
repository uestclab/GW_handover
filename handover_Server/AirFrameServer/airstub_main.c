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
#include <assert.h>

#include "zlog.h"

//#include "cJSON.h"
#include "ThreadPool.h"
#include "air_process.h"
#include "ipc_server_stub.h"
#include <pthread.h>
#include "hash_tbl.h"


zlog_category_t * serverLog(const char* path){
	int rc;
	zlog_category_t *zlog_handler = NULL;
	rc = zlog_init(path);

	if (rc) {
		return NULL;
	}

	zlog_handler = zlog_get_category("airFrame");

	if (!zlog_handler) {
		zlog_fini();
		return NULL;
	}

	return zlog_handler;
}

void closeServerLog(){
	zlog_fini();
}



char *server_path = "server.socket";

int main(int argc, char *argv[]) // main thread
{
	if(argc == 2){
		if(argv[1] != NULL){
			if(strcmp(argv[1],"-v") == 0)
				printf("version : [%s %s] \n",__DATE__,__TIME__);
			else
				printf("error parameter\n");
			return 0;
		}
	}

	fflush(stdout);
    setvbuf(stdout, NULL, _IONBF, 0);

	zlog_category_t *zlog_handler = serverLog("../conf/zlog_default.conf");

	test_char_hash_tbl();

	test_int_hash_tbl();

	g_air_frame_para* g_air_frame = NULL;
	ThreadPool* g_threadpool = NULL;
	createThreadPool(1024, 8, &g_threadpool);
	int ret = initProcessAirThread(&g_air_frame, g_threadpool, zlog_handler);

	g_ipc_server_para* g_ipc_server = start_ipc_server_stub(server_path,&(g_air_frame->g_ipc_server),zlog_handler);

	startProcessAir(g_air_frame, 1);

	block_accept_loop(g_ipc_server);

	closeServerLog();

    return 0;
}




