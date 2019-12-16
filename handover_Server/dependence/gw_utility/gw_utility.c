#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
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
	free(para_t); // mem leak clean
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

char *get_prog_name(char *argv)
{
	int len = strlen(argv);
	int i;
	char *tmp = argv;
	
	for(i=len; i >=0; i--)
	{
		if(tmp[i] == '/'){
			i++;
			break;
		}
	}
	
	if(-1 == i){
		i = 0;
	}

	return argv + i;
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


void hexdump(const void* p, size_t size) {
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

void hexdump_zlog(const void* p, size_t size, zlog_category_t *zlog_handler) {
	const uint8_t *c = p;
	assert(p);

	zlog_info(zlog_handler,"Dumping %u bytes from %p:\n", (unsigned int)size, p);

	while (size > 0) {
		unsigned i;

		for (i = 0; i < 16; i++) {
			if (i < size)
				zlog_info(zlog_handler,"%02x ", c[i]);
			else
				zlog_info(zlog_handler,"  ");
		}

		for (i = 0; i < 16; i++) {
			if (i < size)
				zlog_info(zlog_handler,"%c", c[i] >= 32 && c[i] < 127 ? c[i] : '.');
			else
				zlog_info(zlog_handler," ");
		}

		zlog_info(zlog_handler,"\n");

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

int get_ip(char* ip, char* arg){
 
	int sockfd;
	struct ifreq ifr;
	struct sockaddr_in sin;
	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		printf("get_ip->socket error");
		return -1;
	}
	strcpy(ifr.ifr_name, arg);
	if(ioctl(sockfd, SIOCGIFADDR, &ifr) < 0)//直接获取IP地址
	{
		printf("get_ip->ioctl error");
		close(sockfd);
		return -2;
	}
	memcpy(&sin, &ifr.ifr_dstaddr, sizeof(sin));

	memcpy(ip, inet_ntoa(sin.sin_addr) , strlen(inet_ntoa(sin.sin_addr)) + 1);

	close(sockfd);

	return 0;
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

char numberToChar(int num){
	if(num>= 0 && num <=9){
		return '0' + num;
	}else{
		return 'a' + num - 10;
	}
}

//"000c29d46f68" low_32_str(8) high_16_str mac[6] -- 48bit
char* getLow32Str(char* mac){
	char* low32str = malloc(32);
	low32str[0] = '0'; low32str[1] = 'x';
	// mac[2] , mac[3] , mac[4], mac[5]
	//printf("mac[2] = %d , mac[3] = %d , mac[4] = %d, mac[5] = %d \n",mac[2],mac[3],mac[4],mac[5]);
	int temp[4];
	temp[0] = (mac[2]+256)%256;
	temp[1] = (mac[3]+256)%256;
	temp[2] = (mac[4]+256)%256;
	temp[3] = (mac[5]+256)%256;
	//printf("temp[0] = %d , temp[1] = %d, temp[2] = %d , temp[3] = %d \n",temp[0],temp[1],temp[2],temp[3]);
	int number[8];
	int index = 2;
	number[0] = temp[0] / 16;
	number[1] = temp[0] % 16;
	number[2] = temp[1] / 16;
	number[3] = temp[1] % 16;
	number[4] = temp[2] / 16;
	number[5] = temp[2] % 16;
	number[6] = temp[3] / 16;
	number[7] = temp[3] % 16;
	int i;
	int flag = 0;
	for(i=0;i<8;i++){
		if(flag == 0){
			if(number[i] == 0)
				continue;
			low32str[index] = numberToChar(number[i]);
			index = index + 1;
			flag = 1;
		}else{
			low32str[index] = numberToChar(number[i]);
			index = index + 1;
		}
		//printf("number[%d] = %d\n",i,number[i]);
	}
	if(flag == 0){
		low32str[index] = '0';
		index = index + 1;
	}
	low32str[index] = '\0';
	return low32str; 
}

char* getHigh16Str(char* mac){
	char* high16str = malloc(32);
	high16str[0] = '0'; high16str[1] = 'x';
	// mac[0] , mac[1]
	//printf("mac[0] = %d , mac[1] = %d \n",mac[0],mac[1]);
	int temp[2];
	temp[0] = (mac[0]+256)%256;
	temp[1] = (mac[1]+256)%256;
	//printf("temp[0] = %d , temp[1] = %d \n",temp[0],temp[1]);
	int number[4];
	int index = 2;
	number[0] = temp[0] / 16;
	number[1] = temp[0] % 16;
	number[2] = temp[1] / 16;
	number[3] = temp[1] % 16;
	int i;
	int flag = 0;
	for(i=0;i<4;i++){
		if(flag == 0){
			if(number[i] == 0)
				continue;
			high16str[index] = numberToChar(number[i]);
			index = index + 1;
			flag = 1;
		}else{
			high16str[index] = numberToChar(number[i]);
			index = index + 1;
		}
		//printf("number[%d] = %d\n",i,number[i]);
	}
	if(flag == 0){
		high16str[index] = '0';
		index = index + 1;
	}
	high16str[index] = '\0';
	return high16str;
}

uint32_t getLow32(char* dst){//"000A | 35000123" -- 0 1 2 3 4 5 
	uint32_t low32 = 0;
	char temp[4];
	memset(temp,0,4);
	memcpy(temp,&(dst[2]), 4);
	reverseBuf(temp,(char*)(&low32),4);
	return low32;
}

uint32_t getHigh16(char* dst){
	uint32_t high16 = 0;
	char temp[4];
	memset(temp,0,4);
	memcpy(temp + 2,&(dst[0]),2);
	reverseBuf(temp,((char*)(&high16)),4);
	return high16;
}

void reverseBuf(char* in_buf, char* out_buf, int number){
	int i,j;
	for(i=0,j=number-1;i<number;i++){
		out_buf[i] = in_buf[j];
		j=j-1;
	}
}



/* ---------------------- timer ------------------------------------- */

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


/* ---------------------------- crc caculate ------------------------------------------ */

/* calculate crc via one byte */
unsigned char cal_table_high_first(unsigned char value)
{
    unsigned char i, crc;
 
    crc = value;
    /* 数据往左移了8位，需要计算8次 */
    for (i=8; i>0; --i)
    { 
        if (crc & 0x80)  /* 判断最高位是否为1 */
        {
        /* 最高位为1，不需要异或，往左移一位，然后与0x31异或 */
        /* 0x31(多项式：x8+x5+x4+1，100110001)，最高位不需要异或，直接去掉 */
            crc = (crc << 1) ^ 0x31;        }
        else
        {
            /* 最高位为0时，不需要异或，整体数据往左移一位 */
            crc = (crc << 1);
        }
    }
 
    return crc;
}

/* create lookup table */
void  create_crc_table(void)
{
    unsigned short i;
    unsigned char j;
 
    for (i=0; i<=0xFF; i++)
    {
        if (0 == (i%16))
            printf("\n");
 
        j = i&0xFF;
            printf("0x%.2x, ", cal_table_high_first (j));  /*依次计算每个字节的crc校验值*/
    }
}


/* crc 8 */
unsigned char crc_high_first(unsigned char *ptr, unsigned char len)
{
    unsigned char i;
    unsigned char crc=0x00;
 
    while(len--)
    {
		/* 每次先与需要计算的数据异或,计算完指向下一数据 */ 
        crc ^= *ptr++;
        for (i=8; i>0; --i) 
        { 
            if (crc & 0x80)
                crc = (crc << 1) ^ 0x31;
            else
                crc = (crc << 1);
        }
    }
 
    return (crc);
}


unsigned char cal_crc_table(unsigned char *ptr, unsigned char len) 
{
    unsigned char  crc = 0x00;
 
    while (len--)
    {
        crc = crc_table[crc ^ *ptr++];
    }
    return (crc);
}

void print_buf(char* buf, int length){
	printf("buf: \n");
	for(int i = 0 ;i < length ;i++){
		printf("0x%.2x,",buf[i]);
	}
	printf("\n");
}

unsigned char crc_high_first_by_init_crc(unsigned char *ptr, unsigned char len, unsigned char init_crc)
{
    unsigned char i;
    //unsigned char crc=0x00;
	unsigned char crc = init_crc;
 
    while(len--)
    {
		/* 每次先与需要计算的数据异或,计算完指向下一数据 */ 
        crc ^= *ptr++;
        for (i=8; i>0; --i) 
        { 
            if (crc & 0x80)
                crc = (crc << 1) ^ 0x31;
            else
                crc = (crc << 1);
        }
    }
 
    return (crc);
}


unsigned char cal_crc_table_by_init_crc(unsigned char *ptr, unsigned char len, unsigned char init_crc) 
{
    //unsigned char  crc = 0x00;
	unsigned char crc = init_crc;

    while (len--)
    {
        crc = crc_table[crc ^ *ptr++];
    }
    return (crc);
}


/* -------------------------------- aligned memory allocate ------------------------------------------- */
void* aligned32_malloc(size_t size){
	void *original = malloc(size + 32);
	unsigned int aligned_address = ( (unsigned int)original & ~((unsigned int)31) ) + 32;
	void *aligned = (void*)aligned_address;
	*((void**)aligned - 1) = original;
	return aligned;
}

void aligned32_free(void* ptr){
	free(*((void**)ptr - 1));
}









