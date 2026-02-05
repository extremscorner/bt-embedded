#include "obex.h"

#include "logging.h"
#include "sdp.h"
#include "utils.h"

#include <stdlib.h>


typedef struct {
    BteClient *client;
    BteObexDiscoverCb callback;
    void *userdata;
} DiscoverData;

static uint8_t find_rfcomm_channel(BteSdpDeReader *reader, bool *is_error)
{
    if (!bte_sdp_de_reader_enter(reader)) goto error;

    uint8_t channel_id = 0;
    while (bte_sdp_de_reader_next(reader)) {
    /* Each element is a sequence */
        if (!bte_sdp_de_reader_enter(reader) ||
            !bte_sdp_de_reader_next(reader)) goto error;

        uint16_t uuid = bte_sdp_de_reader_read_uuid16(reader);
        if (uuid == BTE_SDP_PROTO_RFCOMM &&
            bte_sdp_de_reader_next(reader)) {
            channel_id = bte_sdp_de_reader_read_uint8(reader);
        }

        if (!bte_sdp_de_reader_leave(reader)) goto error;
    }

    if (!bte_sdp_de_reader_leave(reader)) goto error;
    *is_error = false;
    return channel_id;

error:
    *is_error = true;
    return 0;
}

static void service_search_attr_cb(
    BteSdpClient *sdp, const BteSdpServiceAttrReply *attrs, void *userdata)
{
    DiscoverData *dd = userdata;

    BteObexDiscoverReply reply = { 0, };

    if (UNLIKELY(attrs->error_code != 0)) goto error;

    BteSdpDeReader reader;
    bte_sdp_de_reader_init(&reader, attrs->attr_list_de);
    if (!bte_sdp_de_reader_enter(&reader)) goto error;
    /* Each element is a sequence */
    while (bte_sdp_de_reader_next(&reader)) {

        if (!bte_sdp_de_reader_enter(&reader)) goto error;
        while (bte_sdp_de_reader_next(&reader)) {
            uint16_t attr_id = bte_sdp_de_reader_read_uint16(&reader);
            if (!bte_sdp_de_reader_next(&reader)) goto error;
            if (attr_id == BTE_SDP_ATTR_ID_PROTO_DESC_LIST) {
                bool is_error = false;
                reply.opp_rfcomm_channel = find_rfcomm_channel(&reader,
                                                               &is_error);
                if (UNLIKELY(is_error)) goto error;
            } else if (attr_id == BTE_SDP_ATTR_ID_GOEP_L2CAP_PSM) {
                reply.opp_l2cap_psm = bte_sdp_de_reader_read_uint16(&reader);
            }
        }

        if (!bte_sdp_de_reader_leave(&reader)) goto error;
    }

error:
    bte_sdp_client_unref(sdp);
    dd->callback(dd->client, &reply, dd->userdata);
    bte_free(dd);
}

static void new_configured_cb(
    BteL2cap *l2cap, const BteL2capNewConfiguredReply *reply, void *userdata)
{
    DiscoverData *dd = userdata;
    BteSdpClient *sdp = NULL;

    if (UNLIKELY(!l2cap)) goto error;

    sdp = bte_sdp_client_new(l2cap);
    if (UNLIKELY(!sdp)) goto error;

    uint8_t pattern[20];
    bte_sdp_de_write(pattern, sizeof(pattern),
                     BTE_SDP_DE_TYPE_SEQUENCE,
                     BTE_SDP_DE_TYPE_UUID16, BTE_SDP_SRV_CLASS_OBEX_OP,
                     BTE_SDP_DE_END,
                     BTE_SDP_DE_END);
    uint8_t id_list[20];
    bte_sdp_de_write(id_list, sizeof(id_list),
                     BTE_SDP_DE_TYPE_SEQUENCE,
                     BTE_SDP_DE_TYPE_UINT16, BTE_SDP_ATTR_ID_PROTO_DESC_LIST,
                     BTE_SDP_DE_TYPE_UINT16, BTE_SDP_ATTR_ID_GOEP_L2CAP_PSM,
                     BTE_SDP_DE_END,
                     BTE_SDP_DE_END);
    bool ok = bte_sdp_service_search_attr_req(sdp, pattern, 0x1000, id_list,
                                              service_search_attr_cb, dd);
    if (UNLIKELY(!ok)) goto error;

    return;

error:
    if (sdp) bte_sdp_client_unref(sdp);
    if (l2cap) bte_l2cap_unref(l2cap);
    BteObexDiscoverReply err_reply = { 0, };
    dd->callback(dd->client, &err_reply, dd->userdata);
    bte_free(dd);
}

bool bte_obex_discover(BteClient *client, const BteBdAddr *address,
                       const BteHciConnectParams *params,
                       BteObexDiscoverCb callback, void *userdata)
{
    DiscoverData *dd = bte_malloc(sizeof(DiscoverData));
    if (UNLIKELY(!dd)) return false;

    dd->client = client;
    dd->callback = callback;
    dd->userdata = userdata;

    bool ok = bte_l2cap_new_configured(client, address, BTE_L2CAP_PSM_SDP,
                                       params, NULL, new_configured_cb, dd);
    if (UNLIKELY(!ok)) {
        bte_free(dd);
        return false;
    }

    return true;
}
