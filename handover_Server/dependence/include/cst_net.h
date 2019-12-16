#ifndef	__LIB_AXIDMA_H__

#define	__LIB_AXIDMA_H__
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <netdb.h>
#include <setjmp.h>
#include <signal.h>
#include <paths.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stddef.h>
#include <string.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <getopt.h>
#include <time.h>
#include <pthread.h>
#include <sys/wait.h>


#define	WAITE_TIMEOUT	10 //s
// The standard name for the AXI DMA device
#define AXIDMA_DEV_NAME     "cst"

// The standard path to the AXI DMA device
#define AXIDMA_DEV_PATH     ("/dev/" AXIDMA_DEV_NAME)


#define CST_IOCTL_MAGIC              'W'

#define CST_START                    _IOW(CST_IOCTL_MAGIC, 0, int)


typedef int (*recv_cb)(char* buf, int buf_len, void* arg); 


struct axidma_mmp {    

};


struct axidma_dev {    
	int fd;                     ///< File descriptor for the device    
	pthread_t recv_pid;

	recv_cb	recv_callback;
	void *user_arg;
};

static inline void* xmalloc(size_t size)
{
	void *ptr = malloc(size);
	return ptr;
}

#define	xfree(ptr) do{ \
	if(ptr){ \
		free(ptr); \
		ptr = NULL; \
	} \
}while(0)


void *axidma_open(void);
void axidma_close(void *dev);
void *axidma_malloc(void *dev, size_t size);
struct axidma_mmp *axidma_mmp(void *dev, size_t size);
int axidma_unmmp(void *dev, size_t size);
int axidma_start(void *dev);
int axidma_stop(void *dev);
int axidma_register_callback(void *dev, recv_cb cb, void* arg);
int axidma_reserve(int byte);
int axidma_chan(void *dev, int chan);

#endif

