#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "cJSON.h"
#include "gw_utility.h"
#include "gw_control.h"
#include "broker.h"
#include "regdev_common.h"

#ifdef PC_STUB
	
#endif


int lowlevel_transfer(char *buf, int buf_len){
	int ret = -1;
	char* stat_buf = NULL;
	int stat_buf_len = 0;
	cJSON * root = NULL;
    cJSON * item = NULL;
	cJSON * item_type = NULL;
    root = cJSON_Parse(buf);
    item = cJSON_GetObjectItem(root,"dst"); // different device is a dst
	printf("dst = %s , \n",item->valuestring);
#ifndef PC_STUB
	ret = dev_transfer(buf, buf_len, &stat_buf, &stat_buf_len, item->valuestring, -1);
#endif
	if(ret == 0 && stat_buf_len > 0){
		if(strcmp(item->valuestring,"gpio") == 0){
			printf("stat_buf = %s \n",stat_buf); 
		}
		free(stat_buf);
	}
	cJSON_Delete(root);
	return ret;
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

int initBroker(char *argv, recv_cb exception_cb){
#ifndef PC_STUB
	int ret = init_broker(get_prog_name(argv), NULL, -1, NULL, NULL);
	printf("get_prog_name(argv) = %s , ret = %d \n",get_prog_name(argv),ret);
	if( ret != 0)
		return -2;

	ret = register_callback("all", exception_cb, "#");
	if(ret != 0){
		printf("register_callback error in initBroker\n");
		return -1;
	}
#endif

#ifdef PC_STUB
	int ret = 0;
	printf("get_prog_name(argv) = %s , ret = %d \n",get_prog_name(argv),ret);
#endif
	return 0;
}

// src_mac 48bit
int set_src_mac(char* low_32_str, char* high_16_str){
	int ret = 0;
	cJSON *root = cJSON_CreateObject();
	cJSON_AddStringToObject(root, "base", "0x43c20000");
	cJSON_AddStringToObject(root, "map_size", "0x1000");
	cJSON_AddStringToObject(root, "dst", "reg");

	cJSON *array=cJSON_CreateArray();
	cJSON_AddItemToObject(root,"op_cmd",array);

	cJSON *arrayobj=cJSON_CreateObject();
	cJSON_AddItemToArray(array,arrayobj);
	cJSON_AddStringToObject(arrayobj, "name","src_mac_0_low_32_bit");
	cJSON_AddStringToObject(arrayobj, "id","0x0");
	cJSON_AddStringToObject(arrayobj, "offset","0x830");
	cJSON_AddStringToObject(arrayobj, "cmd","1");
	cJSON_AddStringToObject(arrayobj, "val",low_32_str);
	cJSON_AddStringToObject(arrayobj, "waite_time","0");

	cJSON *arrayobj_1=cJSON_CreateObject();
	cJSON_AddItemToArray(array,arrayobj_1);
	cJSON_AddStringToObject(arrayobj_1, "name","src_mac_1_high_16_bit");
	cJSON_AddStringToObject(arrayobj_1, "id","0x0");
	cJSON_AddStringToObject(arrayobj_1, "offset","0x834");
	cJSON_AddStringToObject(arrayobj_1, "cmd","1");
	cJSON_AddStringToObject(arrayobj_1, "val",high_16_str);
	cJSON_AddStringToObject(arrayobj_1, "waite_time","0");

	char* srcMac_jsonfile = cJSON_Print(root);
	printf("srcMac_jsonfile = %s\n",srcMac_jsonfile);
#ifndef PC_STUB
	ret = lowlevel_transfer(srcMac_jsonfile,strlen(srcMac_jsonfile)+1);
#endif	
	cJSON_Delete(root);
	free(srcMac_jsonfile);
	return ret;
}

// dst_mac 48bit 
int set_dst_mac(char* low_32_str, char* high_16_str){
	int ret = 0;
	cJSON *root = cJSON_CreateObject();
	cJSON_AddStringToObject(root, "base", "0x43c20000");
	cJSON_AddStringToObject(root, "map_size", "0x1000");
	cJSON_AddStringToObject(root, "dst", "reg");

	cJSON *array=cJSON_CreateArray();
	cJSON_AddItemToObject(root,"op_cmd",array);

	cJSON *arrayobj=cJSON_CreateObject();
	cJSON_AddItemToArray(array,arrayobj);
	cJSON_AddStringToObject(arrayobj, "name","dest_mac_0_low_32_bit");
	cJSON_AddStringToObject(arrayobj, "id","0x0");
	cJSON_AddStringToObject(arrayobj, "offset","0x838");
	cJSON_AddStringToObject(arrayobj, "cmd","1");
	cJSON_AddStringToObject(arrayobj, "val",low_32_str);
	cJSON_AddStringToObject(arrayobj, "waite_time","0");

	cJSON *arrayobj_1=cJSON_CreateObject();
	cJSON_AddItemToArray(array,arrayobj_1);
	cJSON_AddStringToObject(arrayobj_1, "name","dest_mac_1_high_16_bit");
	cJSON_AddStringToObject(arrayobj_1, "id","0x0");
	cJSON_AddStringToObject(arrayobj_1, "offset","0x83C");
	cJSON_AddStringToObject(arrayobj_1, "cmd","1");
	cJSON_AddStringToObject(arrayobj_1, "val",high_16_str);
	cJSON_AddStringToObject(arrayobj_1, "waite_time","0");

	char* dstMac_jsonfile = cJSON_Print(root);
	printf("dstMac_jsonfile = %s\n",dstMac_jsonfile);
#ifndef PC_STUB
	ret = lowlevel_transfer(dstMac_jsonfile,strlen(dstMac_jsonfile)+1);
#endif	
	cJSON_Delete(root);
	free(dstMac_jsonfile);
	return ret;
}

 /* ----------------------- new interface to read and write register ---------------------------- */

int initRegdev(g_RegDev_para** g_RegDev, zlog_category_t* handler)
{
	zlog_info(handler,"initPhyRegdev()");

	*g_RegDev = (g_RegDev_para*)malloc(sizeof(struct g_RegDev_para));

	(*g_RegDev)->mem_dev_phy = NULL;
	(*g_RegDev)->mem_dev_dac = NULL;
	(*g_RegDev)->log_handler = handler;


	int rc = 0;
	
	regdev_init(&((*g_RegDev)->mem_dev_phy));
	regdev_set_para((*g_RegDev)->mem_dev_phy, REG_PHY_ADDR, REG_MAP_SIZE);
	rc = regdev_open((*g_RegDev)->mem_dev_phy);
	if(rc < 0){
		zlog_info(handler," phy regdev_open err !!\n");
		return -1;
	}
	/*
	regdev_init(&((*g_RegDev)->mem_dev_dac));
	regdev_set_para((*g_RegDev)->mem_dev_dac, REG_PHY_ADDR, REG_MAP_SIZE);
	rc = regdev_open((*g_RegDev)->mem_dev_dac);
	if(rc < 0){
		zlog_info(handler," dac regdev_open err !!\n");
		return -1
	}
*/

	return 0;
}


int disable_dac(g_RegDev_para* g_RegDev){
	zlog_info(g_RegDev->log_handler,"disable_dac\n");
	int rc = 0;
	//gpio
	return rc;
}

int enable_dac(g_RegDev_para* g_RegDev){
	zlog_info(g_RegDev->log_handler,"enable_dac\n");
	int rc = 0;
	// gpio 
	return rc;
}

int set_dst_mac_fast(g_RegDev_para* g_RegDev, char* dst_mac_buf){ // mac_buf[6]
	zlog_info(g_RegDev->log_handler,"set_dst_mac_fast\n");

	uint32_t low32 = getLow32(dst_mac_buf);
	uint32_t high16 = getHigh16(dst_mac_buf);	
	
	// dest_mac_0_low_32_bit
	int	rc = regdev_write(g_RegDev->mem_dev_phy, 0x838, low32);
	if(rc < 0){
		zlog_info(g_RegDev->log_handler,"dest_mac_0_low_32_bit write failed !!! \n");
		return rc;
	}
	//dest_mac_1_high_16_bit
	rc = regdev_write(g_RegDev->mem_dev_phy, 0x83c, high16);
	if(rc < 0){
		zlog_info(g_RegDev->log_handler,"dest_mac_1_high_16_bit write failed !!! \n");
		return rc;
	}
	return 0;
}

int set_src_mac_fast(g_RegDev_para* g_RegDev, char* src_mac_buf){ //src_mac_buf[6]
	zlog_info(g_RegDev->log_handler,"set_src_mac_fast\n");

	uint32_t low32 = getLow32(src_mac_buf);
	uint32_t high16 = getHigh16(src_mac_buf);

	// src_mac_0_low_32_bit
	int	rc = regdev_write(g_RegDev->mem_dev_phy, 0x830, low32);
	if(rc < 0){
		zlog_info(g_RegDev->log_handler,"src_mac_0_low_32_bit write failed !!! \n");
		return rc;
	}
	//src_mac_1_high_16_bit
	rc = regdev_write(g_RegDev->mem_dev_phy, 0x834, high16);
	if(rc < 0){
		zlog_info(g_RegDev->log_handler,"src_mac_1_high_16_bit write failed !!! \n");
		return rc;
	}
	return 0;
}












