#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

#include <math.h>

#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include "gw_utility.h"
#include "gw_control.h"
#include "zlog.h"
#include "regdev_common.h"
#include "ThreadPool.h"

#define	REG_PHY_ADDR	0x43C20000
#define	REG_MAP_SIZE	0X10000

#define GET_SIZE        300

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

typedef struct id_Info{
	int32_t id;
}id_Info;

void* test_process_thread(void* arg){
	id_Info* tmp = (id_Info*)arg;
	uint32_t sum = 0;
	for(int i = 0; i < 100000; i++)
		for(int j = 0; j < 10000; j++)
			sum = sum + i + j;
	printf("call id = %d sum = %u \n", tmp->id, sum);
}

void WorkToThreadPool(int32_t id, ThreadPool* g_threadpool){
	id_Info* tmp = (id_Info*)malloc(sizeof(id_Info));
	tmp->id = id;
	AddWorker(test_process_thread,(void*)tmp,g_threadpool);
} 


int main(int argc, char *argv[]){

	zlog_category_t *handler = serverLog("../conf/zlog_default.conf");


	printf(" +++++++++++++++++++++++++++++ start ut_main ++++++++++++++++++++++++++++++++++++++++++++++ \n");
	/* ThreadPool handler */
	ThreadPool* g_threadpool = NULL;
	createThreadPool(4096, 8, &g_threadpool);
	
		

	for(int32_t id = 0; id < 20; id++)
		WorkToThreadPool(id, g_threadpool);

	
	user_wait();
	printf("------------------- end ----------------------------------\n");
	
}














