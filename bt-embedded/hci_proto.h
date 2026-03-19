#ifndef BTE_HCI_PROTO_H
#define BTE_HCI_PROTO_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup hci
 * @{
 */

/* HCI packet types */
#define BTE_HCI_DM1 0x0008
#define BTE_HCI_DM3 0x0400
#define BTE_HCI_DM5 0x4000
#define BTE_HCI_DH1 0x0010
#define BTE_HCI_DH3 0x0800
#define BTE_HCI_DH5 0x8000

#define BTE_HCI_HV1 0x0020
#define BTE_HCI_HV2 0x0040
#define BTE_HCI_HV3 0x0080

#define SCO_PTYPE_MASK (BTE_HCI_HV1 | BTE_HCI_HV2 | BTE_HCI_HV3)
#define ACL_PTYPE_MASK (~SCO_PTYPE_MASK)

/** @defgroup BteErrorG Error codes from the HCI controller
 * @{ */
/** Success code */
#define BTE_HCI_SUCCESS 0x00
/* Possible error codes */
/** Unknown HCI command */
#define BTE_HCI_UNKNOWN_BTE_HCI_COMMAND              0x01
/** No connection */
#define BTE_HCI_NO_CONNECTION                        0x02
/** Hardware failure */
#define BTE_HCI_HW_FAILURE                           0x03
/** Page timeout */
#define BTE_HCI_PAGE_TIMEOUT                         0x04
/** Authentication failure */
#define BTE_HCI_AUTHENTICATION_FAILURE               0x05
/** PIN or key missing */
#define BTE_HCI_KEY_MISSING                          0x06
/** Memory capacity exceeded */
#define BTE_HCI_MEMORY_FULL                          0x07
/** Connection timeout */
#define BTE_HCI_CONN_TIMEOUT                         0x08
/** Connection limit exceeded */
#define BTE_HCI_MAX_NUMBER_OF_CONNECTIONS            0x09
/** Synchronous connection limit to a device exceeded */
#define BTE_HCI_MAX_NUMBER_OF_SCO_CONNECTIONS        0x0A
/** Connection already exists */
#define BTE_HCI_ACL_CONNECTION_EXISTS                0x0B
/** Command disallowed */
#define BTE_HCI_COMMAND_DISSALLOWED                  0x0C
/** Connection rejected due to limited resources */
#define BTE_HCI_HOST_REJECTED_RESOURCES              0x0D
/** Connection rejected due to security reasons */
#define BTE_HCI_HOST_REJECTED_SECURITY               0x0E
/** Connection rejected due to unacceptable BD address */
#define BTE_HCI_HOST_REJECTED_BD_ADDR                0x0F
/** Connection accept timeout exceeded */
#define BTE_HCI_HOST_TIMEOUT                         0x10
/** Unsupported feature or parameter value */
#define BTE_HCI_UNSUPPORTED_FEAT_OR_PARAM            0x11
/** Invalid HCI command parameters */
#define BTE_HCI_INVALID_HCI_COMMAND_PARAMS           0x12
/** Remote user terminated connection */
#define BTE_HCI_OTHER_END_CLOSED_CONN_USER           0x13
/** Remote device terminated connection due to low resources */
#define BTE_HCI_OTHER_END_CLOSED_CONN_RESOURCES      0x14
/** Remote device terminated connection due to power off */
#define BTE_HCI_OTHER_END_CLOSED_CONN_POWER_OFF      0x15
/** Connection terminated by local host */
#define BTE_HCI_CONN_TERMINATED_BY_LOCAL_HOST        0x16
/** Repeated attempts */
#define BTE_HCI_REPEATED_ATTEMPTS                    0x17
/** Pairing not allowed */
#define BTE_HCI_PAIRING_NOT_ALLOWED                  0x18
/** Unknown LMP PDU */
#define BTE_HCI_UNKNOWN_LMP_PDU                      0x19
/** Unsupported remote feature */
#define BTE_HCI_UNSUPPORTED_REMOTE_FEATURE           0x1A
/** SCO offset rejected */
#define BTE_HCI_SCO_OFFSET_REJECTED                  0x1B
/** SCO interval rejected */
#define BTE_HCI_SCO_INTERVAL_REJECTED                0x1C
/** SCO air mode rejected */
#define BTE_HCI_SCO_AIR_MODE_REJECTED                0x1D
/** Invalid LMP parameters / Invalid LL parameters */
#define BTE_HCI_INVALID_LMP_PARAMETERS               0x1E
/** Unspecified error */
#define BTE_HCI_UNSPECIFIED_ERROR                    0x1F
/** Unsupported LMP parameter value / Unsupported LL parameter value */
#define BTE_HCI_UNSUPPORTED_LMP_PARAM                0x20
/** Role change not allowed */
#define BTE_HCI_ROLE_CHANGE_NOT_ALLOWED              0x21
/** LMP response timeout / LL response timeout */
#define BTE_HCI_LMP_RESPONSE_TIMEOUT                 0x22
/** LMP error transaction collision / LL procedure collision */
#define BTE_HCI_LMP_ERROR_TRANSACTION_COLLISION      0x23
/** LMP PDU not allowed */
#define BTE_HCI_LMP_PDU_NOT_ALLOWED                  0x24
/** Encryption mode not acceptable */
#define BTE_HCI_ENCRYPTION_MODE_NOT_ACCEPTABLE       0x25
/** Link key cannot be changed */
#define BTE_HCI_UNIT_KEY_USED                        0x26
/** Requested QoS not supported */
#define BTE_HCI_QOS_NOT_SUPPORTED                    0x27
/** Instant passed */
#define BTE_HCI_INSTANT_PASSED                       0x28
/** Pairing with unit key not supported */
#define BTE_HCI_PAIRING_UNIT_KEY_NOT_SUPPORTED       0x29
/** @} */

/**
 * @}
 */

/**
 * @ingroup InformationalG
 * @defgroup BteHciFeaturesG BteHciFeatures values
 * HCI Link manager feature masks
 * @{
 */
/** 3-slot packets */
#define BTE_HCI_FEAT_3_SLOT_PACKETS               ((uint64_t)(1 << 0))
/** 5-slot packets */
#define BTE_HCI_FEAT_5_SLOT_PACKETS               ((uint64_t)(1 << 1))
/** Encryption of packets */
#define BTE_HCI_FEAT_ENCRYPTION                   ((uint64_t)(1 << 2))
/** Transfer of the slot offset */
#define BTE_HCI_FEAT_SLOT_OFFSET                  ((uint64_t)(1 << 3))
/** Requests for timing accuracy */
#define BTE_HCI_FEAT_TIMING_ACCURACY              ((uint64_t)(1 << 4))
/** Role switch */
#define BTE_HCI_FEAT_ROLE_SWITCH                  ((uint64_t)(1 << 5))
/** Hold mode */
#define BTE_HCI_FEAT_HOLD_MODE                    ((uint64_t)(1 << 6))
/** Sniff mode */
#define BTE_HCI_FEAT_SNIFF_MODE                   ((uint64_t)(1 << 7))
/** Park state */
#define BTE_HCI_FEAT_PARK_STATE                   ((uint64_t)(1 << 8))
/** Power control requests */
#define BTE_HCI_FEAT_POWER_CONTROL_REQUESTS       ((uint64_t)(1 << 9))
/** Channel quality driven data rate (CQDDR) */
#define BTE_HCI_FEAT_CQDDR                        ((uint64_t)(1 << 10))
/** SCO link */
#define BTE_HCI_FEAT_SCO_LINK                     ((uint64_t)(1 << 11))
/** HV2 packets */
#define BTE_HCI_FEAT_HV2_PACKETS                  ((uint64_t)(1 << 12))
/** HV3 packets */
#define BTE_HCI_FEAT_HV3_PACKETS                  ((uint64_t)(1 << 13))
/** μ-law log synchronous data */
#define BTE_HCI_FEAT_Μ_LAW_LOG_SYNCHRONOUS_DATA   ((uint64_t)(1 << 14))
/** A-law log synchronous data */
#define BTE_HCI_FEAT_A_LAW_LOG_SYNCHRONOUS_DATA   ((uint64_t)(1 << 15))
/** CVSD synchronous data */
#define BTE_HCI_FEAT_CVSD_SYNCHRONOUS_DATA        ((uint64_t)(1 << 16))
/** Paging parameter negotiation */
#define BTE_HCI_FEAT_PAGING_PARAMETER_NEGOTIATION ((uint64_t)(1 << 17))
/** Power control */
#define BTE_HCI_FEAT_POWER_CONTROL                ((uint64_t)(1 << 18))
/** Transparent synchronous data */
#define BTE_HCI_FEAT_TRANSPARENT_SYNCHRONOUS_DATA ((uint64_t)(1 << 19))
/** Flow control lag (least significant bit) */
#define BTE_HCI_FEAT_FLOW_CONTROL_LAG_LSB         ((uint64_t)(1 << 20))
/** Flow control lag (middle bit) */
#define BTE_HCI_FEAT_FLOW_CONTROL_LAG_MID         ((uint64_t)(1 << 21))
/** Flow control lag (most significant bit) */
#define BTE_HCI_FEAT_FLOW_CONTROL_LAG_MSB         ((uint64_t)(1 << 22))
/** Broadcast encryption */
#define BTE_HCI_FEAT_BROADCAST_ENCRYPTION         ((uint64_t)(1 << 23))
/** Enhanced inquiry scan */
#define BTE_HCI_FEAT_ENHANCED_INQUIRY_SCAN        ((uint64_t)(1 << 27))
/** Interlaced inquiry scan */
#define BTE_HCI_FEAT_INTERLACED_INQUIRY_SCAN      ((uint64_t)(1 << 28))
/** Interlaced page scan */
#define BTE_HCI_FEAT_INTERLACED_PAGE_SCAN         ((uint64_t)(1 << 29))
/** RSSI with inquiry results */
#define BTE_HCI_FEAT_RSSI_WITH_INQUIRY_RESULTS    ((uint64_t)(1 << 30))
/** Extended SCO link (EV3 packets) */
#define BTE_HCI_FEAT_EXTENDED_SCO_LINK            ((uint64_t)(1 << 31))
/** EV4 packets */
#define BTE_HCI_FEAT_EV4_PACKETS                  ((uint64_t)(1 << 32))
/** EV5 packets */
#define BTE_HCI_FEAT_EV5_PACKETS                  ((uint64_t)(1 << 33))
/** AFH capable Peripheral */
#define BTE_HCI_FEAT_AFH_CAPABLE_SLAVE            ((uint64_t)(1 << 35))
/** AFH classification Peripheral */
#define BTE_HCI_FEAT_AFH_CLASSIFICATION_SLAVE     ((uint64_t)(1 << 36))
/** AFH capable Central */
#define BTE_HCI_FEAT_AFH_CAPABLE_MASTER           ((uint64_t)(1 << 43))
/** AFH classification Central */
#define BTE_HCI_FEAT_AFH_CLASSIFICATION_MASTER    ((uint64_t)(1 << 44))
/** Extended features */
#define BTE_HCI_FEAT_EXTENDED_FEATURES            ((uint64_t)(1 << 63))
/** @} */

/**
 * @ingroup ControllerBasebandG
 * @defgroup BteHciEventMaskG BteHciEventMask values
 * Possible values for the HCI event mask.
 * @{
 */
/** Inquiry completed */
#define BTE_HCI_EVENT_INQUIRY_COMPL                  ((uint64_t)(1 << 0))
/** Inquiry result */
#define BTE_HCI_EVENT_INQUIRY_RESULT                 ((uint64_t)(1 << 1))
/** Connection complete */
#define BTE_HCI_EVENT_CONN_COMPL                     ((uint64_t)(1 << 2))
/** Connection requested */
#define BTE_HCI_EVENT_CONN_REQUEST                   ((uint64_t)(1 << 3))
/** Disconnection complete */
#define BTE_HCI_EVENT_DISCONN_COMPL                  ((uint64_t)(1 << 4))
/** Authentication complete */
#define BTE_HCI_EVENT_AUTHENTICATION_COMPL           ((uint64_t)(1 << 5))
/** Remote name request complete */
#define BTE_HCI_EVENT_REMOTE_NAME_REQUEST_COMPL      ((uint64_t)(1 << 6))
/** Encryption change */
#define BTE_HCI_EVENT_ENCRYPTION_CHANGE              ((uint64_t)(1 << 7))
/** Change connection link key complete */
#define BTE_HCI_EVENT_CHANGE_CONN_LINK_KEY_COMPL     ((uint64_t)(1 << 8))
/** Master link key complete */
#define BTE_HCI_EVENT_MASTER_LINK_KEY_COMPL          ((uint64_t)(1 << 9))
/** Read remote features complete */
#define BTE_HCI_EVENT_READ_REMOTE_FEATURES_COMPL     ((uint64_t)(1 << 10))
/** Read remote version complete */
#define BTE_HCI_EVENT_READ_REMOTE_VERS_INFO_COMPL    ((uint64_t)(1 << 11))
/** Quality Of Service setup complete */
#define BTE_HCI_EVENT_QOS_SETUP_COMPL                ((uint64_t)(1 << 12))
/** Hardware error */
#define BTE_HCI_EVENT_HARDWARE_ERROR                 ((uint64_t)(1 << 15))
/** Flush occurred */
#define BTE_HCI_EVENT_FLUSH_OCCURRED                 ((uint64_t)(1 << 16))
/** Role change */
#define BTE_HCI_EVENT_ROLE_CHANGE                    ((uint64_t)(1 << 17))
/** Mode change */
#define BTE_HCI_EVENT_MODE_CHANGE                    ((uint64_t)(1 << 19))
/** Return link keys */
#define BTE_HCI_EVENT_RETURN_LINK_KEYS               ((uint64_t)(1 << 20))
/** PIN code requested */
#define BTE_HCI_EVENT_PIN_CODE_REQUEST               ((uint64_t)(1 << 21))
/** Link key requested */
#define BTE_HCI_EVENT_LINK_KEY_REQUEST               ((uint64_t)(1 << 22))
/** Link key notification */
#define BTE_HCI_EVENT_LINK_KEY_NOTIFICATION          ((uint64_t)(1 << 23))
/** Loopback command */
#define BTE_HCI_EVENT_LOOPBACK_COMMAND               ((uint64_t)(1 << 24))
/** Data buffer overflow */
#define BTE_HCI_EVENT_DATA_BUFFER_OVERFLOW           ((uint64_t)(1 << 25))
/** Max slots change */
#define BTE_HCI_EVENT_MAX_SLOTS_CHANGE               ((uint64_t)(1 << 26))
/** Read clock offset complete */
#define BTE_HCI_EVENT_READ_CLOCK_OFFSET_COMPL        ((uint64_t)(1 << 27))
/** Connection packet type changed */
#define BTE_HCI_EVENT_CONN_PACKET_TYPE_CHANGED       ((uint64_t)(1 << 28))
/** Quality Of Service violation */
#define BTE_HCI_EVENT_QOS_VIOLATION                  ((uint64_t)(1 << 29))
/** Page scan mode changed */
#define BTE_HCI_EVENT_PAGE_SCAN_MODE_CHANGE          ((uint64_t)(1 << 30))
/** Scan repetition mode changed */
#define BTE_HCI_EVENT_PAGE_SCAN_REP_MODE_CHANGE      ((uint64_t)(1 << 31))
/** Flow specification comlete */
#define BTE_HCI_EVENT_FLOW_SPEC_COMPL                ((uint64_t)(1 << 32))
/** Inquiry result with RSSI */
#define BTE_HCI_EVENT_INQUIRY_RESULT_WITH_RSSI       ((uint64_t)(1 << 33))
/** Read remote extended features complete */
#define BTE_HCI_EVENT_READ_REMOTE_EXT_FEATURES_COMPL ((uint64_t)(1 << 34))
/** Synchronous connection complete */
#define BTE_HCI_EVENT_SYNC_CONN_COMPL                ((uint64_t)(1 << 43))
/** Synchronous connection changed */
#define BTE_HCI_EVENT_SYNC_CONN_CHANGED              ((uint64_t)(1 << 44))
/** All events enabled (default) */
#define BTE_HCI_EVENT_ALL                            ((uint64_t)0x1fffffffffff)
/** @} */

#define BTE_HCI_MAX_NAME_LEN 248

#ifdef __cplusplus
}
#endif

#endif /* BTE_HCI_PROTO_H */
