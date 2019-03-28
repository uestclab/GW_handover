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
#include <sys/wait.h>

#include "gw_frame.h"
#include "gw_utility.h"


#ifndef	ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif

typedef struct g_args_frame{
	struct pollfd poll_fd;
	int fd;
	char* buf;
}g_args_frame;

struct g_args_frame* g_paramter = NULL; // thread safe ?


int ManagementFrame_create_monitor_interface(){
	int fd = -1;
	fd = open("/dev/sig-chan", O_RDWR|O_NONBLOCK);
	g_paramter = (g_args_frame*)malloc(sizeof(g_args_frame));

	g_paramter->buf = malloc(1500);
	g_paramter->poll_fd.fd = fd;
	g_paramter->poll_fd.events=POLLIN;
	g_paramter->fd = fd;
	
	return fd;
}

// bit26 ~ bit29
void bits_set_subtype(unsigned int* frame_control, unsigned int subtype){
	(*frame_control) &= (~(0xf<<26));
	(*frame_control) |= (subtype<<26);
	//printf("frame_control = 0x%x.\n", *frame_control);
}

//bit8 ~ bit25
void bits_set_length(unsigned int* frame_control, unsigned int length){
	(*frame_control) &= (~(0x3ffff<<8));
	(*frame_control) |= (length<<8);
	//printf("frame_control = 0x%x.\n", *frame_control);
}

unsigned int bits_get_subtype(unsigned int frame_control){
	unsigned int subtype = frame_control & (0xf<<26);
	subtype = subtype>>26;
	return subtype;
}

unsigned int bits_get_length(unsigned int frame_control){
	unsigned int length = frame_control & (0x3ffff<<8);
	length = length>>8;
	return length;
}
  

/*
	reserve 2 byte in front of frame_control -- 20190319
*/

void fill_buffer(management_frame_Info* frame_Info, char* buf){
	// 4 byte Frame_control
	unsigned int frame_control = 0;
	bits_set_subtype(&frame_control,frame_Info->subtype);
	bits_set_length(&frame_control,frame_Info->length);
	char reverse_buf[4];
	char tmp_buf[4];
	memcpy(tmp_buf,&frame_control,4);
	reverseBuf(tmp_buf,reverse_buf,4); /// note that !!!!!!!!!!!!!

	memset(buf,0,2);
	memcpy(buf + 2, reverse_buf, 4);
	// 6 byte source_mac_addr
	memcpy(buf + 6,frame_Info->source_mac_addr,6);

	// 6 byte Dest_mac_addr
	memcpy(buf + 12,frame_Info->dest_mac_addr,6);
	
	// 6 byte next_dest_mac_addr
	memcpy(buf + 18,frame_Info->Next_dest_mac_addr,6);
	
	// 2 byte seq_id
	memset(buf + 24, 0, 2);

	// payload
	if(frame_Info->length > 26){
		memcpy(buf + 26,frame_Info->payload,frame_Info->length - 26);
	}
}

void parse_buffer(management_frame_Info* frame_Info , char* buf){
	int32_t pre_length = frame_Info->length;
	
	// parse Frame_control
	char tmp_buf[4];
	char control_buf[4];
	memcpy(tmp_buf,buf + 2,4);
	reverseBuf(tmp_buf,control_buf,4); // note that !!!!!!
	unsigned int frame_control = *((unsigned int*)(control_buf));
	printf("parse_buffer : frame_control = 0x%x.\n", frame_control);

	frame_Info->subtype = bits_get_subtype(frame_control);
	frame_Info->length  = bits_get_length(frame_control);
	
	memcpy(frame_Info->source_mac_addr,buf + 6,6);
	memcpy(frame_Info->dest_mac_addr,buf + 12,6);
	memcpy(frame_Info->Next_dest_mac_addr,buf + 18,6);
	
	printf("subtype = %d ,length = %d \n",frame_Info->subtype,frame_Info->length);

	if(frame_Info->length > 26){
		if(pre_length < frame_Info->length){
			printf("resize and remalloc new payload buffer \n");
			free(frame_Info->payload);
			frame_Info->payload = malloc(frame_Info->length - 26);
		}
		memcpy(frame_Info->payload,buf + 26,frame_Info->length - 26);
	}
}

int handle_air_tx(management_frame_Info* frame_Info, zlog_category_t *zlog_handler){
	if(frame_Info == NULL)
		return -1;
	int rc = 0;

	fill_buffer(frame_Info, g_paramter->buf);
	int fill_len = frame_Info->length;
	rc = write(g_paramter->fd,g_paramter->buf,fill_len);
	// debug	
	zlog_info(zlog_handler,"handle_monitor_tx_with_response->write : rc = %d , g_paramter->fd = %d , fill_len = %d \n" ,
				rc , g_paramter->fd , fill_len);
	hexdump(g_paramter->buf,frame_Info->length,zlog_handler);
	// ------ 
	if(rc < 0)
		return rc;

	return 0; // tx _ success
}

// thread safe ?
int gw_monitor_poll(management_frame_Info* frame_Info, int time_cnt){
	int loop = 0;
	int rc = 0;
	int ret = 3;
	for(loop = 0; loop <time_cnt; loop++){
		rc = poll(&(g_paramter->poll_fd),1,5); // ms
		if(rc > 0){
			if(POLLIN == g_paramter->poll_fd.revents){
				rc = read(g_paramter->fd, g_paramter->buf, ARRAY_SIZE(g_paramter->buf));
				if(rc){
					// get management frame
					parse_buffer(frame_Info,g_paramter->buf);
					break;
				}else{
					ret = 4;
				}
			}else{
				ret = 5;
			}
		}else{
				ret = 6;
		}
	}
	if(loop == time_cnt)
		return ret;
	else
		return 0;
}

void close_monitor_interface(){
	close(g_paramter->fd);
	free(g_paramter->buf);
	free(g_paramter);
}

management_frame_Info* new_air_frame(int32_t subtype, int32_t payload_len, char* mac_buf, char* mac_buf_dest, char* mac_buf_next){
	management_frame_Info* temp_Info = (management_frame_Info*)malloc(sizeof(management_frame_Info));
	temp_Info->type = 0;
	temp_Info->subtype = subtype; ///---------subtype
	temp_Info->length  = 26 + payload_len;
	memset(temp_Info->source_mac_addr,0,6);
	memset(temp_Info->dest_mac_addr,0,6);
	memset(temp_Info->Next_dest_mac_addr,0,6);
	
	if(mac_buf != NULL)
		memcpy(temp_Info->source_mac_addr,mac_buf,6);
	if(mac_buf_dest != NULL)
		memcpy(temp_Info->dest_mac_addr,mac_buf_dest,6);
	if(mac_buf_next != NULL)
		memcpy(temp_Info->Next_dest_mac_addr,mac_buf_next,6);


	temp_Info->payload = NULL;
	if(temp_Info->length > 26){
		temp_Info->payload = malloc(payload_len);
	}
	return temp_Info;
}


void reverseBuf(char* in_buf, char* out_buf, int number){
	int i,j;
	for(i=0,j=number-1;i<number;i++){
		out_buf[i] = in_buf[j];
		j=j-1;
	}
}


// 0 == memcmp(buf, buf2, rc)





















