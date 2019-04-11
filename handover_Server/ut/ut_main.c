#include <stdio.h>
#include <stdlib.h>
#include "gw_utility.h"
#include "gw_control.h"
#include "zlog.h"
#include "regdev_common.h"

#define	REG_PHY_ADDR	0x43C20000
#define	REG_MAP_SIZE	0X10000

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

	zlog_category_t *zlog_handler = serverLog("../conf/zlog_default.conf");


	zlog_info(zlog_handler," +++++++++++++++++++++++++++++ start reg time compare ++++++++++++++++++++++++++++++++++++++++++++++ \n");

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

	g_RegDev_para* g_RegDev = NULL;
	int stat = initRegdev(&g_RegDev, zlog_handler);
	if(stat != 0 ){
		zlog_info(zlog_handler,"initRegdev create failed !");
		return 0;
	}
	
	char source[6];
	change_mac_buf(szMac,source);
	zlog_info(zlog_handler," start set_dst_mac_fast \n");
	stat = set_dst_mac_fast(g_RegDev, source);
	//stat = set_src_mac_fast(g_RegDev, source);
	zlog_info(zlog_handler," end set_dst_mac_fast \n");

	initBroker(argv[0], process_exception);
	
	char* high16str = getHigh16Str(source);
	zlog_info(zlog_handler,"high16str = %s\n",high16str);
	char* low32str = getLow32Str(source);
	zlog_info(zlog_handler,"low32str = %s\n",low32str);
	int ret = set_src_mac(low32str, high16str);
	zlog_info(zlog_handler,"set_dst_mac : ret = %d \n", ret);
	free(high16str);
	free(low32str);
	
}














