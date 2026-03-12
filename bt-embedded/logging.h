#ifndef BTE_LOGGING_H
#define BTE_LOGGING_H

#include "platform_defs.h"

#include <inttypes.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/* BTE_LOG might have been defined in platform_defs.h */
#ifndef BTE_LOG
#define BTE_LOG(fmt, ...) fprintf(stderr, fmt, ##__VA_ARGS__)
#endif

#define BTE_WARN(fmt, ...) BTE_LOG("[E] " fmt "\n", ##__VA_ARGS__)
#ifdef WITH_DEBUG
#define BTE_DEBUG(fmt, ...) BTE_LOG("[D] %s:%d: " fmt "\n", __func__, __LINE__, ##__VA_ARGS__)
#define BTE_INFO(fmt, ...) BTE_LOG("[I] " fmt "\n", ##__VA_ARGS__)
#else
#define BTE_DEBUG(fmt, ...) (void)0
#define BTE_INFO(fmt, ...) (void)0
#endif

#ifdef __cplusplus
}
#endif

#endif /* BTE_LOGGING_H */
