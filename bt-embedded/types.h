#ifndef BTE_TYPES_H
#define BTE_TYPES_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Common **public** types used by the library and its clients */

#define BTE_PACKED __attribute__((packed))

typedef struct bte_buffer_t BteBuffer;
typedef struct bte_client_t BteClient;
typedef struct bte_hci_t BteHci;
typedef struct bte_l2cap_t BteL2cap;

typedef struct {
    uint8_t bytes[6];
} BteBdAddr;

typedef struct {
    uint8_t bytes[3];
} BteClassOfDevice;

static inline uint16_t bte_cod_get_service_class(BteClassOfDevice cod) {
    return (cod.bytes[2] << 3) | (cod.bytes[1] >> 5);
}

static inline uint8_t bte_cod_get_major_dev_class(BteClassOfDevice cod) {
    return cod.bytes[1] & 0x1f;
}

static inline uint8_t bte_cod_get_minor_dev_class(BteClassOfDevice cod) {
    return cod.bytes[0] >> 2;
}

static inline BteClassOfDevice bte_cod_compose(
    uint16_t service_class, uint8_t major_dev_class, uint8_t minor_dev_class) {
    BteClassOfDevice cod = {{
        (uint8_t)(minor_dev_class << 2),
        (uint8_t)(major_dev_class | (service_class << 5)),
        (uint8_t)(service_class >> 3),
    }};
    return cod;
}

/** Limited Discoverable Mode */
#define BTE_COD_SERVICE_CLASS_LDM       (uint16_t)(1 << 0)
#define BTE_COD_SERVICE_CLASS_LE_AUDIO  (uint16_t)(1 << 1) /**< LE audio */
/** Positioning (local identification) */
#define BTE_COD_SERVICE_CLASS_POSITION  (uint16_t)(1 << 3)
/** Networking (LAN, Ad hoc, ...) */
#define BTE_COD_SERVICE_CLASS_NETWORK   (uint16_t)(1 << 4)
/** Rendering (Printing, Speakers, ...) */
#define BTE_COD_SERVICE_CLASS_RENDER    (uint16_t)(1 << 5)
/** Capturing (Scanner, Microphone, ...) */
#define BTE_COD_SERVICE_CLASS_CAPTURE   (uint16_t)(1 << 6)
/** Object Transfer (v-Inbox, v-Folder, ...) */
#define BTE_COD_SERVICE_CLASS_TRANSFER  (uint16_t)(1 << 7)
/** Audio (Speaker, Microphone, Headset service, ...) */
#define BTE_COD_SERVICE_CLASS_AUDIO     (uint16_t)(1 << 8)
/** Telephony (Cordless telephony, Modem, Headset service, ...) */
#define BTE_COD_SERVICE_CLASS_TELEPHONY (uint16_t)(1 << 9)
/** Information (WEB-server, WAP-server, ...) */
#define BTE_COD_SERVICE_CLASS_INFO      (uint16_t)(1 << 10)

/** Miscellaneous */
#define BTE_COD_MAJOR_DEV_CLASS_MISC     (uint8_t)0x0
/** Computer (desktop, notebook, PDA, organizer, ...) */
#define BTE_COD_MAJOR_DEV_CLASS_COMPUTER (uint8_t)0x1
/** Phone (cellular, cordless, pay phone, modem, ...) */
#define BTE_COD_MAJOR_DEV_CLASS_PHONE    (uint8_t)0x2
/** LAN/Network Access Point */
#define BTE_COD_MAJOR_DEV_CLASS_NETWORK  (uint8_t)0x3
/** Audio/Video (headset, speaker, stereo, video display, VCR, ...) */
#define BTE_COD_MAJOR_DEV_CLASS_AV       (uint8_t)0x4
/** Peripheral (mouse, joystick, keyboard, ...) */
#define BTE_COD_MAJOR_DEV_CLASS_PERIPH   (uint8_t)0x5
/** Imaging (printer, scanner, camera, display, ...) */
#define BTE_COD_MAJOR_DEV_CLASS_IMAGING  (uint8_t)0x6
/** Wearable */
#define BTE_COD_MAJOR_DEV_CLASS_WEAR     (uint8_t)0x7
/** Toy */
#define BTE_COD_MAJOR_DEV_CLASS_TOY      (uint8_t)0x8
/** Health */
#define BTE_COD_MAJOR_DEV_CLASS_HEALTH   (uint8_t)0x9
/** Uncategorized (device code not specified) */
#define BTE_COD_MAJOR_DEV_CLASS_UNCAT    (uint8_t)0x1f

typedef struct {
    uint8_t bytes[16];
} BteLinkKey;

typedef uint32_t BteLap;

typedef uint16_t BtePacketType;

#define BTE_PACKET_TYPE_DM1 (BtePacketType)0x0008
#define BTE_PACKET_TYPE_DH1 (BtePacketType)0x0010
#define BTE_PACKET_TYPE_DM3 (BtePacketType)0x0400
#define BTE_PACKET_TYPE_DH3 (BtePacketType)0x0800
#define BTE_PACKET_TYPE_DH5 (BtePacketType)0x4000
#define BTE_PACKET_TYPE_DM5 (BtePacketType)0x8000

typedef uint16_t BteConnHandle;

/* The connection handle is 12 bits, so this value is certainly invalid */
#define BTE_CONN_HANDLE_INVALID (BteConnHandle)0xffff

typedef uint8_t BteLinkType;

#define BTE_LINK_TYPE_SCO  (BteLinkType)0
#define BTE_LINK_TYPE_ACL  (BteLinkType)1
#define BTE_LINK_TYPE_ESCO (BteLinkType)2

#ifdef __cplusplus
}
#endif

#endif
