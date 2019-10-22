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
	char* send_buf;
	char* recv_buf;
}g_args_frame;

struct g_args_frame* g_paramter = NULL; // thread safe ?


int ManagementFrame_create_monitor_interface(){
	int fd = -1;
	fd = open("/dev/sig-chan", O_RDWR|O_NONBLOCK);
	g_paramter = (g_args_frame*)malloc(sizeof(g_args_frame));

	g_paramter->send_buf = malloc(1500);
	g_paramter->recv_buf = malloc(1500); // buf
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
	char seq_id_buf[2];
	char reverse_seq_id[2];
	memcpy(seq_id_buf,(char*)(&frame_Info->seq_id),2);
	reverseBuf(seq_id_buf,reverse_seq_id,2); /// note that !!!!!!!!!!!!!
	memcpy(buf + 24, reverse_seq_id, 2);

	// caculate crc and set into last byte of reverse_buf
	unsigned char crc = cal_crc_table_by_init_crc(reverse_buf, 3, 0x00);
	crc = cal_crc_table_by_init_crc(buf + 6, 20, crc);
	memcpy(buf+2+3,&crc,1);
}

int parse_buffer(management_frame_Info* frame_Info , char* buf){

	// check crc
	unsigned char crc = cal_crc_table_by_init_crc(buf+2, 3, 0x00);
	crc = cal_crc_table_by_init_crc(buf + 6, 20, crc);
	unsigned char get_crc = 0x00;
	memcpy(&get_crc, buf+2+3,1);
	//printf("get_crc = 0x%.2x, crc = 0x%.2x \n", get_crc, crc);
	if(get_crc != crc){
		return -1;
	}

	
	// parse Frame_control
	char tmp_buf[4];
	char control_buf[4];
	memcpy(tmp_buf,buf + 2,4);
	reverseBuf(tmp_buf,control_buf,4); // note that !!!!!!
	unsigned int frame_control = *((unsigned int*)(control_buf));
	//printf("parse_buffer : frame_control = 0x%x.\n", frame_control);

	frame_Info->subtype = bits_get_subtype(frame_control);
	frame_Info->length  = bits_get_length(frame_control);
	
	memcpy(frame_Info->source_mac_addr,buf + 6,6);
	memcpy(frame_Info->dest_mac_addr,buf + 12,6);
	memcpy(frame_Info->Next_dest_mac_addr,buf + 18,6);

	char reveses_seq_buf[2];
	char seq_buf[2];
	memcpy(reveses_seq_buf,buf + 24,2);
	reverseBuf(reveses_seq_buf,seq_buf,2);
	
	memcpy((char*)(&frame_Info->seq_id),seq_buf,2);
	return 0;
}

int handle_air_tx(management_frame_Info* frame_Info, zlog_category_t *zlog_handler){
	if(frame_Info == NULL)
		return -1;
	int rc = 0;
	zlog_info(zlog_handler,"air_tx : send_id = %d ", frame_Info->seq_id);
	fill_buffer(frame_Info, g_paramter->send_buf); // g_parameter->buf ??? 
	int fill_len = frame_Info->length;
	int64_t start = now();
	rc = write(g_paramter->fd,g_paramter->send_buf,fill_len);
	int64_t end = now();
	double msec = (end-start)/1000.0;
	zlog_info(zlog_handler,"msec = %f ", msec);
	if(msec > 1.0)
		zlog_info(zlog_handler," ***** handle_air_tx : write is too long ---- msec = %f",msec);
	if(rc < 0)
		return rc;

	return rc; // tx _ success
}

// thread safe ?
int gw_monitor_poll(management_frame_Info* frame_Info, int time_cnt, zlog_category_t *zlog_handler){
	int loop = 0;
	int rc = 0;
	int ret = -1;
	for(loop = 0; loop <time_cnt; loop++){
		rc = poll(&(g_paramter->poll_fd),1,5); // ms
		if(rc > 0){
			if(POLLIN == g_paramter->poll_fd.revents){
				rc = read(g_paramter->fd, g_paramter->recv_buf, 1500);
				if(rc){
					// get management frame
					int state = parse_buffer(frame_Info,g_paramter->recv_buf); // g_parameter->buf ????
					if(state == -1){
						zlog_info(zlog_handler,"received air signal crc error ! ! ! ");
						return -11;
					}
					//hexdump_zlog(g_paramter->buf,28,zlog_handler);
					break;
				}else{
					ret = -2;
				}
			}else{
				ret = -3;
			}
		}else{
				ret = -4;
		}
	}
	if(loop == time_cnt)
		return ret;
	else
		return rc;
}

void close_monitor_interface(){
	close(g_paramter->fd);
	free(g_paramter->send_buf);
	free(g_paramter->recv_buf);
	free(g_paramter);
}

management_frame_Info* new_air_frame(int32_t subtype, int32_t payload_len, char* mac_buf, char* mac_buf_dest, char* mac_buf_next,
									uint16_t seq_id){
	management_frame_Info* temp_Info = (management_frame_Info*)malloc(sizeof(management_frame_Info));
	temp_Info->type = 0;
	temp_Info->subtype = subtype; ///---------subtype
	temp_Info->length  = 26 + payload_len;
	temp_Info->seq_id = seq_id;
	memset(temp_Info->source_mac_addr,0,6);
	memset(temp_Info->dest_mac_addr,0,6);
	memset(temp_Info->Next_dest_mac_addr,0,6);
	
	if(mac_buf != NULL)
		memcpy(temp_Info->source_mac_addr,mac_buf,6);
	if(mac_buf_dest != NULL)
		memcpy(temp_Info->dest_mac_addr,mac_buf_dest,6);
	if(mac_buf_next != NULL)
		memcpy(temp_Info->Next_dest_mac_addr,mac_buf_next,6);

	return temp_Info;
}



// 0 == memcmp(buf, buf2, rc)





















