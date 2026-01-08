#include "bte_l2cap_cpp.h"
#include "dummy_driver.h"
#include "mock_backend.h"
#include "type_utils.h"

#include <gtest/gtest.h>
#include <iostream>
#include <memory>
#include <set>

#include "bt-embedded/bte.h"
#include "bt-embedded/client.h"
#include "bt-embedded/l2cap_proto.h"

class TestL2capServer: public testing::Test
{
protected:
    TestL2capServer()
    {
    }

    void SetUp() override
    {
        m_backend.clear();
        dummy_driver_set_acl_limits(600, 2);
        setLocalCid(0x0040);
        setRemoteCid(0x0040);
        m_cmdId = 1;
    }

    void TearDown() override
    {
        m_backend.onSendData({});
        while (!m_connections.empty()) {
            sendHciDisconnectionComplete(*m_connections.begin());
        }
        bte_handle_events();
    }

    void setLocalCid(BteL2capChannelId cid)
    {
        m_localCid = cid;
    }

    void setRemoteCid(BteL2capChannelId cid)
    {
        m_remoteCid = cid;
    }

    static uint8_t low(uint16_t v)
    {
        return v & 0xff;
    }

    static uint8_t high(uint16_t v)
    {
        return v >> 8;
    }

    Buffer makeConnectionResponse(
        uint8_t reqId, uint16_t result = BTE_L2CAP_CONN_RESP_RES_OK,
        uint16_t status = BTE_L2CAP_CONN_RESP_STATUS_NO_INFO)
    {
        return Buffer{
            low(m_connHandle), uint8_t(0x20 | high(m_connHandle)),
            16, 0, /* Total length */
            12, 0, /* L2CAP length */
            0x01, 0x00, /* Signalling channel */
            L2CAP_SIGNAL_CONN_RSP,
            reqId,
            8, 0, /* command length */
            low(m_localCid), high(m_localCid), /* source CID */
            low(m_remoteCid), high(m_remoteCid), /* dest CID */
            low(result), high(result),
            low(status), high(status),
        };
    }

    Buffer makeConnectionRequest(uint8_t reqId, BteL2capPsm psm)
    {
        return Buffer{
            low(m_connHandle), uint8_t(0x20 | high(m_connHandle)),
            12, 0, /* Total length */
            8, 0, /* L2CAP length */
            0x01, 0x00, /* signalling channel */
            L2CAP_SIGNAL_CONN_REQ,
            reqId,
            4, 0, /* cmd length */
            low(psm), high(psm),
            low(m_localCid), high(m_localCid), /* source CID */
        };
    }

    void sendConnectionRequest(uint8_t reqId, BteL2capPsm psm)
    {
        m_backend.sendData(makeConnectionRequest(reqId, psm));
    }

    Buffer makeHciCreateConnection(const BteBdAddr &address,
                                   BtePacketType packetType,
                                   uint8_t pageScanRepMode,
                                   uint16_t clockOffset, bool allowRoleSwitch)
    {
        uint8_t size = 13;
        const uint8_t *b = address.bytes;
        return { 0x5, 0x4, size,
                 b[0], b[1], b[2], b[3], b[4], b[5],
                 low(packetType), high(packetType),
                 pageScanRepMode,
                 0,
                 low(clockOffset), high(clockOffset),
                 allowRoleSwitch };
    }

    void sendHciConnectionComplete(BteConnHandle handle,
                                   const BteBdAddr &address)
    {
        const uint8_t eventSize = 1 + 2 + 6 + 1 + 1;
        uint8_t link_type = 1; /* ACL */
        uint8_t enc_mode = 0;
        uint8_t status = 0;
        m_backend.sendEvent(Buffer{ HCI_CONNECTION_COMPLETE, eventSize, status,
                                    low(handle), high(handle) } +
                            address + Buffer{ link_type, enc_mode });
        m_connections.insert(handle);
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

    void sendHciConnectionReq(const BteBdAddr &address,
                              const BteClassOfDevice &cod,
                              uint8_t link_type = BTE_LINK_TYPE_ACL)
    {
        m_backend.sendEvent(
            Buffer{ HCI_CONNECTION_REQUEST, 6 + 3 + 1 } + address + cod +
            Buffer{link_type});
    }

    MockBackend m_backend;
    Bte::Client m_client;
    BteConnHandle m_connHandle;
    BteL2capChannelId m_localCid;
    BteL2capChannelId m_remoteCid;
    uint8_t m_cmdId;
    std::set<BteConnHandle> m_connections;
};

TEST_F(TestL2capServer, testOneOk)
{
    Bte::L2capServer server(m_client, BTE_L2CAP_PSM_SDP);
    std::vector<Bte::L2cap> receivedConnections;
    server.onConnected(
        [&](Bte::L2cap &l2cap) { receivedConnections.push_back(l2cap); });

    /* Check that the controller is ready to accept connections */
    std::vector<Buffer> expectedCommands{
        Buffer{ 0x1a, 0xc, 1, BTE_HCI_SCAN_ENABLE_PAGE },
    };
    ASSERT_EQ(m_backend.sentCommands(), expectedCommands);
    m_backend.clear();

    /* Send an incoming connection */
    BteBdAddr address = {1, 2, 3, 4, 5, 6};
    BteClassOfDevice cod = {7, 8, 9};
    sendHciConnectionReq(address, cod);
    bte_handle_events();

    /* Verify that the connection was accepted */
    uint8_t role = BTE_HCI_ROLE_SLAVE;
    expectedCommands = {
        Buffer{ 0x9, 0x4, 6 + 1 } + address + Buffer{role},
    };
    ASSERT_EQ(m_backend.sentCommands(), expectedCommands);

    /* And that the connection was not yet reported */
    ASSERT_TRUE(receivedConnections.empty());

    /* Send the connection complete */
    uint8_t hciStatus = 0;
    m_backend.sendEvent({HCI_COMMAND_STATUS, 4, hciStatus, 1, 0x9, 0x4});
    m_connHandle = 0x102;
    sendHciConnectionComplete(m_connHandle, address);
    bte_handle_events();

    /* And that the connection was not yet reported */
    ASSERT_TRUE(receivedConnections.empty());

    /* Send the L2CAP connection request */
    uint8_t incomingReqId = 56;
    sendConnectionRequest(incomingReqId, BTE_L2CAP_PSM_SDP);
    bte_handle_events();

    /* Verify that we accepted the request */
    std::vector<Buffer> expectedData {
        makeConnectionResponse(incomingReqId),
    };
    ASSERT_EQ(m_backend.sentData(), expectedData);

    ASSERT_EQ(receivedConnections.size(), 1);
    Bte::L2cap l2cap = receivedConnections[0];
    ASSERT_EQ(l2cap.state(), BTE_L2CAP_WAIT_CONFIG);
    ASSERT_EQ(l2cap.psm(), BTE_L2CAP_PSM_SDP);
    ASSERT_EQ(l2cap.connectionHandle(), m_connHandle);
}

TEST_F(TestL2capServer, testUnsupportedPsm)
{
    Bte::L2capServer server(m_client, BTE_L2CAP_PSM_SDP);
    std::vector<Bte::L2cap> receivedConnections;
    server.onConnected(
        [&](Bte::L2cap &l2cap) { receivedConnections.push_back(l2cap); });

    /* Send an incoming connection */
    BteBdAddr address = {1, 2, 3, 4, 5, 6};
    sendHciConnectionReq(address, {7, 8, 9});
    bte_handle_events();

    /* Send the connection complete */
    uint8_t hciStatus = 0;
    m_backend.sendEvent({HCI_COMMAND_STATUS, 4, hciStatus, 1, 0x9, 0x4});
    m_connHandle = 0x102;
    sendHciConnectionComplete(m_connHandle, address);
    bte_handle_events();

    /* Send the L2CAP connection request */
    uint8_t incomingReqId = 56;
    sendConnectionRequest(incomingReqId, BTE_L2CAP_PSM_HID_CTRL);
    bte_handle_events();

    /* Verify that we refused the request */
    m_localCid = BTE_L2CAP_CHANNEL_ID_NULL;
    std::vector<Buffer> expectedData {
        makeConnectionResponse(incomingReqId, BTE_L2CAP_CONN_RESP_RES_ERR_PSM),
    };
    ASSERT_EQ(m_backend.sentData(), expectedData);

    ASSERT_TRUE(receivedConnections.empty());
}
