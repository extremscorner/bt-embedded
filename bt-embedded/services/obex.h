#ifndef BTE_SERVICES_OBEX_H
#define BTE_SERVICES_OBEX_H

#include "../l2cap.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    BteL2capPsm opp_l2cap_psm;
    uint8_t opp_rfcomm_channel;
} BteObexDiscoverReply;

typedef void (*BteObexDiscoverCb)(
    BteClient *client, const BteObexDiscoverReply *reply, void *userdata);

bool bte_obex_discover(BteClient *client, const BteBdAddr *address,
                       const BteHciConnectParams *params,
                       BteObexDiscoverCb callback, void *userdata);

#ifdef __cplusplus
}
#endif

#endif /* BTE_SERVICES_OBEX_H */
