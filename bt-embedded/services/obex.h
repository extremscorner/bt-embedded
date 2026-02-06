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

#ifdef WITH_OPENOBEX

#include <openobex/obex.h>
/**
 * Creates a transport for OpenOBEX
 *
 * Pass the returned object to OBEX_RegisterCTransport().
 */
obex_ctrans_t *bte_openobex_transport();

/* This must be called before OBEX_TransportConnect().
 */
void bte_openobex_set_l2cap(obex_t *handle, BteL2cap *l2cap);

#endif /* WITH_OPENOBEX */

#ifdef __cplusplus
}
#endif

#endif /* BTE_SERVICES_OBEX_H */
