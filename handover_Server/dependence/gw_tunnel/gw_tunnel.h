#ifndef GLORY_TUNNEL_H
#define GLORY_TUNNEL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "If_tunnel.h"

int change_tunnel(char* daddr, char* saddr,char* dual_daddr);
void initTunnelSystem(char* script);

#ifdef __cplusplus
}
#endif

#endif//GLORY_TUNNEL_H
