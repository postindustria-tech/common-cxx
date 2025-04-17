/* *********************************************************************
 * This Original Work is copyright of 51 Degrees Mobile Experts Limited.
 * Copyright 2025 51 Degrees Mobile Experts Limited, Davidson House,
 * Forbury Square, Reading, Berkshire, United Kingdom RG1 3EU.
 *
 * This Original Work is licensed under the European Union Public Licence
 * (EUPL) v.1.2 and is subject to its terms as set out below.
 *
 * If a copy of the EUPL was not distributed with this file, You can obtain
 * one at https://opensource.org/licenses/EUPL-1.2.
 *
 * The 'Compatible Licences' set out in the Appendix to the EUPL (as may be
 * amended by the European Commission) shall be deemed incompatible for
 * the purposes of the Work and the provisions of the compatibility
 * clause in Article 5 of the EUPL shall not apply.
 *
 * If using the Work as, or as part of, a network application, by
 * including the attribution notice(s) required under Article 5 of the EUPL
 * in the end user terms of the application under an appropriate heading,
 * such notice(s) shall fulfill the requirements of that article.
 * ********************************************************************* */

#include "../fiftyone.h"
#include "../string.h"
#include "Base.hpp"

static void destroyCollection(fiftyoneDegreesCollection * const ptr) {
    if (ptr) {
        ptr->freeCollection(ptr);
    }
}
using CollectionPtr = std::unique_ptr<fiftyoneDegreesCollection, decltype(&destroyCollection)>;

typedef uint32_t Offset;
typedef std::vector<byte> ByteBuffer;

class StoredBinaryValues : public Base {
public:
    void SetUp() override;
    void TearDown() override;
    ByteBuffer rawStringsBuffer;
    CollectionPtr collection { nullptr, destroyCollection };
    struct {
        Offset string1;
        Offset string2;
        Offset ipv4;
        Offset ipv6;
        Offset wkb;
        Offset shortValue;
    } offsets;
    CollectionHeader header;
};

static constexpr char string1_rawValueBytes[] = "\x12\0some-string-value";
static constexpr char string2_rawValueBytes[] = "\xE\0another-world";
static constexpr byte ipv4_rawValueBytes[] = {
    0x04, 0x00,
    0xD4, 0x0C, 0x00, 0x01,
};
static constexpr byte ipv6_rawValueBytes[] = {
    0x10, 0x00,
    0x20,0x01,0x0d,0xb8,
    0x85,0xa3,0x00,0x00,
    0x00,0x00,0x8a,0x2e,
    0x03,0x70,0x73,0x34,
};
static constexpr byte wkb_rawValueBytes[] = {
    0x14, 0x00,
    0x00,
    0x00, 0x00, 0x00, 0x01,
    0x40, 0x31, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x40, 0x8b, 0xe0, 0xc0, 0x00, 0x00, 0x00, 0x00,
};
static constexpr byte shortValue_rawValueBytes[] = {
    0x60, 0x60, // 24672
};

static CollectionPtr buildMemoryCollection(
    ByteBuffer &rawStringsBuffer,
    CollectionHeader &header) {
    byte * const ptr = rawStringsBuffer.data();
    MemoryReader reader = {
        .startByte = ptr,
        .current = ptr,
        .lastByte = ptr + rawStringsBuffer.size(),
        .length = (FileOffset)rawStringsBuffer.size(),
    };
    fiftyoneDegreesCollection * const collection = CollectionCreateFromMemory(
        &reader,
        header);
    CollectionPtr result(collection, destroyCollection);
    return result;
}

void StoredBinaryValues::SetUp() {
    // add some junk into start to emulate file header
    rawStringsBuffer.push_back(42);
    rawStringsBuffer.push_back(29);
    rawStringsBuffer.push_back(13);
    // add contents
#   define add_value_to_buffer(x) \
    offsets.x = (Offset)rawStringsBuffer.size(); \
    for (size_t i = 0; i < sizeof(x##_rawValueBytes); i++) { \
        rawStringsBuffer.push_back(x##_rawValueBytes[i]); \
    }
    add_value_to_buffer(string1)
    add_value_to_buffer(string2)
    add_value_to_buffer(ipv4)
    add_value_to_buffer(ipv6)
    add_value_to_buffer(wkb)
    add_value_to_buffer(shortValue)
#   undef add_value_to_buffer
    header = {
        .startPosition = 0,
        .length = (uint32_t)rawStringsBuffer.size(),
        .count = (uint32_t)rawStringsBuffer.size(),
    };
    collection = buildMemoryCollection(rawStringsBuffer, header);
}

void StoredBinaryValues::TearDown() {
}

static void releaseItem(Item * const item) {
    if (item) {
        COLLECTION_RELEASE(item->collection, item);
    }
}
using ItemPtr = std::unique_ptr<Item, decltype(&releaseItem)>;

class ItemBox {
    Item item = {};
public:
    ItemBox() {
        DataReset(&item.data);
    }
    ~ItemBox() {
        if (item.collection) {
            COLLECTION_RELEASE(item.collection, &item);
        }
    }
    Item *operator*() { return &item; }
};

TEST_F(StoredBinaryValues, StoredBinaryValue_Get_String1) {
    EXCEPTION_CREATE;
    ItemBox item;
    StoredBinaryValue * const value = StoredBinaryValueGet(
        collection.get(),
        offsets.string1,
        FIFTYONE_DEGREES_PROPERTY_VALUE_TYPE_STRING,
        *item,
        exception);
    ASSERT_EQ(rawStringsBuffer.data() + offsets.string1, (byte *)value);
    ASSERT_EQ(sizeof(string1_rawValueBytes) - 2, value->stringValue.size);
    for (size_t i = 0; i < sizeof(string1_rawValueBytes) - 2; i++) {
        ASSERT_EQ(string1_rawValueBytes[i + 2], (&value->stringValue.value)[i]);
    }
}

TEST_F(StoredBinaryValues, StoredBinaryValue_Get_String2) {
    EXCEPTION_CREATE;
    ItemBox item;
    StoredBinaryValue * const value = StoredBinaryValueGet(
        collection.get(),
        offsets.string2,
        FIFTYONE_DEGREES_PROPERTY_VALUE_TYPE_STRING,
        *item,
        exception);
    ASSERT_EQ(rawStringsBuffer.data() + offsets.string2, (byte *)value);
    ASSERT_EQ(sizeof(string2_rawValueBytes) - 2, value->stringValue.size);
    for (size_t i = 0; i < sizeof(string2_rawValueBytes) - 2; i++) {
        ASSERT_EQ(string2_rawValueBytes[i + 2], (&value->stringValue.value)[i]);
    }
}