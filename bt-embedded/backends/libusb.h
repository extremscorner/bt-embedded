#ifndef BTE_BACKEND_LIBUSB_H
#define BTE_BACKEND_LIBUSB_H

#include <libusb.h>

#ifdef __cplusplus
extern "C" {
#endif

void bte_backend_libusb_set_context(libusb_context *context);

#ifdef __cplusplus
}
#endif

#endif /* BTE_BACKEND_LIBUSB_H */
