#include "backend.h"
#include "internals.h"
#include "logging.h"
#include "utils.h"

#include <assert.h>
#include <errno.h>
#include <ogc/machine/processor.h>
#include <ogc/system.h>
#include <ogc/usb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tuxedo/mailbox.h>
#include <tuxedo/tick.h>

#define USB_VENDOR_NINTENDO 0x057E
#define USB_PRODUCT_WII_BT  0x0305

#define ENDPOINT_HCI_INTR 0x81
#define ENDPOINT_ACL_IN   0x82
#define ENDPOINT_ACL_OUT  0x02

/* The max size of an HCI event is 255 (payload) + 3 (header); here we round it
 * up a bit */
#define CTRL_BUF_SIZE 264
/* Typical MTU is 339; we round it up */
#define ACL_BUF_SIZE  352

#define WII_MAX_EVENTS        128
#define WII_BUFFER_INTR_COUNT 16
#define WII_BUFFER_DATA_COUNT 64

typedef enum {
    WII_EVENT_SENT,
    WII_EVENT_INTR,
    WII_EVENT_DATA,
} WiiEventType;

typedef struct wii_event_t WiiEvent;

typedef struct {
    struct wii_event_t {
        WiiEventType type;
        BteBuffer *buffer;
    } ATTRIBUTE_PACKED events[WII_MAX_EVENTS];
    int current_index;
    int missed_events;
} WiiEventQueue;

typedef struct {
    BteBuffer buffer;
    uint8_t data[CTRL_BUF_SIZE];
} WiiBufferIntr;

typedef struct {
    BteBuffer buffer;
    uint8_t data[ACL_BUF_SIZE];
} WiiBufferData;

static WiiBufferIntr *s_wii_buffer_intr;
static WiiBufferData *s_wii_buffer_data;
static WiiEventQueue s_wii_event_queue;
static bool s_intr_request_failed = false;
static bool s_bulk_request_failed = false;

static int s_bt_fd = -1;
static KMailbox s_event_sem;
static KTickTask s_event_timer;
/* We need one slot for the timeout message, and another to tell whether the
 * queue has any events; but let's add some more slots just to be safe. */
static uptr s_event_sem_slots[4];

static int read_intr();
static int read_bulk();

static void wii_buffer_free(BteBuffer *buffer)
{
    /* If the CPU is slow in processing events, we might run short of buffers;
     * in that case, we set the s_intr_request_failed and s_bulk_request_failed
     * flags. Here we have just released one buffer, so we can restart the
     * reading chains. */
    if (UNLIKELY(s_intr_request_failed)) {
        s_intr_request_failed = false;
        read_intr();
    } else if (UNLIKELY(s_bulk_request_failed)) {
        s_bulk_request_failed = false;
        read_bulk();
    }
}

static void *alloc_usb_buffers(int size)
{
    /* For some reason we need to allocate the buffers for the USB incoming
     * data in MEM2, otherwise some bytes in the payload get corrupted */
    void *ptr = (void *)ROUNDDOWN32(((u32)SYS_GetArena2Hi() - size));
    if ((u32)ptr < (u32)SYS_GetArena2Lo()) return NULL;
    SYS_SetArena2Hi(ptr);
    memset(ptr, 0, size);
    return ptr;
}

static BteBuffer *wii_buffer_intr_alloc(uint16_t size)
{
    for (int i = 0; i < WII_BUFFER_INTR_COUNT; i++) {
        BteBuffer *buffer = &s_wii_buffer_intr[i].buffer;
        if (buffer->ref_count == 0) {
            buffer->ref_count = 1;
            buffer->free_func = wii_buffer_free;
            buffer->total_size = buffer->size = size;
            buffer->next = NULL;
            return buffer;
        }
    }
    return NULL;
}

static BteBuffer *wii_buffer_data_alloc(uint16_t size)
{
    for (int i = 0; i < WII_BUFFER_DATA_COUNT; i++) {
        BteBuffer *buffer = &s_wii_buffer_data[i].buffer;
        if (buffer->ref_count == 0) {
            buffer->ref_count = 1;
            buffer->free_func = wii_buffer_free;
            buffer->total_size = buffer->size = size;
            buffer->next = NULL;
            return buffer;
        }
    }
    return NULL;
}

static inline void queue_event(WiiEventType type, BteBuffer *buffer)
{
    if (s_wii_event_queue.current_index >= WII_MAX_EVENTS) {
        s_wii_event_queue.missed_events++;
        return;
    }

    WiiEvent *e = &s_wii_event_queue.events[s_wii_event_queue.current_index++];
    e->type = type;
    e->buffer = buffer;
    KMailboxTrySend(&s_event_sem, (uptr)&s_wii_event_queue);
}

static s32 read_intr_cb(s32 result, void *userdata)
{
    BteBuffer *buf = userdata;

    if (result > 0) {
        bte_buffer_shrink(buf, result);
        queue_event(WII_EVENT_INTR, buf);
    } else {
        bte_buffer_unref(buf);
    }
    return read_intr();
}

static s32 read_intr()
{
    if (UNLIKELY(s_bt_fd < 0)) return -EBADF;

    u16 len = CTRL_BUF_SIZE;
    BteBuffer *buf = wii_buffer_intr_alloc(len);
    if (!buf) {
        s_intr_request_failed = true;
        return -ENOMEM;
    }

    u8 *ptr = bte_buffer_contiguous_data(buf, len);
    s32 rc = USB_ReadIntrMsgAsync(s_bt_fd, ENDPOINT_HCI_INTR, len, ptr,
                                  read_intr_cb, buf);
    return rc;
}

static s32 read_bulk_cb(s32 result, void *userdata)
{
    BteBuffer *buf = userdata;

    if (result > 0) {
        bte_buffer_shrink(buf, result);
        queue_event(WII_EVENT_DATA, buf);
    } else {
        bte_buffer_unref(buf);
    }

    return read_bulk();
}

static s32 read_bulk()
{
    if (UNLIKELY(s_bt_fd < 0)) return -EBADF;

    u16 len = ACL_BUF_SIZE;
    BteBuffer *buf = wii_buffer_data_alloc(len);
    if (!buf) {
        s_bulk_request_failed = true;
        return -ENOMEM;
    }

    u8 *ptr = bte_buffer_contiguous_data(buf, len);
    s32 ret = USB_ReadBlkMsgAsync(s_bt_fd, ENDPOINT_ACL_IN, len, ptr,
                                  read_bulk_cb, buf);
    return ret;
}

static int wii_init()
{
    int rc = USB_Initialize();
    BTE_DEBUG("USB_Initialize returned %d", rc);
    if (rc != USB_OK) return -1;

    rc = USB_OpenDevice(USB_OH1_DEVICE_ID,
                        USB_VENDOR_NINTENDO,
                        USB_PRODUCT_WII_BT,
                        &s_bt_fd);
    BTE_DEBUG("USB_OpenDevice returned %d", rc);
    if (rc != USB_OK) return -1;

    KMailboxPrepare(&s_event_sem, s_event_sem_slots, ARRAY_SIZE(s_event_sem_slots));

    s_wii_buffer_intr =
        alloc_usb_buffers(sizeof(WiiBufferIntr) * WII_BUFFER_INTR_COUNT);
    s_wii_buffer_data =
        alloc_usb_buffers(sizeof(WiiBufferData) * WII_BUFFER_DATA_COUNT);
    if (!s_wii_buffer_intr || !s_wii_buffer_data)
        return -ENOMEM;

    read_intr();
    read_bulk();

    return 0;
}

static void event_timer_cb(KTickTask *timer)
{
    KMailboxTrySend(&s_event_sem, (uptr)timer);
}

static int wii_handle_events(bool wait_for_events, uint32_t timeout_us)
{
    WiiEventQueue queue;
    u32 level;

    bool has_events = 0;
    uptr message;
    while (KMailboxTryRecv(&s_event_sem, &message)) {
        has_events = true;
    }
    if (!has_events && wait_for_events) {
        u64 timeout_ticks = PPCUsToTicks(timeout_us);
        KTickTaskStart(&s_event_timer, event_timer_cb, timeout_ticks, 0);
        /* We disable interrupts because we don't want our timeout alarm to
         * trigger (and therefore increment the semaphore) between the time
         * that KMailboxRecv returns and the time we cancel it. */
        _CPU_ISR_Disable(level);
        message = KMailboxRecv(&s_event_sem);
        if (message == (uptr)&s_event_timer) {
            _CPU_ISR_Restore(level);
            return 0;
        } else {
            KTickTaskStop(&s_event_timer);
            has_events = true;
        }
        _CPU_ISR_Restore(level);
    }

    _CPU_ISR_Disable(level);
    /* Create a copy of the queue to ensure it doesn't get modified while we
     * process it */
    memcpy(&queue, &s_wii_event_queue, sizeof(queue));
    s_wii_event_queue.current_index = 0;
    s_wii_event_queue.missed_events = 0;
    _CPU_ISR_Restore(level);

    if (UNLIKELY(queue.missed_events > 0)) {
        BTE_WARN("%d events were not reported!", queue.missed_events);
    }

    for (int i = 0; i < queue.current_index; i++) {
        WiiEvent *event = &queue.events[i];

        if (event->type == WII_EVENT_INTR) {
            _bte_hci_dev_handle_event(event->buffer);
        } else if (event->type == WII_EVENT_DATA) {
            _bte_hci_dev_handle_data(event->buffer);
        }
        bte_buffer_unref(event->buffer);
    }

    return queue.current_index;
}

static s32 hci_send_command_cb(s32 result, void *userdata)
{
    BteBuffer *buf = userdata;
    queue_event(WII_EVENT_SENT, buf);
    return result;
}

static int wii_hci_send_command(BteBuffer *buf)
{
    if (UNLIKELY(s_bt_fd < 0)) return -EBADF;

    int rc;
    if (buf->total_size == buf->size) {
        bte_buffer_ref(buf);
        rc = USB_WriteCtrlMsgAsync(s_bt_fd, 0x20, 0, 0, 0,
                                   buf->size, buf->data,
                                   hci_send_command_cb, buf);
        if (UNLIKELY(rc != USB_OK)) {
            bte_buffer_unref(buf);
            rc = -EIO;
        }
    } else {
        /* HCI commands are not big, they should all fit in a single packet */
        BTE_WARN("Sending command with size %d", buf->total_size);
        rc = -ENOMEM;
    }
    return rc;
}

static s32 hci_send_data_cb(s32 result, void *userdata)
{
    BteBuffer *buf = userdata;
    bte_buffer_unref(buf);
    return result;
}

static int wii_hci_send_data(BteBuffer *buf)
{
    if (UNLIKELY(s_bt_fd < 0)) return -EBADF;

    bte_buffer_ref(buf);
    int rc = USB_WriteBlkMsgAsync(s_bt_fd, ENDPOINT_ACL_OUT,
                                  buf->size, buf->data,
                                  hci_send_data_cb, buf);
    if (UNLIKELY(rc != USB_OK)) {
        bte_buffer_unref(buf);
        rc = -EIO;
    }
    return rc;
}

static int wii_deinit()
{
    USB_CloseDevice(&s_bt_fd);

    return 0;
}

const BteBackend _bte_backend = {
    .init = wii_init,

    .handle_events = wii_handle_events,

    .hci_send_command = wii_hci_send_command,
    .hci_send_data = wii_hci_send_data,

    .deinit = wii_deinit,
};
