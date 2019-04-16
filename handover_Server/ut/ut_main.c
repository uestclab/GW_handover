#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include "gw_utility.h"
#include "gw_control.h"
#include "zlog.h"
#include "regdev_common.h"

#define	REG_PHY_ADDR	0x43C20000
#define	REG_MAP_SIZE	0X10000


void setTimer(int seconds, int useconds)
{
    struct timeval temp;

    temp.tv_sec = seconds;
    temp.tv_usec = useconds;

    select(0, NULL, NULL, NULL, &temp);
    return ;
}


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


// broker callback interface
int process_exception(char* buf, int buf_len, char *from, void* arg)
{
	int ret = 0;
	if(strcmp(from,"mon/all/pub/system_stat") == 0){
		printf("system_stat is %s \n" , buf);
	}
	return ret;
}

int main(int argc, char *argv[]){

	zlog_category_t *handler = serverLog("../conf/zlog_default.conf");


	printf(" +++++++++++++++++++++++++++++ start reg time compare ++++++++++++++++++++++++++++++++++++++++++++++ \n");
/*
	printf("argc = %d \n",argc);
	if(argc!=2)
	{
		printf("Usage : ethname\n");
		return 1;
	}
    char    szMac[18];
    int     nRtn = get_mac(szMac, sizeof(szMac),argv[1]);
    if(nRtn > 0) // nRtn = 12
    {	
        printf("argv[1] = %s , nRtn = %d , MAC ADDR : %s\n", argv[1],nRtn,szMac);
    }else{
		return 0;
	}
*/
	g_RegDev_para* g_RegDev = NULL;
	int stat = initRegdev(&g_RegDev, handler);
	if(stat != 0 ){
		printf("initRegdev create failed !");
		return 0;
	}
	
	int rc = enable_dac(g_RegDev);
	rc = open_ddr(g_RegDev);
	printf("enable_dac and open_ddr \n");

	uint32_t value = getPowerLatch(g_RegDev);
	printf("getPowerLatch() = %x \n", value);

	value = get_crc_correct_cnt(g_RegDev);
	printf("get_crc_correct_cnt() = %x \n", value);

	value = get_crc_error_cnt(g_RegDev);
	printf("get_crc_error_cnt() = %x \n", value);

	value = reset_ddr_full_flag(g_RegDev);
	printf("reset_ddr_full_flag() = %x \n", value);

	value = ddr_full_flag(g_RegDev);
	printf("ddr_full_flag() = %x \n", value);
	value = airdata_buf2_empty_flag(g_RegDev);
	printf("airdata_buf2_empty_flag() = %x \n", value);
	value = airsignal_buf2_empty_flag(g_RegDev);
	printf("airsignal_buf2_empty_flag() = %x \n", value);

	value = read_unfilter_byte_low32(g_RegDev);
	printf("read_unfilter_byte_low32() = %x \n", value);
	value = read_unfilter_byte_high32(g_RegDev);
	printf("read_unfilter_byte_high32() = %x \n", value);
	value = rx_byte_filter_ether_low32(g_RegDev);
	printf("rx_byte_filter_ether_low32() = %x \n", value);
	value = rx_byte_filter_ether_high32(g_RegDev);
	printf("rx_byte_filter_ether_high32() = %x \n", value);
	
}














