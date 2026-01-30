#include "bte_cpp.h"

#include "bt-embedded/services/sdp.h"

#include <gtest/gtest.h>
#include <string>

using namespace Bte;

class TestSdpDeFixture: public testing::Test
{
protected:
    TestSdpDeFixture() {
        buffer.clear();
        buffer.resize(100);
        std::fill(buffer.begin(), buffer.end(), 0xee);
    }

    Buffer buffer;
};

TEST_F(TestSdpDeFixture, testWriteBasicNil)
{
    uint32_t size = bte_sdp_de_write(buffer.data(), buffer.size(),
                                     BTE_SDP_DE_TYPE_NIL, -1);
    buffer.resize(size);
    Buffer expected = {0x00};
    EXPECT_EQ(buffer, expected);
}

TEST_F(TestSdpDeFixture, testWriteBasicUint8)
{
    uint32_t size = bte_sdp_de_write(buffer.data(), buffer.size(),
                                     BTE_SDP_DE_TYPE_UINT8, 180,
                                     -1);
    buffer.resize(size);
    Buffer expected = {0x08, 180};
    EXPECT_EQ(buffer, expected);
}

TEST_F(TestSdpDeFixture, testWriteBasicInt64)
{
    uint32_t size = bte_sdp_de_write(buffer.data(), buffer.size(),
                                     BTE_SDP_DE_TYPE_INT64, 0x1122334455667788,
                                     -1);
    buffer.resize(size);
    Buffer expected = {
        0x13, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
    };
    EXPECT_EQ(buffer, expected);
}

TEST_F(TestSdpDeFixture, testWriteBasicUuid128)
{
    BteSdpDeUuid128 uuid = {
        0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
        0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff,
    };
    uint32_t size = bte_sdp_de_write(buffer.data(), buffer.size(),
                                     BTE_SDP_DE_TYPE_UUID128, &uuid,
                                     -1);
    buffer.resize(size);
    Buffer expected = {
        0x1c, 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
        0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff,
    };
    EXPECT_EQ(buffer, expected);
}

#ifdef __SIZEOF_INT128__
TEST_F(TestSdpDeFixture, testWriteBasicUInt128)
{
    unsigned __int128 v = 0x0011223344556677;
    v <<= 64;
    v += 0x8899aabbccddeeff;
    uint32_t size = bte_sdp_de_write(buffer.data(), buffer.size(),
                                     BTE_SDP_DE_TYPE_UINT128, &v,
                                     -1);
    buffer.resize(size);
    Buffer expected = {
        0x0c, 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
        0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff,
    };
    EXPECT_EQ(buffer, expected);
}
#endif

TEST_F(TestSdpDeFixture, testWriteString)
{
    uint32_t size = bte_sdp_de_write(buffer.data(), buffer.size(),
                                     BTE_SDP_DE_TYPE_STRING, "Hello", -1);
    buffer.resize(size);
    Buffer expected = {0x25, 5, 'H', 'e', 'l', 'l', 'o'};
    EXPECT_EQ(buffer, expected);
}

TEST_F(TestSdpDeFixture, testWriteSequence)
{
    uint32_t size = bte_sdp_de_write(buffer.data(), buffer.size(),
                                     BTE_SDP_DE_TYPE_SEQUENCE,
                                     BTE_SDP_DE_TYPE_URL, "file:a.log",
                                     BTE_SDP_DE_TYPE_INT64, 0x1122334455667788,
                                     BTE_SDP_DE_TYPE_BOOL, true,
                                     -1,
                                     BTE_SDP_DE_TYPE_INT8, -3,
                                     -1);
    buffer.resize(size);
    Buffer expected = {
        0x35, 23,
        0x45, 10, 'f', 'i', 'l', 'e', ':', 'a', '.', 'l', 'o', 'g',
        0x13, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
        0x28, 1,
        0x10, 0xfd,
    };
    EXPECT_EQ(buffer, expected);
}

TEST_F(TestSdpDeFixture, testWriteSequenceNested)
{
    uint32_t size = bte_sdp_de_write(buffer.data(), buffer.size(),
        BTE_SDP_DE_TYPE_SEQUENCE,
            BTE_SDP_DE_TYPE_BOOL, false,
            BTE_SDP_DE_TYPE_SEQUENCE,
                BTE_SDP_DE_TYPE_UINT16, 0x1234,
                BTE_SDP_DE_TYPE_INT16, -200,
            BTE_SDP_DE_END,
            BTE_SDP_DE_TYPE_SEQUENCE,
                BTE_SDP_DE_TYPE_UUID32, 0x11223344,
                BTE_SDP_DE_TYPE_UUID16, 0x5566,
                BTE_SDP_DE_TYPE_CHOICE,
                    BTE_SDP_DE_TYPE_STRING, "yes",
                    BTE_SDP_DE_TYPE_STRING, "no",
                BTE_SDP_DE_END,
                BTE_SDP_DE_TYPE_INT8, -1,
            BTE_SDP_DE_END,
        BTE_SDP_DE_END,
        BTE_SDP_DE_TYPE_UINT8, 42,
        BTE_SDP_DE_END);
    buffer.resize(size);
    Buffer expected = {
        0x35, 33,
            0x28, 0,
            0x35, 6,
                0x09, 0x12, 0x34,
                0x11, 0xff, 0x38,
            0x35, 21,
                0x1a, 0x11, 0x22, 0x33, 0x44,
                0x19, 0x55, 0x66,
                0x3d, 9,
                    0x25, 3, 'y', 'e', 's',
                    0x25, 2, 'n', 'o',
                0x10, 0xff,
        0x08, 42,
    };
    EXPECT_EQ(buffer, expected);
}

TEST_F(TestSdpDeFixture, testWriteLarge)
{
    buffer.resize(100010);
    Buffer tenChars = {'H', 'e', 'l', 'l', 'o', 'W', 'o', 'r', 'l', 'd'};
    Buffer longString;
    longString.reserve(100000);
    for (int i = 0; i < 4999; i++) {
        longString += tenChars;
    }
    longString.push_back(0);
    uint32_t size = bte_sdp_de_write(buffer.data(), buffer.size(),
                                     BTE_SDP_DE_TYPE_SEQUENCE,
                                     BTE_SDP_DE_TYPE_STRING, longString.data(),
                                     BTE_SDP_DE_TYPE_BOOL, true,
                                     BTE_SDP_DE_TYPE_STRING, longString.data(),
                                     -1,
                                     -1);
    buffer.resize(size);
    Buffer expected = Buffer {
        0x37, 0x00, 0x01, 0x86, 0x94,
        0x26, 0xc3, 0x46, };
    Buffer mid {
        0x28, 1, /* Boolean */
        0x26, 0xc3, 0x46, /* Header for second string */
    };
    expected.reserve(100010);
    expected.insert(expected.end(), longString.begin(), longString.end() - 1);
    expected.insert(expected.end(), mid.begin(), mid.end());
    expected.insert(expected.end(), longString.begin(), longString.end() - 1);
    EXPECT_EQ(buffer, expected);
}

TEST_F(TestSdpDeFixture, testWriteSequenceArrayInt8)
{
    int8_t numbers[] = { 10, 20, 30, 40, 50, 60, 70, 80, 90, 100 };
    uint32_t size = bte_sdp_de_write(buffer.data(), buffer.size(),
                                     BTE_SDP_DE_TYPE_SEQUENCE,
                                     BTE_SDP_DE_ARRAY(10),
                                     BTE_SDP_DE_TYPE_INT8, numbers,
                                     -1, -1);
    buffer.resize(size);
    Buffer expected = {
        0x35, 20,
        0x10, 10, 0x10, 20, 0x10, 30, 0x10, 40, 0x10, 50,
        0x10, 60, 0x10, 70, 0x10, 80, 0x10, 90, 0x10, 100,
    };
    EXPECT_EQ(buffer, expected);
}

TEST_F(TestSdpDeFixture, testWriteSequenceArrayUint32)
{
    uint32_t numbers[] = { 0x11223344, 0x55667788, 0x99001122 };
    uint32_t size = bte_sdp_de_write(buffer.data(), buffer.size(),
                                     BTE_SDP_DE_TYPE_SEQUENCE,
                                     BTE_SDP_DE_ARRAY(3),
                                     BTE_SDP_DE_TYPE_UINT32, numbers,
                                     -1, -1);
    buffer.resize(size);
    Buffer expected = {
        0x35, 15,
        0x0a, 0x11, 0x22, 0x33, 0x44,
        0x0a, 0x55, 0x66, 0x77, 0x88,
        0x0a, 0x99, 0x00, 0x11, 0x22,
    };
    EXPECT_EQ(buffer, expected);
}

TEST_F(TestSdpDeFixture, testWriteSequenceArrayStrings)
{
    const char *strings[] = { "Hi", "From", "Me" };
    uint32_t size = bte_sdp_de_write(buffer.data(), buffer.size(),
                                     BTE_SDP_DE_TYPE_SEQUENCE,
                                     BTE_SDP_DE_ARRAY(3),
                                     BTE_SDP_DE_TYPE_STRING, strings,
                                     -1, -1);
    buffer.resize(size);
    Buffer expected = {
        0x35, 14,
        0x25, 2, 'H', 'i',
        0x25, 4, 'F', 'r', 'o', 'm',
        0x25, 2, 'M', 'e',
    };
    EXPECT_EQ(buffer, expected);
}
