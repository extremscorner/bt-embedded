#include "l2cap_fixtures.h"

#include "bt-embedded/services/obex.h"

#include <gtest/gtest.h>
#include <memory>

class TestObexDiscovery: public TestL2capFixture
{
protected:
    void SetUp() override {
        TestL2capFixture::SetUp();
        m_connHandle = 0x0100;
        m_remoteCid = 0x0040;
    }

    Buffer makeSdpMsg(BteL2capChannelId destCid,
                      uint16_t reqId, uint8_t pduId, const Buffer &params) {
        uint16_t size = params.size();
        return makeL2capMessage(m_connHandle, destCid, Buffer{
            pduId,
            high(reqId), low(reqId),
            high(size), low(size),
        } + params);
    }

    Buffer makeServiceSearchAttrReq(uint16_t reqId, const Buffer &pattern,
                                    uint16_t maxCount, const Buffer &idList,
                                    const Buffer &contState = { 0 }) {
        Buffer params = pattern + Buffer{ high(maxCount), low(maxCount) } +
            idList + contState;
        return makeSdpMsg(m_remoteCid, reqId, 0x06, params);
    }

    Buffer makeServiceSearchAttrRsp(uint8_t reqId, uint16_t size,
                                    const Buffer &attrList,
                                    const Buffer &contState = { 0 }) {
        Buffer params = Buffer{
            high(size), low(size),
        } + attrList + contState;
        return makeSdpMsg(m_localCid, reqId, 0x07, params);
    }

    void sendServiceSearchAttrRsp(uint8_t reqId, uint16_t size,
                                  const Buffer &attrList,
                                  const Buffer &contState = { 0 }) {
        m_backend.sendData(makeServiceSearchAttrRsp(reqId, size, attrList,
                                                    contState));
    }
    void peerConnects(const BteBdAddr &address) {
        /* Send the status reply for HCI create connection */
        uint8_t status = 0;
        m_backend.sendEvent({HCI_COMMAND_STATUS, 4, status, 1, 0x5, 0x4});
        bte_handle_events();

        /* Send the actual reply */
        sendHciConnectionComplete(address, m_connHandle);
        bte_handle_events();

        /* Send the L2cap connect response */
        uint8_t reqId = m_cmdId++;
        sendConnectResponse(reqId);
        bte_handle_events();

        /* Send an empty configure reply */
        reqId = m_cmdId++;
        sendConfigResponse(Buffer(), reqId);
        bte_handle_events();

        m_backend.clear();

        /* And send the peer's configuration request */
        sendConfigRequest({}, 51);
        bte_handle_events();
    }

    uint16_t m_sdpReqId = 0;
};

TEST_F(TestObexDiscovery, testDefaults) {
    struct Callbacks {
        static void discover_cb(
            BteClient *client, const BteObexDiscoverReply *r, void *userdata) {
            Callbacks *data = (Callbacks*)userdata;
            data->reply = *r;
            data->numInvocations++;
        }

        BteObexDiscoverReply reply = { 0, };
        int numInvocations = 0;
    } cb_data;

    BteBdAddr address = {1, 2, 3, 4, 5, 6};
    bte_obex_discover(m_client.c_type(), &address, NULL,
                      &Callbacks::discover_cb, &cb_data);

    peerConnects(address);

    uint8_t reqId = m_sdpReqId++;
    Buffer pattern {
        0x35, 3,
        0x19, 0x11, 0x05,
    };
    Buffer idList {
        0x35, 6,
        0x09, 0x00, 0x04, 0x09, 0x02, 0x00,
    };
    Buffer expectedData = makeServiceSearchAttrReq(reqId, pattern, 0x1000,
                                                   idList);
    ASSERT_EQ(m_backend.lastData(), expectedData);

    /* Send our reply (copied from a wireshack dump) */
    Buffer attrList = {
        0x35, 0x66, 0x35, 0x64, 0x09, 0x00, 0x00, 0x0a,
        0x00, 0x01, 0x00, 0x0f, 0x09, 0x00, 0x01, 0x35,
        0x03, 0x19, 0x11, 0x05, 0x09, 0x00, 0x04, 0x35,
        0x11, 0x35, 0x03, 0x19, 0x01, 0x00, 0x35, 0x05,
        0x19, 0x00, 0x03, 0x08, 0x09, 0x35, 0x03, 0x19,
        0x00, 0x08, 0x09, 0x00, 0x05, 0x35, 0x03, 0x19,
        0x10, 0x02, 0x09, 0x00, 0x09, 0x35, 0x08, 0x35,
        0x06, 0x19, 0x11, 0x05, 0x09, 0x01, 0x02, 0x09,
        0x01, 0x00, 0x25, 0x0b, 0x4f, 0x62, 0x6a, 0x65,
        0x63, 0x74, 0x20, 0x50, 0x75, 0x73, 0x68, 0x09,
        0x02, 0x00, 0x09, 0x10, 0x09, 0x09, 0x03, 0x03,
        0x35, 0x0e, 0x08, 0x01, 0x08, 0x02, 0x08, 0x03,
        0x08, 0x04, 0x08, 0x05, 0x08, 0x06, 0x08, 0xff,
    };
    sendServiceSearchAttrRsp(reqId, attrList.size(), attrList);
    bte_handle_events();

    EXPECT_EQ(cb_data.numInvocations, 1);
    EXPECT_EQ(cb_data.reply.opp_l2cap_psm, 0x1009);
    EXPECT_EQ(cb_data.reply.opp_rfcomm_channel, 9);
}
