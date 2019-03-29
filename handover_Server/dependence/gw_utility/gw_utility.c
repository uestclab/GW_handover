#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdint.h>
#include <assert.h>

#include <signal.h>
#include <sys/time.h>

#include <stdio.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>        //for struct ifreq
#include "gw_utility.h"


para_thread* newThreadPara(){

	para_thread* para_t = (para_thread*)malloc(sizeof(para_thread));
	para_t->thread_pid = (pthread_t*)malloc(sizeof(pthread_t));
	para_t->mutex_ = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(para_t->mutex_,NULL);
	para_t->cond_ = (pthread_cond_t*)malloc(sizeof(pthread_cond_t));
	pthread_cond_init(para_t->cond_,NULL);	
	return para_t;
}

void destoryThreadPara(para_thread* para_t){
	pthread_cond_destroy(para_t->cond_);
    pthread_mutex_destroy(para_t->mutex_);
    free(para_t->cond_);
    free(para_t->mutex_);
    free(para_t->thread_pid);
	para_t = NULL;
}

int32_t myNtohl(const char* buf){
	int32_t be32 = 0;
	memcpy(&be32, buf, sizeof(be32));
	return ntohl(be32);
}

int filelength(FILE *fp)
{
	int num;
	fseek(fp,0,SEEK_END);
	num=ftell(fp);
	fseek(fp,0,SEEK_SET);
	return num;
}

char* readfile(const char *path)
{
	FILE *fp;
	int length;
	char *ch;
	if((fp=fopen(path,"r"))==NULL)
	{
		return NULL;
	}
	length=filelength(fp);
	ch=(char *)malloc(length+1);
	fread(ch,length,1,fp);
	*(ch+length)='\0';
	fclose(fp);
	return ch;
}


//	===========================
	/*	
	int64_t start = now();
	.....
	int64_t end = now();
	double sec = (end-start)/1000000.0;
	printf("%f sec %f ms \n", sec, 1000*sec);
	*/
//  ===========================
int64_t now()
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec * 1000000 + tv.tv_usec;
}


void hexdump(const void* p, size_t size, zlog_category_t *zlog_handler) {
	const uint8_t *c = p;
	assert(p);

	printf("Dumping %u bytes from %p:\n", (unsigned int)size, p);

	while (size > 0) {
		unsigned i;

		for (i = 0; i < 16; i++) {
			if (i < size)
				printf("%02x ", c[i]);
			else
				printf("  ");
		}

		for (i = 0; i < 16; i++) {
			if (i < size)
				printf("%c", c[i] >= 32 && c[i] < 127 ? c[i] : '.');
			else
				printf(" ");
		}

		printf("\n");

		c += 16;

		if (size <= 16)
		break;

		size -= 16;
	}
} 


int get_mac(char * mac, int len_limit, char *arg)
{
    struct ifreq ifreq;
    int sock;

    if ((sock = socket (AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror ("socket");
        return -1;
    }
    strcpy (ifreq.ifr_name, arg);

    if (ioctl (sock, SIOCGIFHWADDR, &ifreq) < 0)
    {
        perror ("ioctl");
        return -1;
    }

	close(sock);    

    return snprintf (mac, len_limit, "%02x%02x%02x%02x%02x%02x", (unsigned char) ifreq.ifr_hwaddr.sa_data[0], (unsigned char) ifreq.ifr_hwaddr.sa_data[1], (unsigned char) ifreq.ifr_hwaddr.sa_data[2], (unsigned char) ifreq.ifr_hwaddr.sa_data[3], (unsigned char) ifreq.ifr_hwaddr.sa_data[4], (unsigned char) ifreq.ifr_hwaddr.sa_data[5]);
}

// in_addr[12] , out_addr[6] : 000c29d46f68 , 0
void change_mac_buf(char* in_addr, char* out_addr){
	int i = 0;
	char number = 0;
	char temp = 0;
	int cnt = 0;	
	for(i=0;i<12;i++){
		if(in_addr[i]>='0'&&in_addr[i]<='9')
			temp = in_addr[i] - '0';
		else if(in_addr[i]>='a'&&in_addr[i]<='f')
			temp = in_addr[i] - 'a' + 10;
		else if(in_addr[i]>='A'&&in_addr[i]<='F')
			temp = in_addr[i] - 'A' + 10;
		
		if(i%2 == 1){
			number = number * 16 + temp;
			out_addr[cnt] = number;
			number = 0;
			cnt = cnt + 1;
		}else{
			number = number + temp;
		}
	}
}




/* 
	---------------------- timer -------------------------------------
*/




/* ==== temp */

void user_wait()
{
	int c;
	printf("user_wait... \n");
	do
	{
		c = getchar();
		if(c == 'g') break;
	} while(c != '\n');
}

void gw_sleep(){
	sleep(1);
}















