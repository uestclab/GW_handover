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

#define	REG_PHY_ADDR	0x43C20000
#define	REG_MAP_SIZE	0X10000

#define GET_SIZE        100

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


double calculateFreq(unsigned int number){
	int freq_offset_i = 0, freq_offset_q =0, freq_offset_i_pre=0, freq_offset_q_pre=0;

	freq_offset_q_pre = number - ((number>>8) << 8);
	number = number >> 8;
	freq_offset_i_pre = number - ((number>>8) << 8);
	number = number >> 8;
	freq_offset_q = number - ((number>>8) << 8);
	number = number >> 8;
	freq_offset_i = number - ((number>>8) << 8);

	if(freq_offset_i > 127)
		freq_offset_i = freq_offset_i - 256;

	if(freq_offset_q > 127)
		freq_offset_q = freq_offset_q - 256;

	if(freq_offset_i_pre > 127)
		freq_offset_i_pre = freq_offset_i_pre - 256;

	if(freq_offset_q_pre > 127)
		freq_offset_q_pre = freq_offset_q_pre - 256;

	double temp_1 = atan((freq_offset_q * 1.0)/(freq_offset_i * 1.0));
	double temp_2 = atan((freq_offset_q_pre * 1.0)/(freq_offset_i_pre * 1.0));
	double diff = temp_1 - temp_2;
	double result = (diff / 5529.203) * 1000000000;
	return result;
}


// broker callback interface
int process_exception(char* buf, int buf_len, char *from, void* arg)
{
	int ret = 0;
	if(strcmp(from,"mon/all/pub/system_stat") == 0){
		printf("system_stat is %s \n" , buf);
	}
	return ret;
}

uint32_t freq_offset(g_RegDev_para* g_RegDev){ // value != 0 : full
	uint32_t flag = 0x00000000;
	int	rc = regdev_read(g_RegDev->mem_dev_phy, 0x138, &flag);
	if(rc < 0){
		zlog_info(g_RegDev->log_handler,"freq_offset failed !!! \n");
		return rc;
	}

	return flag;
}

int main(int argc, char *argv[]){

	zlog_category_t *handler = serverLog("../conf/zlog_default.conf");


	printf(" +++++++++++++++++++++++++++++ start ut_main ++++++++++++++++++++++++++++++++++++++++++++++ \n");

	g_RegDev_para* g_RegDev = NULL;
	int stat = initRegdev(&g_RegDev, handler);
	if(stat != 0 ){
		printf("initRegdev create failed !");
		return 0;
	}

	uint32_t* result = (uint32_t*)malloc(sizeof(uint32_t)*GET_SIZE);
	
	uint32_t temp;

	for(int i = 0 ;i<GET_SIZE;i++){
		temp = rx_byte_filter_ether_low32(g_RegDev);
		result[i] = temp;
		usleep(1000);
	}
	
	for(int i = 0;i<GET_SIZE;i++){
		printf("result[%d] = %x , ", i,result[i]);
	}

	printf("------------------- end ----------------------------------\n");
	
}














