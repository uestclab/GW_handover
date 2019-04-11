#ifndef GW_CONTROL_H
#define GW_CONTROL_H

#include "zlog.h"

#define	REG_PHY_ADDR	0x43C20000
#define	REG_MAP_SIZE	0X10000

typedef int (*recv_cb)(char* buf, int buf_len, char *from, void* arg);

int initBroker(char *argv, recv_cb exception_cb);

int set_dst_mac(char* low_32_str, char* high_16_str);
int set_src_mac(char* low_32_str, char* high_16_str);

typedef struct g_RegDev_para{
	void*  			   mem_dev_phy;
	void*  			   mem_dev_dac;
	zlog_category_t*   log_handler;
}g_RegDev_para;


int initRegdev(g_RegDev_para** g_RegDev, zlog_category_t* handler);
int disable_dac(g_RegDev_para* g_RegDev);
int enable_dac(g_RegDev_para* g_RegDev);
int set_dst_mac_fast(g_RegDev_para* g_RegDev, char* dst_mac_buf);
int set_src_mac_fast(g_RegDev_para* g_RegDev, char* src_mac_buf);


#endif//GW_CONTROL_H
