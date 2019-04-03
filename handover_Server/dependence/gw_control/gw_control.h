#ifndef GW_CONTROL_H
#define GW_CONTROL_H

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*recv_cb)(char* buf, int buf_len, char *from, void* arg);

int initBroker(char *argv, recv_cb exception_cb);

int disable_dac();
int enable_dac();
int set_dst_mac(char* low_32_str, char* high_16_str);
int set_src_mac(char* low_32_str, char* high_16_str);

#ifdef __cplusplus
}
#endif

#endif//GW_CONTROL_H
