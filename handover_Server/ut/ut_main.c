#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

#include <math.h>

#include <unistd.h>
#include <signal.h>
#include <sys/time.h>

#include "zlog.h"
#include "adlist.h"
#include "sds.h"
#include "dict.h"
#include "xmalloc.h"

#include "macros_util.h"

#include <sched.h>
#include <pthread.h>
#include <sys/syscall.h>

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

long long mytimeInMilliseconds(void) {
    struct timeval tv;

    gettimeofday(&tv,NULL);
    return (((long long)tv.tv_sec)*1000)+(tv.tv_usec/1000);
}

/*  ----------------------------------------------------------------------------------  */

int set_cpu(pid_t tid)
{
    cpu_set_t mask;

    int NUM_PROCS = sysconf(_SC_NPROCESSORS_CONF);

    int i = random() % NUM_PROCS;
        
    CPU_ZERO(&mask);

    CPU_SET(i, &mask);

    // if (-1 == sched_setaffinity(tid, sizeof(cpu_set_t), &mask))
    // {
    //     return -1;
    // }
	if (-1 == pthread_setaffinity_np(pthread_self(),sizeof(cpu_set_t),&mask))
	{
		return -1;
	}
    
    return 0;
}
 
int main(int argc, char *argv[]){

	zlog_category_t *zlog_handler = serverLog("../conf/zlog_default.conf");

 	int i, nrcpus;
	unsigned long bitmask = 0;
	cpu_set_t mask;

	pid_t tid = syscall(__NR_gettid);
	
	int state = set_cpu(tid);
	if(state == -1){
		printf("set_cpu error !\n");
		return 0;
	}

	/* get logical cpu number */
    nrcpus = sysconf(_SC_NPROCESSORS_CONF);

	state = pthread_getaffinity_np(pthread_self(), sizeof(cpu_set_t), &mask);
    if (state != 0)
    {
        printf("pthread_getaffinity_np : state = %d \n",state);
    }
    
	while(1){
		for (i = 0; i < nrcpus; i++)
		{
			if (CPU_ISSET(i, &mask))
			{
				bitmask |= (unsigned long)0x01 << i;
				printf("processor #%d is set\n", i); 
			}
		}
		printf("bitmask = %#lx\n", bitmask);
		usleep(1000000);
	}

	return 0;


}

/* --------------------------------------------------------------------------------------- */

// #include <stdio.h>
// #include <pthread.h>
// #include <sched.h>
// #include <assert.h>

// static int api_get_thread_policy (pthread_attr_t *attr)
// {
//     int policy;
//     int rs = pthread_attr_getschedpolicy (attr, &policy);
//     assert (rs == 0);

//     switch (policy)
//     {
//         case SCHED_FIFO:
//             printf ("policy = SCHED_FIFO\n");
//             break;
//         case SCHED_RR:
//             printf ("policy = SCHED_RR");
//             break;
//         case SCHED_OTHER:
//             printf ("policy = SCHED_OTHER\n");
//             break;
//         default:
//             printf ("policy = UNKNOWN\n");
//             break; 
//     }
//     return policy;
// }

// static void api_show_thread_priority (pthread_attr_t *attr,int policy)
// {
//     int priority = sched_get_priority_max (policy);
//     assert (priority != -1);
//     printf ("max_priority = %d\n", priority);
//     priority = sched_get_priority_min (policy);
//     assert (priority != -1);
//     printf ("min_priority = %d\n", priority);
// }

// static int api_get_thread_priority (pthread_attr_t *attr)
// {
//     struct sched_param param;
//     int rs = pthread_attr_getschedparam (attr, &param);
//     assert (rs == 0);
//     printf ("priority = %d\n", param.__sched_priority);
//     return param.__sched_priority;
// }

// static void api_set_thread_policy (pthread_attr_t *attr,int policy)
// {
//     int rs = pthread_attr_setschedpolicy (attr, policy);
//     assert (rs == 0);
//     api_get_thread_policy (attr);
// }
    
// int main(void)
// {
//     pthread_attr_t attr;       // 线程属性
//     struct sched_param sched;  // 调度策略
//     int rs;

//     /* 
//      * 对线程属性初始化
//      * 初始化完成以后，pthread_attr_t 结构所包含的结构体
//      * 就是操作系统实现支持的所有线程属性的默认值
//      */
//     rs = pthread_attr_init (&attr);
//     assert (rs == 0);     // 如果 rs 不等于 0，程序 abort() 退出

//     /* 获得当前调度策略 */
//     int policy = api_get_thread_policy (&attr);

//     /* 显示当前调度策略的线程优先级范围 */
//     printf ("Show current configuration of priority\n");
//     api_show_thread_priority(&attr, policy);

//     /* 获取 SCHED_FIFO 策略下的线程优先级范围 */
//     printf ("show SCHED_FIFO of priority\n");
//     api_show_thread_priority(&attr, SCHED_FIFO);

//     /* 获取 SCHED_RR 策略下的线程优先级范围 */
//     printf ("show SCHED_RR of priority\n");
//     api_show_thread_priority(&attr, SCHED_RR);

//     /* 显示当前线程的优先级 */
//     printf ("show priority of current thread\n");
//     int priority = api_get_thread_priority (&attr);

//     /* 手动设置调度策略 */
//     printf ("Set thread policy\n");

//     printf ("set SCHED_FIFO policy\n");
//     api_set_thread_policy(&attr, SCHED_FIFO);

//     printf ("set SCHED_RR policy\n");
//     api_set_thread_policy(&attr, SCHED_RR);

//     /* 还原之前的策略 */
//     printf ("Restore current policy\n");
//     api_set_thread_policy (&attr, policy);

//     /* 
//      * 反初始化 pthread_attr_t 结构
//      * 如果 pthread_attr_init 的实现对属性对象的内存空间是动态分配的，
//      * phread_attr_destory 就会释放该内存空间
//      */
//     rs = pthread_attr_destroy (&attr);
//     assert (rs == 0);

//     return 0;
// }




