#include "l2cap_fixtures.h"
#include "dummy_driver.h"
#include "mock_backend.h"
#include "type_utils.h"

#include "bt-embedded/client.h"
#include "bt-embedded/bte.h"
#include "bt-embedded/l2cap_proto.h"
#include <gtest/gtest.h>
#include <iostream>
#include <memory>

class TestL2capData: public TestL2capFixtureConfigured
{
};

TEST_F(TestL2capData, testSendShort) {
    Buffer data = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
    int rc = m_l2cap->sendMessage(data);
    ASSERT_EQ(rc, 1);

    /* Verify that our request is as expected */
    BufferList expectedData = makeData(data, m_remoteCid, 200);
    ASSERT_EQ(m_backend.sentData(), expectedData);
}

TEST_F(TestL2capData, testSendFragmented) {
    dummy_driver_set_acl_limits(16, 2);
    Buffer data = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
    int rc = m_l2cap->sendMessage(data);
    ASSERT_EQ(rc, 1);

    /* Verify that our request is as expected */
    BufferList expectedData = makeData(data, m_remoteCid, 16);
    ASSERT_EQ(m_backend.sentData(), expectedData);
}

TEST_F(TestL2capData, testSendQueue) {
    dummy_driver_set_acl_limits(16, 1);
    Buffer data = {
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
        17, 18, 19, 20, 21, 22, 23, 24, 25,
    };
    int rc = m_l2cap->sendMessage(data);
    ASSERT_EQ(rc, 0);

    /* Verify that our request is as expected */
    BufferList sentData = makeData(data, m_remoteCid, 16);
    std::vector<Buffer> expectedData {
        sentData[0],
    };
    ASSERT_EQ(m_backend.sentData(), expectedData);

    /* Now signal that a packet has been completed */
    Buffer packetDone {HCI_NBR_OF_COMPLETED_PACKETS, 1 + 4 * 1, 1,
        low(m_connHandle), high(m_connHandle), 1, 0};
    m_backend.sendEvent(packetDone);
    bte_handle_events();

    expectedData = {
        sentData[0],
        sentData[1],
    };
    ASSERT_EQ(m_backend.sentData(), expectedData);

    m_backend.sendEvent(packetDone);
    bte_handle_events();

    ASSERT_EQ(m_backend.sentData(), sentData);
}

TEST_F(TestL2capData, testReceiveShort) {
    std::vector<Buffer> receivedData;
    m_l2cap->onMessageReceived([&](BufferList::Reader &reader) {
        receivedData.push_back(reader.readAll());
    });

    /* Send some data */
    Buffer data = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
    sendData(data);
    bte_handle_events();

    /* Verify we got the same data */
    std::vector<Buffer> expectedData {
        data,
    };
    ASSERT_EQ(receivedData, expectedData);
}

TEST_F(TestL2capData, testReceiveFragmented) {
    std::vector<Buffer> receivedData;
    m_l2cap->onMessageReceived([&](BufferList::Reader &reader) {
        receivedData.push_back(reader.readAll());
    });

    /* Send some data */
    Buffer data = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
    sendData(data, 16);
    bte_handle_events();

    /* Verify we got the same data */
    std::vector<Buffer> expectedData {
        data,
    };
    ASSERT_EQ(receivedData, expectedData);
}

