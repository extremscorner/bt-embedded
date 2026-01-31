#ifndef BTE_SDP_H
#define BTE_SDP_H

#include "../types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef uint8_t BteSdpDeType;
typedef struct {
    uint8_t bytes[16];
} BteSdpDeUuid128;

#define BTE_SDP_DE_TYPE_MASK      (BteSdpDeType)0xf8

#define BTE_SDP_DE_TYPE_NIL       (BteSdpDeType)0x00
#define BTE_SDP_DE_TYPE_UINT      (BteSdpDeType)0x08
#define BTE_SDP_DE_TYPE_UINT8     (BteSdpDeType)0x08
#define BTE_SDP_DE_TYPE_UINT16    (BteSdpDeType)0x09
#define BTE_SDP_DE_TYPE_UINT32    (BteSdpDeType)0x0a
#define BTE_SDP_DE_TYPE_UINT64    (BteSdpDeType)0x0b
#define BTE_SDP_DE_TYPE_UINT128   (BteSdpDeType)0x0c
#define BTE_SDP_DE_TYPE_INT       (BteSdpDeType)0x10
#define BTE_SDP_DE_TYPE_INT8      (BteSdpDeType)0x10
#define BTE_SDP_DE_TYPE_INT16     (BteSdpDeType)0x11
#define BTE_SDP_DE_TYPE_INT32     (BteSdpDeType)0x12
#define BTE_SDP_DE_TYPE_INT64     (BteSdpDeType)0x13
#define BTE_SDP_DE_TYPE_INT128    (BteSdpDeType)0x14
#define BTE_SDP_DE_TYPE_UUID      (BteSdpDeType)0x18
#define BTE_SDP_DE_TYPE_UUID16    (BteSdpDeType)0x19
#define BTE_SDP_DE_TYPE_UUID32    (BteSdpDeType)0x1a
#define BTE_SDP_DE_TYPE_UUID128   (BteSdpDeType)0x1c
#define BTE_SDP_DE_TYPE_STRING    (BteSdpDeType)(4 << 3)
#define BTE_SDP_DE_TYPE_BOOL      (BteSdpDeType)(5 << 3)
#define BTE_SDP_DE_TYPE_SEQUENCE  (BteSdpDeType)(6 << 3)
#define BTE_SDP_DE_TYPE_CHOICE    (BteSdpDeType)(7 << 3)
#define BTE_SDP_DE_TYPE_URL       (BteSdpDeType)(8 << 3)

#define BTE_SDP_DE_END (-1)
#define BTE_SDP_DE_SPECIAL_MASK   0xff000000
#define BTE_SDP_DE_SPECIAL_ARRAY  0xaa000000
#define BTE_SDP_DE_ARRAY(n)       (n | BTE_SDP_DE_SPECIAL_ARRAY)

uint32_t bte_sdp_de_get_data_size(const uint8_t *de);
uint32_t bte_sdp_de_get_header_size(const uint8_t *de);
uint32_t bte_sdp_de_get_total_size(const uint8_t *de);

BteSdpDeType bte_sdp_de_get_type(const uint8_t *de);

/* Returns 0 on error; call it with \a buffer_size set to 0 in order to receive
 * the size needed for storing the data element */
uint32_t bte_sdp_de_write(uint8_t *de, size_t buffer_size, ...);

typedef struct bte_sdp_client_t BteSdpClient;

/* Note: this replaces the l2cap's userdata with its own; the l2cap userdata is
 * moved into the SdpClient's userdata.
 * \sa bte_sdp_client_get_userdata */
BteSdpClient *bte_sdp_client_new(BteL2cap *l2cap);

BteSdpClient *bte_sdp_client_ref(BteSdpClient *sdp);
void bte_sdp_client_unref(BteSdpClient *sdp);

void bte_sdp_client_set_userdata(BteSdpClient *sdp, void *userdata);
void *bte_sdp_client_get_userdata(BteSdpClient *sdp);

BteL2cap *bte_sdp_client_get_l2cap(BteSdpClient *sdp);

typedef struct {
    uint16_t error_code;
    uint16_t total_count;
    uint16_t count; /* In the current reply */
    bool has_more;
    const uint32_t *handles;
} BteSdpServiceSearchReply;

typedef bool (*BteSdpServiceSearchCb)(BteSdpClient *sdp,
                                      const BteSdpServiceSearchReply *reply,
                                      void *userdata);
bool bte_sdp_service_search_req(BteSdpClient *sdp, const uint8_t *pattern,
                                uint16_t max_count,
                                BteSdpServiceSearchCb cb, void *userdata);
bool bte_sdp_service_search_req_uuid16(
    BteSdpClient *sdp, uint16_t *uuids, int n_uuids, uint16_t max_count,
    BteSdpServiceSearchCb cb, void *userdata);

/* For testing only */
void bte_sdp_reset();

#ifdef __cplusplus
}
#endif

#endif /* BTE_SDP_H */
