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
#include "gw_frame.h"
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


// bit26 ~ bit29
void set_subtype(unsigned int* frame_control, unsigned int subtype){
	(*frame_control) &= (~(0xf<<26));
	(*frame_control) |= (subtype<<26);
	//printf("frame_control = 0x%x.\n", *frame_control);
}

//bit8 ~ bit25
void set_length(unsigned int* frame_control, unsigned int length){
	(*frame_control) &= (~(0x3ffff<<8));
	(*frame_control) |= (length<<8);
	//printf("frame_control = 0x%x.\n", *frame_control);
}


void fill_buffer_tmp(management_frame_Info* frame_Info, char* buf){
	// 4 byte Frame_control
	unsigned int frame_control = 0;
	set_subtype(&frame_control,frame_Info->subtype);
	set_length(&frame_control,frame_Info->length);
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

unsigned int get_subtype(unsigned int frame_control){
	unsigned int subtype = frame_control & (0xf<<26);
	subtype = subtype>>26;
	return subtype;
}

unsigned int get_length(unsigned int frame_control){
	unsigned int length = frame_control & (0x3ffff<<8);
	length = length>>8;
	return length;
}

int parse_buffer_tmp(management_frame_Info* frame_Info , char* buf){

	// check crc
	unsigned char crc = cal_crc_table_by_init_crc(buf+2, 3, 0x00);
	crc = cal_crc_table_by_init_crc(buf + 6, 20, crc);
	unsigned char get_crc = 0x00;
	memcpy(&get_crc, buf+2+3,1);
	printf("get_crc = 0x%.2x, crc = 0x%.2x \n", get_crc, crc);
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

	frame_Info->subtype = get_subtype(frame_control);
	frame_Info->length  = get_length(frame_control);
	
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

//00 66 01 01 02 03
//00:11:01:01:02:03
int main(int argc, char *argv[]){
	char* send_buf = malloc(1500);
	uint16_t send_id = 10;
	char s_mac[6];
	memset(s_mac,0,6);
	s_mac[0] = 0x00; s_mac[1] = 0x66; s_mac[2] = 0x01;
	s_mac[3] = 0x01; s_mac[4] = 0x02; s_mac[5] = 0x03;
	char d_mac[6];
	memset(d_mac,0,6);
	d_mac[0] = 0x00; d_mac[1] = 0x11; d_mac[2] = 0x01;
	d_mac[3] = 0x01; d_mac[4] = 0x02; d_mac[5] = 0x03;

	management_frame_Info* frame_Info = new_air_frame(2,0,s_mac,d_mac,d_mac,send_id);
	fill_buffer_tmp(frame_Info, send_buf);
	printf("-------------- transfer ---------------------- ");
	send_buf[6] = 0x22; 
	management_frame_Info* temp_Info = new_air_frame(-1,0,NULL,NULL,NULL,0);
	int state = parse_buffer_tmp(temp_Info, send_buf);
	printf("state = %d \n", state);
	printf("temp_Info: \n");
	printf("subtype = %d, length = %d, seq_id = %d \n", temp_Info->subtype, temp_Info->length, temp_Info->seq_id);
	printf("source _mac : \n");
	print_buf(temp_Info->source_mac_addr,6);
	printf("dest _mac : \n");
	print_buf(temp_Info->dest_mac_addr,6);
	printf("next_dest _mac : \n");
	print_buf(temp_Info->Next_dest_mac_addr,6);

}











