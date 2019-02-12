#ifndef GLORY_TUNNEL_H
#define GLORY_TUNNEL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "If_tunnel.h"

int do_add(int cmd, char* daddr, char* saddr,char* dual_daddr);

#ifdef __cplusplus
}
#endif

#endif//GLORY_TUNNEL_H
