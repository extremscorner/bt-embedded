#include "terminal.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "bt-embedded/bte.h"
#include "bt-embedded/client.h"
#include "bt-embedded/hci.h"
#include "bt-embedded/services/obex.h"


static struct {
    int num_responses;
    int curr_index;
    BteHciInquiryResponse *responses;
} s_inquiry_responses;

static BteBdAddr s_peerAddress;

static void inquiry_status_cb(BteHci *hci, const BteHciReply *reply, void *)
{
    printf("Inquiry issued, status = %d\n", reply->status);
}

static inline void print_hex(const uint8_t *b, int n, char sep)
{
    for (int i = 0; i < n; i++) {
        printf("%02x%c", b[i], (i == n - 1) ? ' ' : sep);
    }
}

static inline void print_addr(const BteBdAddr *address)
{
    print_hex(address->bytes, 6, ':');
}

#ifdef WITH_OPENOBEX
static obex_t *s_obex = NULL;
static enum {
    NOT_STARTED,
    CONNECTING,
    CONNECTED,
    SENDING_FILE,
    SENT_FILE,
    DISCONNECTING,
    DISCONNECTED,
} s_obex_state = NOT_STARTED;

static uint32_t file_size(FILE *file)
{
    struct stat buf;
    return fstat(fileno(file), &buf) == 0 ? buf.st_size : 0;
}

/* Taken from openobex, with minor modifications */
int OBEX_CharToUnicode(uint8_t *uc, const char *c, int size)
{
	int len, n;

	if (uc == NULL || c == NULL)
		return -1;

	len = n = strlen(c);
	if (n * 2 + 2 > size) return -1;

	uc[n * 2 + 1] = 0;
	uc[n * 2] = 0;

	while (n--) {
		uc[n * 2 + 1] = (uint8_t)c[n];
		uc[n * 2] = 0;
	}

	return (len * 2) + 2;
}

static void send_file(obex_t *handle, const char *filepath)
{
    printf("Sending file %s\n", filepath);
    obex_object_t *object = OBEX_ObjectNew(handle, OBEX_CMD_PUT);

    FILE *file = fopen(filepath, "r");
    OBEX_SetUserData(handle, file);

    const char *last_slash = strrchr(filepath, '/');
    const char *filename = last_slash ? (last_slash + 1) : filepath;
    uint8_t fn_buffer[200];
    int fn_len = OBEX_CharToUnicode(fn_buffer, filename, sizeof(fn_buffer));
    obex_headerdata_t hdd;
    hdd.bs = fn_buffer;
    OBEX_ObjectAddHeader(handle, object, OBEX_HDR_NAME,
                         hdd, fn_len, 0);

    hdd.bq4 = file_size(file);
    OBEX_ObjectAddHeader(handle, object, OBEX_HDR_LENGTH,
                         hdd, sizeof(uint32_t), 0);

    hdd.bs = NULL;
    OBEX_ObjectAddHeader(handle, object, OBEX_HDR_BODY,
                         hdd, 0, OBEX_FL_STREAM_START);

    OBEX_Request(handle, object);
}

static void fill_body(obex_t *handle, obex_object_t *object, FILE *file)
{
    static uint8_t buffer[1000];
    obex_headerdata_t hdd;

    size_t len = fread(buffer, 1, sizeof(buffer), file);
    if (len > 0) {
        hdd.bs = buffer;
        OBEX_ObjectAddHeader(handle, object, OBEX_HDR_BODY,
                             hdd, len, OBEX_FL_STREAM_DATA);
    } else if (len == 0) {
        hdd.bs = buffer;
        OBEX_ObjectAddHeader(handle, object, OBEX_HDR_BODY,
                             hdd, len, OBEX_FL_STREAM_DATAEND);
    } else { /* error! */
        hdd.bs = NULL;
        OBEX_ObjectAddHeader(handle, object, OBEX_HDR_BODY,
                             hdd, 0, OBEX_FL_STREAM_DATA);
    }
}

static void obex_event_cb(obex_t *handle, obex_object_t *obj, int mode,
                          int event, int obex_cmd, int obex_rsp)
{
    switch (event) {
    case OBEX_EV_REQDONE:
        if (obex_cmd == OBEX_CMD_CONNECT) {
            if (obex_rsp == OBEX_RSP_SUCCESS) {
                s_obex_state = CONNECTED;
            }
        } else if (obex_cmd == OBEX_CMD_PUT) {
            if (obex_rsp == OBEX_RSP_SUCCESS) {
                s_obex_state = SENT_FILE;
            }
        } else if (obex_cmd == OBEX_CMD_DISCONNECT) {
            if (obex_rsp == OBEX_RSP_SUCCESS) {
                s_obex_state = DISCONNECTED;
            }
        }
        break;
    case OBEX_EV_STREAMEMPTY:
        fill_body(handle, obj, OBEX_GetUserData(handle));
        break;
    }
}

static void disconnect(obex_t *handle)
{
    obex_object_t *object = OBEX_ObjectNew(handle, OBEX_CMD_DISCONNECT);
    OBEX_Request(handle, object);
}
#endif

static void new_configured_cb(
    BteL2cap *l2cap, const BteL2capNewConfiguredReply *reply, void *userdata)
{
    printf("L2CAP configured, result %d, l2cap valid %d\n", reply->result, l2cap != NULL);
    if (reply->result != 0) return;

#ifdef WITH_OPENOBEX
    obex_t *handle = OBEX_Init(OBEX_TRANS_CUSTOM, obex_event_cb, 0);
    OBEX_RegisterCTransport(handle, bte_openobex_transport());
    bte_openobex_set_l2cap(handle, bte_l2cap_ref(l2cap));

    struct sockaddr saddr; /* unused */
    int rc = OBEX_TransportConnect(handle, &saddr, 0);
    printf("OBEX_TransportConnect returned %d\n", rc);
    if (rc < 0) return;

    obex_object_t *object = OBEX_ObjectNew(handle, OBEX_CMD_CONNECT);
    OBEX_Request(handle, object);

    s_obex = handle;
#endif
}

static void obex_discover_cb(BteClient *client, const BteObexDiscoverReply *reply,
                             void *userdata)
{
    printf("OBEX RFCOMM channel %d, L2CAP PSM %d\n",
           reply->opp_rfcomm_channel,
           reply->opp_l2cap_psm);

    if (reply->opp_l2cap_psm == 0) return;

    bool ok = bte_l2cap_new_configured(client, &s_peerAddress,
                                       reply->opp_l2cap_psm, NULL, 0, NULL,
                                       new_configured_cb, NULL);
    printf("bte_l2cap_new_configured returned %d\n", ok);
}

static bool address_is_new(const BteBdAddr *address)
{
    for (int i = 0; i < s_inquiry_responses.num_responses; i++) {
        if (memcmp(address, &s_inquiry_responses.responses[i].address, 6) == 0)
            return false;
    }
    return true;
}

static void inquiry_cb(BteHci *hci, const BteHciInquiryReply *reply, void *)
{
    printf("Inquiry done, status = %d\n", reply->status);
    if (reply->status != 0) return;

    printf("Results: %d\n", reply->num_responses);

    s_inquiry_responses.num_responses = 0;
    s_inquiry_responses.curr_index = 0;
    free(s_inquiry_responses.responses);
    s_inquiry_responses.responses =
        malloc(sizeof(BteHciInquiryResponse) * reply->num_responses);
    bool found = false;
    for (int i = 0; i < reply->num_responses; i++) {
        const BteHciInquiryResponse *r = &reply->responses[i];

        const uint8_t *b = r->address.bytes;
        printf(" - %02x:%02x:%02x:%02x:%02x:%02x\n", b[0], b[1], b[2], b[3],
               b[4], b[5]);

        if (address_is_new(&r->address)) {
            int j = s_inquiry_responses.num_responses++;
            memcpy(&s_inquiry_responses.responses[j], r, sizeof(*r));
        }

        if (memcmp(&r->address, &s_peerAddress, 6) == 0) {
            found = true;
        }
    }
    static int count = 0;
    if (found || count++ > 5) {
        bte_hci_exit_periodic_inquiry(hci, NULL, NULL);

        if (found) {
            bool ok = bte_obex_discover(bte_hci_get_client(hci),
                                        &s_peerAddress, NULL,
                                        obex_discover_cb, NULL);
            printf("bte_obex_discover OK=%d\n", ok);
        }
    }
}

static void initialized_cb(BteHci *hci, bool success, void *)
{
    printf("Initialized, OK = %d\n", success);
    printf("ACL MTU=%d, max packets=%d\n",
           bte_hci_get_acl_mtu(hci),
           bte_hci_get_acl_max_packets(hci));
    bte_hci_periodic_inquiry(hci, 4, 5, BTE_LAP_GIAC, 3, 0,
                             inquiry_status_cb, inquiry_cb, NULL);
}

static double time_now()
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return ts.tv_sec + 0.001 * (ts.tv_nsec / 1000000);
}

int main(int argc, char **argv)
{
    quit_requested = false;

    /* Some platforms need to perform some more steps before having the console
     * output setup. */
    terminal_init();

    printf("Initializing...\n");
    BteClient *client = bte_client_new();
    BteHci *hci = bte_hci_get(client);
    bte_hci_on_initialized(hci, initialized_cb, NULL);

    char filename[100];

    const char *cmdfile = "bte-obex.txt";
    FILE *file = fopen(cmdfile, "r");
    if (!file) printf("Cannot open %s\n", cmdfile);
    else {
        char line[80];
        int n = 0;
        while (fgets(line, sizeof(line), file)) {
            if (n == 0) {
                sscanf(line, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
                       &s_peerAddress.bytes[0],
                       &s_peerAddress.bytes[1],
                       &s_peerAddress.bytes[2],
                       &s_peerAddress.bytes[3],
                       &s_peerAddress.bytes[4],
                       &s_peerAddress.bytes[5]);
                print_addr(&s_peerAddress);
            } else if (n == 1) {
                strncpy(filename, line, sizeof(filename));
            }
            n++;
        }
    }
    fclose(file);

    double time_start = 0.0;
    while (!quit_requested) {
#ifdef WITH_OPENOBEX
        if (s_obex) {
            OBEX_Work(s_obex);
            if (s_obex_state == CONNECTED) {
                time_start = time_now();
                send_file(s_obex, filename);
                s_obex_state = SENDING_FILE;
            } else if (s_obex_state == SENT_FILE) {
                double time_end = time_now();
                printf("Sent file in %.3f seconds\n", time_end - time_start);
                disconnect(s_obex);
                s_obex_state = DISCONNECTING;
            } else if (s_obex_state == DISCONNECTED) {
                OBEX_TransportDisconnect(s_obex);
            }
        } else
#endif
        bte_wait_events(1000000);
    }

    bte_client_unref(client);
    return EXIT_SUCCESS;
}
