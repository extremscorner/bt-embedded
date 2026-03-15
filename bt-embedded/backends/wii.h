#ifndef BTE_BACKEND_WII_H
#define BTE_BACKEND_WII_H

#include <ogc/message.h>

#ifdef __cplusplus
extern "C" {
#endif

void bte_backend_wii_set_mailbox(mqbox_t mailbox);
int bte_backend_wii_process_events();

#ifdef __cplusplus
}
#endif

#endif /* BTE_BACKEND_WII_H */
