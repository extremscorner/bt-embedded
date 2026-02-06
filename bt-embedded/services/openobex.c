#include "obex.h"

#include "bte.h"
#include "buffer.h"
#include "l2cap.h"
#include "logging.h"
#include "utils.h"

#include <stdlib.h>

static void message_received_cb(BteL2cap *l2cap, BteBufferReader *reader,
                                void *userdata)
{
    obex_t *handle = userdata;

    uint8_t buffer[1024];
    uint16_t len = 0;
    while (len = bte_buffer_reader_read(reader, buffer, sizeof(buffer)),
           len > 0) {
        OBEX_CustomDataFeed(handle, buffer, len);
    }
}

static int oo_connect(obex_t *handle, void *customdata)
{
    BteL2cap *l2cap = OBEX_GetCustomData(handle);

    bte_l2cap_set_userdata(l2cap, handle);
    bte_l2cap_on_message_received(l2cap, message_received_cb);
    return 0;
}

static int oo_disconnect(obex_t *handle, void *customdata)
{
    BteL2cap *l2cap = OBEX_GetCustomData(handle);

    bte_l2cap_on_message_received(l2cap, NULL);
    bte_l2cap_disconnect(l2cap);
    return 0;
}

static int oo_listen(obex_t *handle, void *customdata)
{
    return -1;
}

static int oo_write(obex_t *handle, void *customdata, uint8_t *buf, int len)
{
    BteL2cap *l2cap = OBEX_GetCustomData(handle);

    BteBufferWriter writer;
    bool ok = bte_l2cap_create_message(l2cap, &writer, len);
    if (UNLIKELY(!ok)) return -1;

    ok = bte_buffer_writer_write(&writer, buf, len);
    if (UNLIKELY(!ok)) return -1;

    int rc = bte_l2cap_send_message(l2cap, bte_buffer_writer_end(&writer));
    if (UNLIKELY(rc < 0)) return -1;

    return len;
}

static int oo_handleinput(obex_t *handle, void *customdata, int timeout)
{
    return bte_wait_events(timeout);
}

static obex_ctrans_t s_ctrans = {
    .connect = oo_connect,
    .disconnect = oo_disconnect,
    .listen = oo_listen,
    .read = NULL,
    .write = oo_write,
    .handleinput = oo_handleinput,
};

obex_ctrans_t *bte_openobex_transport()
{
    return &s_ctrans;
}

void bte_openobex_set_l2cap(obex_t *handle, BteL2cap *l2cap)
{
    OBEX_SetCustomData(handle, l2cap);
}
