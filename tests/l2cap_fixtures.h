#ifndef BTE_TESTS_L2CAP_FIXTURES_H
#define BTE_TESTS_L2CAP_FIXTURES_H

#include "bte_l2cap_cpp.h"
#include "dummy_driver.h"
#include "mock_backend.h"
#include "type_utils.h"

#include "bt-embedded/client.h"
#include "bt-embedded/bte.h"
#include "bt-embedded/l2cap_proto.h"
#include <gtest/gtest.h>
#include <memory>

class TestL2capFixture: public testing::Test
{
protected:
    TestL2capFixture() {
        bte_l2cap_reset();
    }

    void SetUp() override {
        m_backend.clear();
        m_cmdId = 1;
        m_localCid = 0x0040;
        m_remoteCid = 0x004f;
        dummy_driver_set_acl_limits(600, 2);
    }

    void TearDown() override {
        m_backend.onSendData({});
        while (!m_connections.empty()) {
            sendHciDisconnectionComplete(*m_connections.begin());
        }
        bte_handle_events();
    }

    static uint8_t low(uint16_t v) {
        return v & 0xff;
    }

    static uint8_t high(uint16_t v) {
        return v >> 8;
    }

    Buffer makeConnectResponse(
        uint8_t reqId, uint16_t result = BTE_L2CAP_CONN_RESP_RES_OK,
        uint16_t status = BTE_L2CAP_CONN_RESP_STATUS_NO_INFO) {
        return Buffer{
            low(m_connHandle), uint8_t(0x20 | high(m_connHandle)),
            16, 0, /* Total length */
            12, 0, /* L2CAP length */
            0x01, 0x00, /* Signalling channel */
            L2CAP_SIGNAL_CONN_RSP,
            reqId,
            8, 0, /* command length */
            low(m_remoteCid), high(m_remoteCid), /* dest CID */
            low(m_localCid), high(m_localCid), /* source CID */
            low(result), high(result),
            low(status), high(status),
        };
    }

    void sendConnectResponse(uint8_t reqId) {
        m_backend.sendData(makeConnectResponse(reqId));
    }

    Buffer makeHciCreateDisconnection(const BteBdAddr &address,
                                   BtePacketType packetType,
                                   uint8_t pageScanRepMode,
                                   uint16_t clockOffset,
                                   bool allowRoleSwitch) {
        uint8_t size = 13;
        const uint8_t *b = address.bytes;
        return {
            0x5, 0x4, size, b[0], b[1], b[2], b[3], b[4], b[5],
            low(packetType), high(packetType), pageScanRepMode,
            0, low(clockOffset), high(clockOffset), allowRoleSwitch
        };
    }

    void sendHciDisconnectionComplete(const BteBdAddr &address) {
        const uint8_t eventSize = 1 + 2 + 6 + 1 + 1;
        uint8_t link_type = 1; /* ACL */
        uint8_t enc_mode = 0;
        uint8_t status = 0;
        m_backend.sendEvent(
            Buffer{HCI_CONNECTION_COMPLETE, eventSize, status, 0x00, 0x01} +
            address + Buffer{link_type, enc_mode});
        m_connections.insert(0x0100);
    }

    void sendHciDisconnectionComplete(BteConnHandle handle,
                                      uint8_t reason = HCI_HOST_TIMEOUT)
    {
        const uint8_t eventSize = 1 + 2 + 1;
        uint8_t status = 0;
        m_backend.sendEvent({ HCI_DISCONNECTION_COMPLETE, eventSize, status,
                              low(handle), high(handle), reason });
        m_connections.erase(handle);
    }

    Buffer makeDisconnectResponse(uint8_t reqId,
                                  BteL2capChannelId destCid,
                                  BteL2capChannelId sourceCid) {
        return Buffer{
            low(m_connHandle), uint8_t(0x20 | high(m_connHandle)),
            12, 0, /* Total length */
            8, 0, /* L2CAP length */
            0x01, 0x00, /* Signalling channel */
            L2CAP_SIGNAL_DISCONN_RSP,
            reqId,
            4, 0, /* command length */
            low(destCid), high(destCid),
            low(sourceCid), high(sourceCid),
        };
    }

    void sendDisconnectResponse(uint8_t reqId) {
        m_backend.sendData(makeDisconnectResponse(reqId,
                                                  m_remoteCid, m_localCid));
    }

    Buffer makeDisconnectRequest(uint8_t reqId,
                                 BteL2capChannelId destCid,
                                 BteL2capChannelId sourceCid) {
        return Buffer{
            low(m_connHandle), uint8_t(0x20 | high(m_connHandle)),
            12, 0, /* Total length */
            8, 0, /* L2CAP length */
            0x01, 0x00, /* Signalling channel */
            L2CAP_SIGNAL_DISCONN_REQ,
            reqId,
            4, 0, /* command length */
            low(destCid), high(destCid),
            low(sourceCid), high(sourceCid),
        };
    }

    void sendDisconnectRequest(uint8_t reqId) {
        m_backend.sendData(makeDisconnectRequest(reqId,
                                                 m_localCid, m_remoteCid));
    }

    MockBackend m_backend;
    Bte::Client m_client;
    BteL2capChannelId m_localCid;
    BteL2capChannelId m_remoteCid;
    BteConnHandle m_connHandle;
    uint8_t m_cmdId;
    std::set<BteConnHandle> m_connections;
};

class TestL2capFixtureConnected: public TestL2capFixture
{
protected:
    void SetUp() override {
        TestL2capFixture::SetUp();
        BteBdAddr address = {1, 2, 3, 4, 5, 6};
        auto onConnected = [this](std::optional<Bte::L2cap> l2cap,
                                  const BteL2capConnectionResponse &reply) {
            ASSERT_TRUE(l2cap.has_value());
            m_l2cap.reset(new Bte::L2cap(l2cap.value()));
        };
        Bte::L2cap::newOutgoing(m_client, address, BTE_L2CAP_PSM_SDP,
                                {}, onConnected);
        /* Send the statue reply for HCI create connection */
        uint8_t status = 0;
        m_backend.sendEvent({HCI_COMMAND_STATUS, 4, status, 1, 0x5, 0x4});
        bte_handle_events();
        /* Send the actual reply */
        const uint8_t eventSize = 1 + 2 + 6 + 1 + 1;
        uint8_t link_type = 1;
        uint8_t enc_mode = 0;
        m_backend.sendEvent(
            Buffer{HCI_CONNECTION_COMPLETE, eventSize, status, 0x00, 0x01} +
            address + Buffer{link_type, enc_mode});
        bte_handle_events();
        m_connHandle = 0x0100;
        m_connections.insert(m_connHandle);

        sendConnectResponse(m_cmdId++);
        bte_handle_events();
        m_backend.clear();
    }

    void TearDown() override {
        TestL2capFixture::TearDown();
    }

protected:
    std::unique_ptr<Bte::L2cap> m_l2cap;
};

#endif // BTE_TESTS_L2CAP_FIXTURES_H
