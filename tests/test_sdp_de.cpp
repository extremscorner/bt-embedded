#include "bte_cpp.h"
#include "bte_sdp_cpp.h"

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
                                     BTE_SDP_DE_TYPE_NIL);
    buffer.resize(size);
    Buffer expected = {0x00};
    EXPECT_EQ(buffer, expected);
}

TEST_F(TestSdpDeFixture, testWriteBasicUint8)
{
    uint32_t size = bte_sdp_de_write(buffer.data(), buffer.size(),
                                     BTE_SDP_DE_TYPE_UINT8, 180);
    buffer.resize(size);
    Buffer expected = {0x08, 180};
    EXPECT_EQ(buffer, expected);
}

TEST_F(TestSdpDeFixture, testWriteBasicInt64)
{
    uint32_t size = bte_sdp_de_write(buffer.data(), buffer.size(),
                                     BTE_SDP_DE_TYPE_INT64, 0x1122334455667788);
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
                                     BTE_SDP_DE_TYPE_UUID128, uuid);
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
                                     BTE_SDP_DE_TYPE_UINT128, v);
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
                                     BTE_SDP_DE_TYPE_STRING, "Hello");
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
                                     BTE_SDP_DE_END);
    buffer.resize(size);
    Buffer expected = {
        0x35, 23,
        0x45, 10, 'f', 'i', 'l', 'e', ':', 'a', '.', 'l', 'o', 'g',
        0x13, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
        0x28, 1,
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
                                     BTE_SDP_DE_END);
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
                                     BTE_SDP_DE_END);
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
                                     BTE_SDP_DE_END);
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
                                     BTE_SDP_DE_END);
    buffer.resize(size);
    Buffer expected = {
        0x35, 14,
        0x25, 2, 'H', 'i',
        0x25, 4, 'F', 'r', 'o', 'm',
        0x25, 2, 'M', 'e',
    };
    EXPECT_EQ(buffer, expected);
}

TEST_F(TestSdpDeFixture, testRead8)
{
    buffer = {0x10, 0xf1};

    BteSdpDeReader reader;
    bte_sdp_de_reader_init(&reader, buffer.data());
    EXPECT_EQ(bte_sdp_de_reader_get_type(&reader), BTE_SDP_DE_TYPE_INT8);
    EXPECT_EQ(bte_sdp_de_reader_read_uint8(&reader), 0xf1);
    EXPECT_EQ(bte_sdp_de_reader_read_uint16(&reader), 0xf1);
    EXPECT_EQ(bte_sdp_de_reader_read_uuid16(&reader), 0xf1);
    EXPECT_EQ(bte_sdp_de_reader_read_uint32(&reader), 0xf1);
    EXPECT_EQ(bte_sdp_de_reader_read_uint64(&reader), 0xf1);

    EXPECT_EQ(bte_sdp_de_reader_read_int8(&reader), -15);
    EXPECT_EQ(bte_sdp_de_reader_read_int16(&reader), -15);
    EXPECT_EQ(bte_sdp_de_reader_read_int32(&reader), -15);
    EXPECT_EQ(bte_sdp_de_reader_read_int64(&reader), -15);

#ifdef __SIZEOF_INT128__
    EXPECT_EQ(bte_sdp_de_reader_read_uint128(&reader), 0xf1);
    EXPECT_EQ(bte_sdp_de_reader_read_int128(&reader), -15);
#endif
}

TEST_F(TestSdpDeFixture, testRead16)
{
    buffer = {0x09, 0x81, 0x22};

    BteSdpDeReader reader;
    bte_sdp_de_reader_init(&reader, buffer.data());
    EXPECT_EQ(bte_sdp_de_reader_get_type(&reader), BTE_SDP_DE_TYPE_UINT16);
    EXPECT_EQ(bte_sdp_de_reader_read_uint16(&reader), 0x8122);
    EXPECT_EQ(bte_sdp_de_reader_read_uuid16(&reader), 0x8122);
    EXPECT_EQ(bte_sdp_de_reader_read_uint32(&reader), 0x8122);

    EXPECT_EQ(bte_sdp_de_reader_read_int16(&reader), -32478);

    /* But we cannot read a uint8 out of this */
    EXPECT_EQ(bte_sdp_de_reader_read_uint8(&reader), 0);

    EXPECT_FALSE(bte_sdp_de_reader_next(&reader));
}

TEST_F(TestSdpDeFixture, testRead32)
{
    buffer = {0x12, 0x88, 0x77, 0x66, 0x55};

    BteSdpDeReader reader;
    bte_sdp_de_reader_init(&reader, buffer.data());
    EXPECT_EQ(bte_sdp_de_reader_get_type(&reader), BTE_SDP_DE_TYPE_INT32);
    EXPECT_EQ(bte_sdp_de_reader_read_int32(&reader), -2005440939);
    EXPECT_EQ(bte_sdp_de_reader_read_uint32(&reader), 0x88776655);
    EXPECT_EQ(bte_sdp_de_reader_read_uuid32(&reader), 0x88776655);

    /* But we cannot read a uint16 out of this */
    EXPECT_EQ(bte_sdp_de_reader_read_uint16(&reader), 0);
}

TEST_F(TestSdpDeFixture, testRead64)
{
    buffer = {0x0b, 0x88, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11};

    BteSdpDeReader reader;
    bte_sdp_de_reader_init(&reader, buffer.data());
    EXPECT_EQ(bte_sdp_de_reader_get_type(&reader), BTE_SDP_DE_TYPE_UINT64);
    EXPECT_EQ(bte_sdp_de_reader_read_int64(&reader), -8613303245920329199);
    EXPECT_EQ(bte_sdp_de_reader_read_uint64(&reader), 0x8877665544332211);

    /* But we cannot read a smaller type out of this */
    EXPECT_EQ(bte_sdp_de_reader_read_uint8(&reader), 0);
    EXPECT_EQ(bte_sdp_de_reader_read_uint16(&reader), 0);
    EXPECT_EQ(bte_sdp_de_reader_read_uint32(&reader), 0);
    EXPECT_EQ(bte_sdp_de_reader_read_int8(&reader), 0);
    EXPECT_EQ(bte_sdp_de_reader_read_int16(&reader), 0);
    EXPECT_EQ(bte_sdp_de_reader_read_int32(&reader), 0);

    /* But we cannot read a UUID out of this */
    BteSdpDeUuid128 uuid = { 0, };
    EXPECT_EQ(bte_sdp_de_reader_read_uuid128(&reader), uuid);
}

TEST_F(TestSdpDeFixture, testRead128)
{
    buffer = {
        0x14, 0x88, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11,
        1, 2, 3, 4, 5, 6, 7, 8,
    };

    BteSdpDeReader reader;
    bte_sdp_de_reader_init(&reader, buffer.data());
    EXPECT_EQ(bte_sdp_de_reader_get_type(&reader), BTE_SDP_DE_TYPE_INT128);
    BteSdpInt128 v128;
    BteSdpUint128 v128_u;
    uint64_t low = 0x0102030405060708;
    uint64_t high = 0x8877665544332211;
    uint64_t *parts = (uint64_t*)&v128;
    uint64_t *parts_u = (uint64_t*)&v128_u;
#if __BYTE_ORDER == __LITTLE_ENDIAN
    parts[0] = parts_u[0] = low;
    parts[1] = parts_u[1] = high;
#else
    parts[1] = parts_u[1] = low;
    parts[0] = parts_u[0] = high;
#endif
    EXPECT_EQ(bte_sdp_de_reader_read_uint128(&reader), v128_u);
    EXPECT_EQ(bte_sdp_de_reader_read_int128(&reader), v128);

    /* But we cannot read a smaller type out of this */
    EXPECT_EQ(bte_sdp_de_reader_read_uint8(&reader), 0);
    EXPECT_EQ(bte_sdp_de_reader_read_uint16(&reader), 0);
    EXPECT_EQ(bte_sdp_de_reader_read_uint32(&reader), 0);
    EXPECT_EQ(bte_sdp_de_reader_read_uint64(&reader), 0);
    EXPECT_EQ(bte_sdp_de_reader_read_int8(&reader), 0);
    EXPECT_EQ(bte_sdp_de_reader_read_int16(&reader), 0);
    EXPECT_EQ(bte_sdp_de_reader_read_int32(&reader), 0);
    EXPECT_EQ(bte_sdp_de_reader_read_int64(&reader), 0);
}

TEST_F(TestSdpDeFixture, testReadUuid16)
{
    buffer = { 0x19, 0xaa, 0xbb };

    BteSdpDeReader reader;
    bte_sdp_de_reader_init(&reader, buffer.data());
    EXPECT_EQ(bte_sdp_de_reader_get_type(&reader), BTE_SDP_DE_TYPE_UUID16);
    EXPECT_EQ(bte_sdp_de_reader_read_uuid16(&reader), 0xaabb);
    EXPECT_EQ(bte_sdp_de_reader_read_uuid32(&reader), 0xaabb);

    BteSdpDeUuid128 uuid = {
        0x00, 0x00, 0xaa, 0xbb,
        0x00, 0x00,
        0x10, 0x00,
        0x80, 0x00,
        0x00, 0x80, 0x5f, 0x9b, 0x34, 0xfb
    };
    EXPECT_EQ(bte_sdp_de_reader_read_uuid128(&reader), uuid);
}

TEST_F(TestSdpDeFixture, testReadUuid128)
{
    buffer = {
        0x1c, 0x88, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11,
        1, 2, 3, 4, 5, 6, 7, 8,
    };

    BteSdpDeReader reader;
    bte_sdp_de_reader_init(&reader, buffer.data());
    EXPECT_EQ(bte_sdp_de_reader_get_type(&reader), BTE_SDP_DE_TYPE_UUID128);
    BteSdpDeUuid128 uuid = {
        0x88, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11,
        1, 2, 3, 4, 5, 6, 7, 8,
    };
    EXPECT_EQ(bte_sdp_de_reader_read_uuid128(&reader), uuid);

    /* But we cannot read a smaller type out of this */
    EXPECT_EQ(bte_sdp_de_reader_read_uuid16(&reader), 0);
    EXPECT_EQ(bte_sdp_de_reader_read_uuid32(&reader), 0);
}

TEST_F(TestSdpDeFixture, testReadString)
{
    buffer = {0x45, 11, 'f', 'i', 'l', 'e', ':', 'b', 't', '.', 't', 'x', 't'};

    BteSdpDeReader reader;
    bte_sdp_de_reader_init(&reader, buffer.data());
    EXPECT_EQ(bte_sdp_de_reader_get_type(&reader), BTE_SDP_DE_TYPE_URL);
    char text[20];
    size_t len = bte_sdp_de_reader_copy_str(&reader, text, sizeof(text));
    EXPECT_EQ(len, 11);
    EXPECT_STREQ(text, "file:bt.txt");

    /* Now try reding it into a smaller buffer */
    char small[8];
    len = bte_sdp_de_reader_copy_str(&reader, small, sizeof(small));
    EXPECT_EQ(len, 11);
    EXPECT_STREQ(small, "file:bt");

    /* And see what happens if we try to read a string out of other data */
    buffer = {0x12, 0x88, 0x77, 0x66, 0x55};
    bte_sdp_de_reader_init(&reader, buffer.data());
    EXPECT_EQ(bte_sdp_de_reader_get_type(&reader), BTE_SDP_DE_TYPE_INT32);
    len = bte_sdp_de_reader_copy_str(&reader, text, sizeof(text));
    EXPECT_EQ(len, 0);
    EXPECT_STREQ(text, "");
}

TEST_F(TestSdpDeFixture, testReadSequence)
{
    uint32_t size = bte_sdp_de_write(buffer.data(), buffer.size(),
        BTE_SDP_DE_TYPE_SEQUENCE,
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

    BteSdpDeReader reader;
    bte_sdp_de_reader_init(&reader, buffer.data());
    EXPECT_EQ(bte_sdp_de_reader_get_type(&reader), BTE_SDP_DE_TYPE_SEQUENCE);
    EXPECT_TRUE(bte_sdp_de_reader_enter(&reader));
    EXPECT_TRUE(bte_sdp_de_reader_next(&reader));

    EXPECT_TRUE(bte_sdp_de_reader_enter(&reader));
    EXPECT_TRUE(bte_sdp_de_reader_next(&reader));

    EXPECT_EQ(bte_sdp_de_reader_get_type(&reader), BTE_SDP_DE_TYPE_BOOL);
    EXPECT_EQ(bte_sdp_de_reader_read_bool(&reader), false);
    EXPECT_FALSE(bte_sdp_de_reader_enter(&reader));
    EXPECT_TRUE(bte_sdp_de_reader_next(&reader));

    EXPECT_EQ(bte_sdp_de_reader_get_type(&reader), BTE_SDP_DE_TYPE_SEQUENCE);
    /* We don't enter this one */
    EXPECT_TRUE(bte_sdp_de_reader_next(&reader));

    EXPECT_EQ(bte_sdp_de_reader_get_type(&reader), BTE_SDP_DE_TYPE_SEQUENCE);
    EXPECT_TRUE(bte_sdp_de_reader_enter(&reader));
    EXPECT_TRUE(bte_sdp_de_reader_next(&reader));

    EXPECT_EQ(bte_sdp_de_reader_get_type(&reader), BTE_SDP_DE_TYPE_UUID32);
    EXPECT_EQ(bte_sdp_de_reader_read_uuid32(&reader), 0x11223344);
    EXPECT_TRUE(bte_sdp_de_reader_next(&reader));

    EXPECT_EQ(bte_sdp_de_reader_get_type(&reader), BTE_SDP_DE_TYPE_UUID16);
    EXPECT_EQ(bte_sdp_de_reader_read_uuid16(&reader), 0x5566);
    EXPECT_TRUE(bte_sdp_de_reader_next(&reader));

    EXPECT_EQ(bte_sdp_de_reader_get_type(&reader), BTE_SDP_DE_TYPE_CHOICE);
    EXPECT_TRUE(bte_sdp_de_reader_enter(&reader));
    EXPECT_TRUE(bte_sdp_de_reader_next(&reader));

    EXPECT_EQ(bte_sdp_de_reader_get_type(&reader), BTE_SDP_DE_TYPE_STRING);
    char text[10];
    size_t len = bte_sdp_de_reader_copy_str(&reader, text, sizeof(text));
    EXPECT_EQ(len, 3);
    EXPECT_STREQ(text, "yes");
    EXPECT_TRUE(bte_sdp_de_reader_next(&reader));

    EXPECT_EQ(bte_sdp_de_reader_get_type(&reader), BTE_SDP_DE_TYPE_STRING);
    len = bte_sdp_de_reader_copy_str(&reader, text, sizeof(text));
    EXPECT_EQ(len, 2);
    EXPECT_STREQ(text, "no");
    EXPECT_FALSE(bte_sdp_de_reader_next(&reader));
    EXPECT_TRUE(bte_sdp_de_reader_leave(&reader));
    EXPECT_TRUE(bte_sdp_de_reader_next(&reader));

    EXPECT_EQ(bte_sdp_de_reader_get_type(&reader), BTE_SDP_DE_TYPE_INT8);
    EXPECT_EQ(bte_sdp_de_reader_read_int8(&reader), -1);
    EXPECT_TRUE(bte_sdp_de_reader_leave(&reader));
    /* We return back to the beginning of the sequence */
    EXPECT_EQ(bte_sdp_de_reader_get_type(&reader), BTE_SDP_DE_TYPE_SEQUENCE);

    EXPECT_FALSE(bte_sdp_de_reader_next(&reader));
    EXPECT_EQ(bte_sdp_de_reader_get_type(&reader), BTE_SDP_DE_TYPE_INVALID);
    EXPECT_FALSE(bte_sdp_de_reader_next(&reader));
    EXPECT_TRUE(bte_sdp_de_reader_leave(&reader));
    EXPECT_TRUE(bte_sdp_de_reader_next(&reader));

    EXPECT_EQ(bte_sdp_de_reader_get_type(&reader), BTE_SDP_DE_TYPE_UINT8);
    EXPECT_EQ(bte_sdp_de_reader_read_uint8(&reader), 42);
    EXPECT_FALSE(bte_sdp_de_reader_next(&reader));

    EXPECT_TRUE(bte_sdp_de_reader_leave(&reader));
    EXPECT_FALSE(bte_sdp_de_reader_next(&reader));
}

TEST_F(TestSdpDeFixture, testReadSequenceLeaveEarly)
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
        BTE_SDP_DE_END);
    buffer.resize(size);

    BteSdpDeReader reader;
    bte_sdp_de_reader_init(&reader, buffer.data());
    EXPECT_EQ(bte_sdp_de_reader_get_type(&reader), BTE_SDP_DE_TYPE_SEQUENCE);
    EXPECT_TRUE(bte_sdp_de_reader_enter(&reader));
    EXPECT_TRUE(bte_sdp_de_reader_next(&reader));

    EXPECT_EQ(bte_sdp_de_reader_get_type(&reader), BTE_SDP_DE_TYPE_BOOL);
    EXPECT_FALSE(bte_sdp_de_reader_enter(&reader));
    EXPECT_TRUE(bte_sdp_de_reader_next(&reader));

    EXPECT_EQ(bte_sdp_de_reader_get_type(&reader), BTE_SDP_DE_TYPE_SEQUENCE);
    EXPECT_TRUE(bte_sdp_de_reader_enter(&reader));
    EXPECT_TRUE(bte_sdp_de_reader_next(&reader));
    EXPECT_EQ(bte_sdp_de_reader_get_type(&reader), BTE_SDP_DE_TYPE_UINT16);
    EXPECT_TRUE(bte_sdp_de_reader_leave(&reader));
    EXPECT_TRUE(bte_sdp_de_reader_next(&reader));

    EXPECT_EQ(bte_sdp_de_reader_get_type(&reader), BTE_SDP_DE_TYPE_SEQUENCE);
    EXPECT_TRUE(bte_sdp_de_reader_enter(&reader));
    EXPECT_TRUE(bte_sdp_de_reader_next(&reader));

    EXPECT_EQ(bte_sdp_de_reader_get_type(&reader), BTE_SDP_DE_TYPE_UUID32);
    EXPECT_TRUE(bte_sdp_de_reader_next(&reader));

    EXPECT_EQ(bte_sdp_de_reader_get_type(&reader), BTE_SDP_DE_TYPE_UUID16);
    EXPECT_TRUE(bte_sdp_de_reader_leave(&reader));
    EXPECT_FALSE(bte_sdp_de_reader_next(&reader));

    EXPECT_EQ(bte_sdp_de_reader_get_type(&reader), BTE_SDP_DE_TYPE_INVALID);
    EXPECT_TRUE(bte_sdp_de_reader_leave(&reader));

    EXPECT_FALSE(bte_sdp_de_reader_next(&reader));
    EXPECT_FALSE(bte_sdp_de_reader_leave(&reader));
    EXPECT_FALSE(bte_sdp_de_reader_next(&reader));
}

TEST_F(TestSdpDeFixture, testReadInvalidCursor)
{
    uint32_t size = bte_sdp_de_write(buffer.data(), buffer.size(),
        BTE_SDP_DE_TYPE_SEQUENCE,
        BTE_SDP_DE_END);
    buffer.resize(size);

    BteSdpDeReader reader;
    bte_sdp_de_reader_init(&reader, buffer.data());
    EXPECT_TRUE(bte_sdp_de_reader_enter(&reader));

    EXPECT_EQ(bte_sdp_de_reader_read_uint8(&reader), 0);
    EXPECT_EQ(bte_sdp_de_reader_read_uint16(&reader), 0);
    EXPECT_EQ(bte_sdp_de_reader_read_uuid16(&reader), 0);
    EXPECT_EQ(bte_sdp_de_reader_read_uint32(&reader), 0);
    EXPECT_EQ(bte_sdp_de_reader_read_uint64(&reader), 0);

    EXPECT_EQ(bte_sdp_de_reader_read_int8(&reader), 0);
    EXPECT_EQ(bte_sdp_de_reader_read_int16(&reader), 0);
    EXPECT_EQ(bte_sdp_de_reader_read_int32(&reader), 0);
    EXPECT_EQ(bte_sdp_de_reader_read_int64(&reader), 0);

    BteSdpDeUuid128 uuid = { 0, };
    EXPECT_EQ(bte_sdp_de_reader_read_uuid128(&reader), uuid);
#ifdef __SIZEOF_INT128__
    EXPECT_EQ(bte_sdp_de_reader_read_uint128(&reader), 0);
    EXPECT_EQ(bte_sdp_de_reader_read_int128(&reader), 0);
#endif

    EXPECT_EQ(bte_sdp_de_reader_read_str(&reader, NULL), nullptr);
}
