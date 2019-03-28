#ifndef GW_CONTROL_H
#define GW_CONTROL_H

#ifdef __cplusplus
extern "C" {
#endif

void disable_dac();
void enable_dac();

int initBroker(char *argv);

#ifdef __cplusplus
}
#endif

#endif//GW_CONTROL_H
