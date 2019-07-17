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

int gpio_read(int pin)
{  

    char path[64];  

    char value_str[3];  

    int fd;  

  
	/* /sys/class/gpio/gpio973/value */
    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/value", pin);  

    fd = open(path, O_RDONLY);  

    if (fd < 0){
        printf("Failed to open gpio value for reading!\n");
        return -1;
    }  

    if (read(fd, value_str, 3) < 0){
        printf("Failed to read value!\n");
        return -1;
    }
    close(fd);
    return (atoi(value_str));
}  

#include <stdio.h>
#include <pthread.h>
static int testcount = 0;
static void *test_thread_handler()
{
	pthread_detach(pthread_self());
    testcount++;
    printf("pthread_detach : %d\n",testcount);
	pthread_exit(0);
    return 0;
}

int main(int argc, char *argv[]){
/*

	zlog_category_t *handler = serverLog("../conf/zlog_default.conf");


	printf(" +++++++++++++++++++++++++++++ start ut_main ++++++++++++++++++++++++++++++++++++++++++++++ \n");

	ThreadPool* g_threadpool = NULL;
	createThreadPool(4096, 8, &g_threadpool);
	
		

	for(int32_t id = 0; id < 20; id++)
		WorkToThreadPool(id, g_threadpool);


	while(1){
		int value = gpio_read(973);
		printf("value = %d \n" , value);
		user_wait();
	}
	printf("------------------- end ----------------------------------\n");
*/
    int ret = 0;
    pthread_t test_tid;
    while(1)
    {
        usleep(10000);
        ret = pthread_create(&test_tid, NULL, test_thread_handler,NULL);
        if(ret != 0)
        {
            printf("Create handler error :%d!\t testcount:%d\n", ret, testcount);
            return -1;
        }
    }
    return 0;
}














