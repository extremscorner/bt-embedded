#include "bte_sdp_cpp.h"
#include "l2cap_fixtures.h"
#include "dummy_driver.h"
#include "mock_backend.h"
#include "type_utils.h"

#include "bt-embedded/bte.h"
#include <gtest/gtest.h>
#include <iostream>
#include <memory>

class TestSdpClient: public TestL2capFixtureConfigured
{
protected:
    using ServiceSearchReply = Bte::SdpClient::ServiceSearchReply;

    void SetUp() override {
        TestL2capFixtureConfigured::SetUp();
        bte_sdp_reset();
        m_sdp.reset(new Bte::SdpClient(*m_l2cap));
    }

    Buffer makeSdpMsg(BteL2capChannelId destCid,
                      uint16_t reqId, uint8_t pduId, const Buffer &params) {
        uint16_t size = params.size();
        return Buffer{
            low(m_connHandle), uint8_t(0x20 | high(m_connHandle)),
            low(9 + size), high(9 + size), /* Total length */
            low(5 + size), high(5 + size), /* L2CAP length */
            low(destCid), high(destCid),
            pduId,
            high(reqId), low(reqId),
            high(size), low(size),
        } + params;
    }

    Buffer makeServiceSearchReq(uint16_t reqId, const Buffer &pattern,
                                uint16_t maxCount,
                                const Buffer &contState = { 0 }) {
        Buffer params = pattern + Buffer{ high(maxCount), low(maxCount) } + contState;
        return makeSdpMsg(m_remoteCid, reqId, 0x02, params);
    }

    Buffer makeServiceSearchRsp(uint8_t reqId, uint16_t totalCount,
                                const std::span<uint32_t> &handles,
                                const Buffer &contState = { 0 }) {
        uint16_t numHandles = handles.size();
        Buffer handlesBuf;
        for (uint32_t handle: handles) {
            handlesBuf += Buffer{
                byte(3, handle),
                byte(2, handle),
                byte(1, handle),
                byte(0, handle),
            };
        }
        Buffer params = Buffer{
            high(totalCount), low(totalCount),
            high(numHandles), low(numHandles),
        } + handlesBuf + contState;
        return makeSdpMsg(m_localCid, reqId, 0x03, params);
    }

    void sendServiceSearchRsp(uint8_t reqId, uint16_t totalCount,
                              const std::span<uint32_t> &handles,
                              const Buffer &contState = { 0 }) {
        m_backend.sendData(makeServiceSearchRsp(reqId, totalCount,
                                                handles, contState));
    }

    std::unique_ptr<Bte::SdpClient> m_sdp;
    uint16_t m_reqId = 0;
};

TEST_F(TestSdpClient, testServiceSearchSimple) {
    std::vector<uint16_t> pattern = { 0x1122, 0x3344, 0x5566 };
    std::vector<ServiceSearchReply> replies;
    bool ok = m_sdp->serviceSearchReq(
        pattern, 4, [&](const ServiceSearchReply &reply) {
        replies.push_back(reply);
        return false;
    });
    ASSERT_TRUE(ok);

    uint8_t reqId = m_reqId++;

    /* Verify that our request is as expected */
    Buffer patternDe = {
        0x35, 9,
        0x19, 0x11, 0x22,
        0x19, 0x33, 0x44,
        0x19, 0x55, 0x66,
    };
    std::vector<Buffer> expectedData = {
        makeServiceSearchReq(reqId, patternDe, 4),
    };
    ASSERT_EQ(m_backend.sentData(), expectedData);

    /* Send a reply */
    std::vector<uint32_t> handles = { 0x11223344, 0x55667788 };
    sendServiceSearchRsp(reqId, handles.size(), handles);
    bte_handle_events();

    std::vector<ServiceSearchReply> expectedReplies = {
        ServiceSearchReply {
            0, 2, false, handles
        },
    };
    ASSERT_EQ(replies, expectedReplies);
}

TEST_F(TestSdpClient, testServiceSearchFragmented) {
    std::vector<uint16_t> pattern = { 0x1122 };
    std::vector<ServiceSearchReply> replies;
    bool ok = m_sdp->serviceSearchReq(
        pattern, 4, [&](const ServiceSearchReply &reply) {
        replies.push_back(reply);
        return true;
    });
    ASSERT_TRUE(ok);

    uint8_t reqId = m_reqId++;

    /* Verify that our request is as expected */
    Buffer patternDe = {
        0x35, 3,
        0x19, 0x11, 0x22,
    };
    std::vector<Buffer> expectedData = {
        makeServiceSearchReq(reqId, patternDe, 4),
    };
    ASSERT_EQ(m_backend.sentData(), expectedData);
    m_backend.clear();

    /* Send a partial reply */
    std::vector<uint32_t> handles = { 0x11223344, 0x55667788 };
    Buffer contState { 2, 0xaa, 0xbb };
    sendServiceSearchRsp(reqId, 4, handles, contState);
    bte_handle_events();

    /* Verify that the callback has been called */
    std::vector<ServiceSearchReply> expectedReplies = {
        ServiceSearchReply { 0, 4, true, handles },
    };
    ASSERT_EQ(replies, expectedReplies);

    /* And that we have submitted a second request with the continuation
     * state */
    reqId = m_reqId++;
    expectedData = {
        makeServiceSearchReq(reqId, patternDe, 4, contState),
    };
    ASSERT_EQ(m_backend.sentData(), expectedData);
    m_backend.clear();
    replies.clear();

    /* Send the second part of the reply */
    handles = { 0xaabbccdd, 0xeeff0011 };
    sendServiceSearchRsp(reqId, 4, handles);
    bte_handle_events();

    /* Verify that the callback has been called */
    expectedReplies = {
        ServiceSearchReply { 0, 4, false, handles },
    };
    ASSERT_EQ(replies, expectedReplies);

    ASSERT_TRUE(m_backend.sentData().empty());
}
