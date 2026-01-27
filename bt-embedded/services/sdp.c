#include "sdp.h"

#include "logging.h"
#include "utils.h"

#include <stdarg.h>
#include <stdlib.h>

#define BTE_SDP_DE_SIZE_MASK      (BteSdpDeType)0x07
#define BTE_SDP_DE_SIZE_1      0
#define BTE_SDP_DE_SIZE_2      1
#define BTE_SDP_DE_SIZE_4      2
#define BTE_SDP_DE_SIZE_8      3
#define BTE_SDP_DE_SIZE_16     4
#define BTE_SDP_DE_SIZE_VAR_8  5
#define BTE_SDP_DE_SIZE_VAR_16 6
#define BTE_SDP_DE_SIZE_VAR_32 7

static const BteSdpDeUuid128 s_base_uuid = {{
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00,
    0x10, 0x00,
    0x80, 0x00,
    0x00, 0x80, 0x5f, 0x9b, 0x34, 0xfb
}};

uint32_t bte_sdp_de_get_data_size(const uint8_t *de)
{
    uint8_t size_id = de[0] & BTE_SDP_DE_SIZE_MASK;
    switch (size_id) {
    case BTE_SDP_DE_SIZE_VAR_8:
        return de[1];
    case BTE_SDP_DE_SIZE_VAR_16:
        return read_be16(de + 1);
    case BTE_SDP_DE_SIZE_VAR_32:
        return read_be32(de + 1);
    default:
        if (de[0] == BTE_SDP_DE_TYPE_NIL) return 0;
        return 1 << size_id;
    }
}

uint32_t bte_sdp_de_get_header_size(const uint8_t *de)
{
    uint8_t size_id = de[0] & BTE_SDP_DE_SIZE_MASK;
    switch (size_id) {
    case BTE_SDP_DE_SIZE_VAR_8:
        return 1 + 1;
    case BTE_SDP_DE_SIZE_VAR_16:
        return 1 + 2;
    case BTE_SDP_DE_SIZE_VAR_32:
        return 1 + 4;
    default:
        return 1;
    }
}

uint32_t bte_sdp_de_get_total_size(const uint8_t *de)
{
    return bte_sdp_de_get_header_size(de) + bte_sdp_de_get_data_size(de);
}

BteSdpDeType bte_sdp_de_get_type(const uint8_t *de)
{
    uint8_t type_id = de[0] & BTE_SDP_DE_TYPE_MASK;
    return type_id >= BTE_SDP_DE_TYPE_STRING ? type_id : de[0];
}

static uint8_t var_size(size_t len, int *header_size, uint8_t *dest)
{
    uint8_t size_id;
    if (len < 0x100) {
        size_id = BTE_SDP_DE_SIZE_VAR_8;
        if (header_size) *header_size = 1 + 1;
        if (dest) dest[1] = len;
    } else if (len < 0x10000) {
        size_id = BTE_SDP_DE_SIZE_VAR_16;
        if (header_size) *header_size = 1 + 2;
        if (dest) write_be16(len, dest + 1);
    } else {
        size_id = BTE_SDP_DE_SIZE_VAR_32;
        if (header_size) *header_size = 1 + 4;
        if (dest) write_be32(len, dest + 1);
    }
    return size_id;
}

static uint32_t de_write(uint8_t *de, size_t buffer_size, va_list *args)
{
    uint32_t size = 0;
    uint8_t *dest = de;
    ssize_t remaining = de ? buffer_size : 0;
    int type = -1;
    while (true) {
        type = va_arg(*args, int);
        if (type == BTE_SDP_DE_END) break;

        bool flip = false;
        ssize_t req_size = 1;
        uint8_t v8;
        uint16_t v16;
        uint32_t v32;
        uint64_t v64;
        uint8_t *vptr;
        switch (type) {
        case BTE_SDP_DE_TYPE_NIL:
            if (remaining > req_size) {
                dest[0] = type;
            }
            break;
        case BTE_SDP_DE_TYPE_INT8:
        case BTE_SDP_DE_TYPE_UINT8:
            req_size = 1 + 1;
            v8 = (uint8_t)va_arg(*args, int);
            if (remaining > req_size) {
                dest[0] = type;
                dest[1] = v8;
            }
            break;
        case BTE_SDP_DE_TYPE_INT16:
        case BTE_SDP_DE_TYPE_UINT16:
        case BTE_SDP_DE_TYPE_UUID16:
            req_size = 1 + 2;
            v16 = va_arg(*args, int);
            if (remaining > req_size) {
                dest[0] = type;
                write_be16(v16, dest + 1);
            }
            break;
        case BTE_SDP_DE_TYPE_INT32:
        case BTE_SDP_DE_TYPE_UINT32:
        case BTE_SDP_DE_TYPE_UUID32:
            req_size = 1 + 4;
            v32 = va_arg(*args, uint32_t);
            if (remaining > req_size) {
                dest[0] = type;
                write_be32(v32, dest + 1);
            }
            break;
        case BTE_SDP_DE_TYPE_INT64:
        case BTE_SDP_DE_TYPE_UINT64:
            v64 = va_arg(*args, uint64_t);
            req_size = 1 + 8;
            if (remaining > req_size) {
                dest[0] = type;
                write_be64(v64, dest + 1);
            }
            break;
        case BTE_SDP_DE_TYPE_INT128:
        case BTE_SDP_DE_TYPE_UINT128:
#if __BYTE_ORDER == __LITTLE_ENDIAN
            flip = true;
            // fallthrough
#endif
        case BTE_SDP_DE_TYPE_UUID128:
            req_size = 1 + 16;
            vptr = va_arg(*args, uint8_t *);
            if (remaining > req_size) {
                dest[0] = type;
                memcpy(dest + 1, vptr, 16);
                if (flip) {
                    uint64_t tmp = *((uint64_t*)(dest + 1));
                    write_be64(*((uint64_t*)(dest + 1 + 8)), dest + 1);
                    write_be64(tmp, dest + 1 + 8);
                }
            }
            break;
        case BTE_SDP_DE_TYPE_BOOL:
            req_size = 1 + 1;
            v8 = va_arg(*args, int) != 0 ? 1 : 0;
            if (remaining > req_size) {
                dest[0] = type;
                dest[1] = v8;
            }
            break;
        case BTE_SDP_DE_TYPE_STRING:
        case BTE_SDP_DE_TYPE_URL:
            {
                const char *str = va_arg(*args, char *);
                size_t len = strlen(str);
                int header_size;
                uint8_t size_id = var_size(len, &header_size, NULL);
                req_size = header_size + len;
                if (remaining > req_size) {
                    dest[0] = type | size_id;
                    var_size(len, NULL, dest);
                    memcpy(dest + header_size, str, len);
                }
            }
            break;
        case BTE_SDP_DE_TYPE_SEQUENCE:
        case BTE_SDP_DE_TYPE_CHOICE:
            {
                /* First, we need to compute the size of the included data */
                va_list args_copy;
                va_copy(args_copy, *args);
                uint32_t len = de_write(NULL, 0, args);
                int header_size;
                uint8_t size_id = var_size(len, &header_size, NULL);
                req_size = header_size + len;
                if (remaining > req_size) {
                    dest[0] = type | size_id;
                    var_size(len, NULL, dest);
                    de_write(dest + header_size, remaining - header_size, &args_copy);
                }
                va_end(args_copy);
            }
            break;
        }
        if (dest) dest += req_size;
        size += req_size;
        remaining -= req_size;
    }

    return size;
}

uint32_t bte_sdp_de_write(uint8_t *de, size_t buffer_size, ...)
{
    va_list args;
    va_start(args, buffer_size);
    uint32_t size = de_write(de, buffer_size, &args);
    va_end(args);
    return size;
}
