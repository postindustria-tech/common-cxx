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

static void freeCollection(fiftyoneDegreesCollection * const ptr) {
    if (ptr) {
        ptr->freeCollection(ptr);
    }
}
using CollectionPtr = std::unique_ptr<fiftyoneDegreesCollection, decltype(&freeCollection)>;
static void releaseFilePool(FilePool * const ptr) {
    if (ptr) {
        FilePoolRelease(ptr);
        delete ptr;
    }
}
using FilePoolPtr = std::unique_ptr<FilePool, decltype(&releaseFilePool)>;
using FileHandlePtr = std::unique_ptr<FileHandle, decltype(&FileHandleRelease)>;

typedef uint32_t Offset;
typedef std::vector<byte> ByteBuffer;

class StoredBinaryValues : public Base {
public:
    void SetUp() override;
    void TearDown() override;

    ByteBuffer rawStringsBuffer;
    struct {
        Offset string1;
        Offset string2;
        Offset ipv4;
        Offset ipv6;
        Offset wkb;
        Offset shortValue;
        Offset floatValue;
        Offset intValue;
    } offsets = {};

    struct FileProps {
        fiftyoneDegreesCollectionConfig config = {
            0, // loaded
            0, // capacity
            11, // concurrency
        };
        FilePoolPtr pool { nullptr, releaseFilePool };
        FileHandlePtr handle { nullptr, FileHandleRelease };
    } file = {};

    CollectionHeader header = {};
    struct {
        CollectionPtr memory { nullptr, freeCollection };
        CollectionPtr file { nullptr, freeCollection };
    } collection;
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
    0x15, 0x00,
    0x00,
    0x00, 0x00, 0x00, 0x01,
    0x40, 0x31, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x40, 0x8b, 0xe0, 0xc0, 0x00, 0x00, 0x00, 0x00,
};

static constexpr byte shortValue_rawValueBytes[] = {
    0x60, 0x60,
};
static constexpr int16_t shortValue_rawValue = 24672;
static constexpr double shortValue_azimuth = 135.53147984252449110385448774682;
static constexpr double shortValue_declination = 67.765739921262245551927243873409;

static constexpr byte floatValue_rawValueBytes[] = {
    0xCE, 0x2E, 0x88, 0x04,
};
static constexpr float floatValue_rawValue = 3.201642965935704e-36f;

static constexpr byte intValue_rawValueBytes[] = {
    0x65, 0x7A, 0x20, 0x52,
};
static constexpr int intValue_rawValue = 1377860197;

static CollectionPtr buildMemoryCollection(
    ByteBuffer &rawStringsBuffer,
    const CollectionHeader &header) {
    byte * const ptr = rawStringsBuffer.data();
    MemoryReader reader = {
        ptr, // startByte
        ptr, // current
        ptr + rawStringsBuffer.size(), // lastByte
        (FileOffset)rawStringsBuffer.size(), // length
    };
    fiftyoneDegreesCollection * const collection = CollectionCreateFromMemory(
        &reader,
        header);
    CollectionPtr result(collection, freeCollection);
    return result;
}

static constexpr char fileName[] = "StoredBinaryValueTests_Data.hex";

static CollectionPtr buildFileCollection(
    StoredBinaryValues::FileProps &fileProps,
    const CollectionHeader &header) {
    EXCEPTION_CREATE;

    auto const pool = new FilePool();
    FilePoolInit(
        pool,
        fileName,
        fileProps.config.concurrency,
        exception);
    EXCEPTION_THROW;
    fileProps.pool = FilePoolPtr(pool, releaseFilePool);

    fileProps.handle = FileHandlePtr(
        FileHandleGet(pool, exception),
        FileHandleRelease);
    EXCEPTION_THROW;

    fiftyoneDegreesCollection * const collection = CollectionCreateFromFile(
        fileProps.handle->file,
        fileProps.pool.get(),
        &fileProps.config,
        header,
        StoredBinaryValueRead);
    CollectionPtr result(collection, freeCollection);
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
    add_value_to_buffer(floatValue)
    add_value_to_buffer(intValue)
#   undef add_value_to_buffer

    FileWrite(fileName, rawStringsBuffer.data(), rawStringsBuffer.size());

    header = {
        0, // startPosition
        (uint32_t)rawStringsBuffer.size(), // length
        (uint32_t)rawStringsBuffer.size(), // count
    };

    collection.memory = buildMemoryCollection(rawStringsBuffer, header);
    collection.file = buildFileCollection(file, header);
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
    Item *operator->() { return &item; }
};


// ============== StoredBinaryValueGet (from memory) ==============


TEST_F(StoredBinaryValues, StoredBinaryValue_Get_String1_Direct_FromMemory) {
    EXCEPTION_CREATE;
    ItemBox item;
    auto * const value = (String *)collection.memory->get(
        collection.memory.get(),
        offsets.string1,
        *item,
        exception);
    ASSERT_EQ(rawStringsBuffer.data() + offsets.string1, (byte *)value);
    ASSERT_EQ(sizeof(string1_rawValueBytes) - 2, value->size);
    for (size_t i = 0; i < sizeof(string1_rawValueBytes) - 2; i++) {
        ASSERT_EQ(string1_rawValueBytes[i + 2], (&value->value)[i]);
    }
}

TEST_F(StoredBinaryValues, StoredBinaryValue_Get_String1_FromMemory) {
    EXCEPTION_CREATE;
    ItemBox item;
    StoredBinaryValue * const value = StoredBinaryValueGet(
        collection.memory.get(),
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

TEST_F(StoredBinaryValues, StoredBinaryValue_Get_String2_FromMemory) {
    EXCEPTION_CREATE;
    ItemBox item;
    StoredBinaryValue * const value = StoredBinaryValueGet(
        collection.memory.get(),
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

TEST_F(StoredBinaryValues, StoredBinaryValue_Get_IPv4_FromMemory) {
    EXCEPTION_CREATE;
    ItemBox item;
    StoredBinaryValue * const value = StoredBinaryValueGet(
        collection.memory.get(),
        offsets.ipv4,
        FIFTYONE_DEGREES_PROPERTY_VALUE_TYPE_IP_ADDRESS,
        *item,
        exception);
    ASSERT_EQ(rawStringsBuffer.data() + offsets.ipv4, (byte *)value);
    ASSERT_EQ(sizeof(ipv4_rawValueBytes) - 2, value->byteArrayValue.size);
    for (size_t i = 0; i < sizeof(ipv4_rawValueBytes) - 2; i++) {
        ASSERT_EQ(ipv4_rawValueBytes[i + 2], (&value->byteArrayValue.firstByte)[i]);
    }
}

TEST_F(StoredBinaryValues, StoredBinaryValue_Get_IPv6_FromMemory) {
    EXCEPTION_CREATE;
    ItemBox item;
    StoredBinaryValue * const value = StoredBinaryValueGet(
        collection.memory.get(),
        offsets.ipv6,
        FIFTYONE_DEGREES_PROPERTY_VALUE_TYPE_IP_ADDRESS,
        *item,
        exception);
    ASSERT_EQ(rawStringsBuffer.data() + offsets.ipv6, (byte *)value);
    ASSERT_EQ(sizeof(ipv6_rawValueBytes) - 2, value->byteArrayValue.size);
    for (size_t i = 0; i < sizeof(ipv6_rawValueBytes) - 2; i++) {
        ASSERT_EQ(ipv6_rawValueBytes[i + 2], (&value->byteArrayValue.firstByte)[i]);
    }
}

TEST_F(StoredBinaryValues, StoredBinaryValue_Get_WKB_FromMemory) {
    EXCEPTION_CREATE;
    ItemBox item;
    StoredBinaryValue * const value = StoredBinaryValueGet(
        collection.memory.get(),
        offsets.wkb,
        FIFTYONE_DEGREES_PROPERTY_VALUE_TYPE_IP_ADDRESS,
        *item,
        exception);
    ASSERT_EQ(rawStringsBuffer.data() + offsets.wkb, (byte *)value);
    ASSERT_EQ(sizeof(wkb_rawValueBytes) - 2, value->byteArrayValue.size);
    for (size_t i = 0; i < sizeof(wkb_rawValueBytes) - 2; i++) {
        ASSERT_EQ(wkb_rawValueBytes[i + 2], (&value->byteArrayValue.firstByte)[i]);
    }
}

TEST_F(StoredBinaryValues, StoredBinaryValue_Get_Azimuth_FromMemory) {
    EXCEPTION_CREATE;
    ItemBox item;
    StoredBinaryValue * const value = StoredBinaryValueGet(
        collection.memory.get(),
        offsets.shortValue,
        FIFTYONE_DEGREES_PROPERTY_VALUE_TYPE_AZIMUTH,
        *item,
        exception);
    ASSERT_EQ(rawStringsBuffer.data() + offsets.shortValue, (byte *)value);
    for (size_t i = 0; i < sizeof(shortValue_rawValueBytes); i++) {
        ASSERT_EQ(shortValue_rawValueBytes[i], ((byte *)value)[i]);
    }
    ASSERT_EQ(shortValue_rawValue, value->shortValue);
    ASSERT_EQ(shortValue_azimuth, StoredBinaryValueToDoubleOrDefault(
        value,
        FIFTYONE_DEGREES_PROPERTY_VALUE_TYPE_AZIMUTH,
        0));
}

TEST_F(StoredBinaryValues, StoredBinaryValue_Get_Declination_FromMemory) {
    EXCEPTION_CREATE;
    ItemBox item;
    StoredBinaryValue * const value = StoredBinaryValueGet(
        collection.memory.get(),
        offsets.shortValue,
        FIFTYONE_DEGREES_PROPERTY_VALUE_TYPE_DECLINATION,
        *item,
        exception);
    ASSERT_EQ(rawStringsBuffer.data() + offsets.shortValue, (byte *)value);
    for (size_t i = 0; i < sizeof(shortValue_rawValueBytes); i++) {
        ASSERT_EQ(shortValue_rawValueBytes[i], ((byte *)value)[i]);
    }
    ASSERT_EQ(shortValue_rawValue, value->shortValue);
    ASSERT_EQ(shortValue_declination, StoredBinaryValueToDoubleOrDefault(
        value,
        FIFTYONE_DEGREES_PROPERTY_VALUE_TYPE_DECLINATION,
        0));
}

TEST_F(StoredBinaryValues, StoredBinaryValue_Get_Float_FromMemory) {
    EXCEPTION_CREATE;
    ItemBox item;
    StoredBinaryValue * const value = StoredBinaryValueGet(
        collection.memory.get(),
        offsets.floatValue,
        FIFTYONE_DEGREES_PROPERTY_VALUE_SINGLE_PRECISION_FLOAT,
        *item,
        exception);
    ASSERT_EQ(rawStringsBuffer.data() + offsets.floatValue, (byte *)value);
    for (size_t i = 0; i < sizeof(floatValue_rawValueBytes); i++) {
        ASSERT_EQ(floatValue_rawValueBytes[i], ((byte *)value)[i]);
    }
    ASSERT_EQ(floatValue_rawValue, FLOAT_TO_NATIVE(value->floatValue));
}

TEST_F(StoredBinaryValues, StoredBinaryValue_Get_Integer_FromMemory) {
    EXCEPTION_CREATE;
    ItemBox item;
    StoredBinaryValue * const value = StoredBinaryValueGet(
        collection.memory.get(),
        offsets.intValue,
        FIFTYONE_DEGREES_PROPERTY_VALUE_TYPE_INTEGER,
        *item,
        exception);
    ASSERT_EQ(rawStringsBuffer.data() + offsets.intValue, (byte *)value);
    for (size_t i = 0; i < sizeof(intValue_rawValueBytes); i++) {
        ASSERT_EQ(intValue_rawValueBytes[i], ((byte *)value)[i]);
    }
    ASSERT_EQ(intValue_rawValue, value->intValue);
}

TEST_F(StoredBinaryValues, StoredBinaryValue_Get_Object_FromMemory) {
    EXCEPTION_CREATE;
    ItemBox item;
    StoredBinaryValue * const value = StoredBinaryValueGet(
        collection.memory.get(),
        offsets.intValue,
        FIFTYONE_DEGREES_PROPERTY_VALUE_TYPE_OBJECT,
        *item,
        exception);
    // In-Memory collection does NOT care about property type
    // => no exception
    ASSERT_TRUE(EXCEPTION_OKAY);
    ASSERT_EQ(rawStringsBuffer.data() + offsets.intValue, (byte *)value);
    for (size_t i = 0; i < sizeof(intValue_rawValueBytes); i++) {
        ASSERT_EQ(intValue_rawValueBytes[i], ((byte *)value)[i]);
    }
    ASSERT_EQ(intValue_rawValue, value->intValue);
}


// ============== StoredBinaryValueGet (from file) ==============


TEST_F(StoredBinaryValues, StoredBinaryValue_Get_String1_Direct_FromFile) {
    EXCEPTION_CREATE;
    ItemBox item;
    auto * const value = (String *)collection.file->get(
        collection.file.get(),
        offsets.string1,
        *item,
        exception);
    EXCEPTION_THROW;
    ASSERT_NE(nullptr, value);
    ASSERT_EQ(sizeof(string1_rawValueBytes), item->data.allocated);
    ASSERT_EQ(sizeof(string1_rawValueBytes) - 2, value->size);
    for (size_t i = 0; i < sizeof(string1_rawValueBytes) - 2; i++) {
        ASSERT_EQ(string1_rawValueBytes[i + 2], (&value->value)[i]);
    }
}

TEST_F(StoredBinaryValues, StoredBinaryValue_Get_String1_FromFile) {
    EXCEPTION_CREATE;
    ItemBox item;
    StoredBinaryValue * const value = StoredBinaryValueGet(
        collection.file.get(),
        offsets.string1,
        FIFTYONE_DEGREES_PROPERTY_VALUE_TYPE_STRING,
        *item,
        exception);
    EXCEPTION_THROW;
    ASSERT_NE(nullptr, value);
    ASSERT_EQ(sizeof(string1_rawValueBytes), item->data.allocated);
    ASSERT_EQ(sizeof(string1_rawValueBytes) - 2, value->stringValue.size);
    for (size_t i = 0; i < sizeof(string1_rawValueBytes) - 2; i++) {
        ASSERT_EQ(string1_rawValueBytes[i + 2], (&value->stringValue.value)[i]);
    }
}

TEST_F(StoredBinaryValues, StoredBinaryValue_Get_String2_FromFile) {
    EXCEPTION_CREATE;
    ItemBox item;
    StoredBinaryValue * const value = StoredBinaryValueGet(
        collection.file.get(),
        offsets.string2,
        FIFTYONE_DEGREES_PROPERTY_VALUE_TYPE_STRING,
        *item,
        exception);
    EXCEPTION_THROW;
    ASSERT_NE(nullptr, value);
    ASSERT_EQ(sizeof(string2_rawValueBytes), item->data.allocated);
    ASSERT_EQ(sizeof(string2_rawValueBytes) - 2, value->stringValue.size);
    for (size_t i = 0; i < sizeof(string2_rawValueBytes) - 2; i++) {
        ASSERT_EQ(string2_rawValueBytes[i + 2], (&value->stringValue.value)[i]);
    }
}

TEST_F(StoredBinaryValues, StoredBinaryValue_Get_IPv4_FromFile) {
    EXCEPTION_CREATE;
    ItemBox item;
    StoredBinaryValue * const value = StoredBinaryValueGet(
        collection.file.get(),
        offsets.ipv4,
        FIFTYONE_DEGREES_PROPERTY_VALUE_TYPE_IP_ADDRESS,
        *item,
        exception);
    EXCEPTION_THROW;
    ASSERT_NE(nullptr, value);
    ASSERT_EQ(sizeof(ipv4_rawValueBytes), item->data.allocated);
    ASSERT_EQ(sizeof(ipv4_rawValueBytes) - 2, value->byteArrayValue.size);
    for (size_t i = 0; i < sizeof(ipv4_rawValueBytes) - 2; i++) {
        ASSERT_EQ(ipv4_rawValueBytes[i + 2], (&value->byteArrayValue.firstByte)[i]);
    }
}

TEST_F(StoredBinaryValues, StoredBinaryValue_Get_IPv6_FromFile) {
    EXCEPTION_CREATE;
    ItemBox item;
    StoredBinaryValue * const value = StoredBinaryValueGet(
        collection.file.get(),
        offsets.ipv6,
        FIFTYONE_DEGREES_PROPERTY_VALUE_TYPE_IP_ADDRESS,
        *item,
        exception);
    EXCEPTION_THROW;
    ASSERT_NE(nullptr, value);
    ASSERT_EQ(sizeof(ipv6_rawValueBytes), item->data.allocated);
    ASSERT_EQ(sizeof(ipv6_rawValueBytes) - 2, value->byteArrayValue.size);
    for (size_t i = 0; i < sizeof(ipv6_rawValueBytes) - 2; i++) {
        ASSERT_EQ(ipv6_rawValueBytes[i + 2], (&value->byteArrayValue.firstByte)[i]);
    }
}

TEST_F(StoredBinaryValues, StoredBinaryValue_Get_WKB_FromFile) {
    EXCEPTION_CREATE;
    ItemBox item;
    StoredBinaryValue * const value = StoredBinaryValueGet(
        collection.file.get(),
        offsets.wkb,
        FIFTYONE_DEGREES_PROPERTY_VALUE_TYPE_IP_ADDRESS,
        *item,
        exception);
    EXCEPTION_THROW;
    ASSERT_NE(nullptr, value);
    ASSERT_EQ(sizeof(wkb_rawValueBytes), item->data.allocated);
    ASSERT_EQ(sizeof(wkb_rawValueBytes) - 2, value->byteArrayValue.size);
    for (size_t i = 0; i < sizeof(wkb_rawValueBytes) - 2; i++) {
        ASSERT_EQ(wkb_rawValueBytes[i + 2], (&value->byteArrayValue.firstByte)[i]);
    }
}

TEST_F(StoredBinaryValues, StoredBinaryValue_Get_Azimuth_FromFile) {
    EXCEPTION_CREATE;
    ItemBox item;
    StoredBinaryValue * const value = StoredBinaryValueGet(
        collection.file.get(),
        offsets.shortValue,
        FIFTYONE_DEGREES_PROPERTY_VALUE_TYPE_AZIMUTH,
        *item,
        exception);
    EXCEPTION_THROW;
    ASSERT_NE(nullptr, value);
    ASSERT_EQ(sizeof(shortValue_rawValueBytes), item->data.allocated);
    for (size_t i = 0; i < sizeof(shortValue_rawValueBytes); i++) {
        ASSERT_EQ(shortValue_rawValueBytes[i], ((byte *)value)[i]);
    }
    ASSERT_EQ(shortValue_rawValue, value->shortValue);
    ASSERT_EQ(shortValue_azimuth, StoredBinaryValueToDoubleOrDefault(
        value,
        FIFTYONE_DEGREES_PROPERTY_VALUE_TYPE_AZIMUTH,
        0));
}

TEST_F(StoredBinaryValues, StoredBinaryValue_Get_Declination_FromFile) {
    EXCEPTION_CREATE;
    ItemBox item;
    StoredBinaryValue * const value = StoredBinaryValueGet(
        collection.file.get(),
        offsets.shortValue,
        FIFTYONE_DEGREES_PROPERTY_VALUE_TYPE_DECLINATION,
        *item,
        exception);
    EXCEPTION_THROW;
    ASSERT_NE(nullptr, value);
    ASSERT_EQ(sizeof(shortValue_rawValueBytes), item->data.allocated);
    for (size_t i = 0; i < sizeof(shortValue_rawValueBytes); i++) {
        ASSERT_EQ(shortValue_rawValueBytes[i], ((byte *)value)[i]);
    }
    ASSERT_EQ(shortValue_rawValue, value->shortValue);
    ASSERT_EQ(shortValue_declination, StoredBinaryValueToDoubleOrDefault(
        value,
        FIFTYONE_DEGREES_PROPERTY_VALUE_TYPE_DECLINATION,
        0));
}

TEST_F(StoredBinaryValues, StoredBinaryValue_Get_Float_FromFile) {
    EXCEPTION_CREATE;
    ItemBox item;
    StoredBinaryValue * const value = StoredBinaryValueGet(
        collection.file.get(),
        offsets.floatValue,
        FIFTYONE_DEGREES_PROPERTY_VALUE_SINGLE_PRECISION_FLOAT,
        *item,
        exception);
    EXCEPTION_THROW;
    ASSERT_NE(nullptr, value);
    ASSERT_EQ(sizeof(floatValue_rawValueBytes), item->data.allocated);
    for (size_t i = 0; i < sizeof(floatValue_rawValueBytes); i++) {
        ASSERT_EQ(floatValue_rawValueBytes[i], ((byte *)value)[i]);
    }
    ASSERT_EQ(floatValue_rawValue, FLOAT_TO_NATIVE(value->floatValue));
}

TEST_F(StoredBinaryValues, StoredBinaryValue_Get_Integer_FromFile) {
    EXCEPTION_CREATE;
    ItemBox item;
    StoredBinaryValue * const value = StoredBinaryValueGet(
        collection.file.get(),
        offsets.intValue,
        FIFTYONE_DEGREES_PROPERTY_VALUE_TYPE_INTEGER,
        *item,
        exception);
    EXCEPTION_THROW;
    ASSERT_NE(nullptr, value);
    ASSERT_EQ(sizeof(intValue_rawValueBytes), item->data.allocated);
    for (size_t i = 0; i < sizeof(intValue_rawValueBytes); i++) {
        ASSERT_EQ(intValue_rawValueBytes[i], ((byte *)value)[i]);
    }
    ASSERT_EQ(intValue_rawValue, value->intValue);
}

TEST_F(StoredBinaryValues, StoredBinaryValue_Get_Object_FromFile) {
    EXCEPTION_CREATE;
    ItemBox item;
    StoredBinaryValue * const value = StoredBinaryValueGet(
        collection.file.get(),
        offsets.intValue,
        FIFTYONE_DEGREES_PROPERTY_VALUE_TYPE_OBJECT,
        *item,
        exception);
    ASSERT_FALSE(EXCEPTION_OKAY);
    ASSERT_EQ(UNSUPPORTED_STORED_VALUE_TYPE, exception->status);
    ASSERT_FALSE(value);
}


// ============== StoredBinaryValueToIntOrDefault ==============


TEST_F(StoredBinaryValues, StoredBinaryValue_ToInt_String_DoubleZero) {
    const char rawBytes[] = { 3, 0, '0', '0', 0 };
    const int result = StoredBinaryValueToIntOrDefault(
        (const StoredBinaryValue *)rawBytes,
        FIFTYONE_DEGREES_PROPERTY_VALUE_TYPE_STRING,
        -1);
    EXPECT_EQ(0, result);
}

TEST_F(StoredBinaryValues, StoredBinaryValue_ToInt_String_x7) {
    const char rawBytes[] = { 3, 0, 'x', '7', 0 };
    const int result = StoredBinaryValueToIntOrDefault(
        (const StoredBinaryValue *)rawBytes,
        FIFTYONE_DEGREES_PROPERTY_VALUE_TYPE_STRING,
        -1);
    EXPECT_EQ(0, result);
}

TEST_F(StoredBinaryValues, StoredBinaryValue_ToInt_String_7x4) {
    const char rawBytes[] = { 4, 0, '7', 'x', '4', 0 };
    const int result = StoredBinaryValueToIntOrDefault(
        (const StoredBinaryValue *)rawBytes,
        FIFTYONE_DEGREES_PROPERTY_VALUE_TYPE_STRING,
        -1);
    EXPECT_EQ(7, result);
}

TEST_F(StoredBinaryValues, StoredBinaryValue_ToInt_String_15) {
    const char rawBytes[] = { 3, 0, '1', '5', 0 };
    const int result = StoredBinaryValueToIntOrDefault(
        (const StoredBinaryValue *)rawBytes,
        FIFTYONE_DEGREES_PROPERTY_VALUE_TYPE_STRING,
        -1);
    EXPECT_EQ(15, result);
}

TEST_F(StoredBinaryValues, StoredBinaryValue_ToInt_String_minus827) {
    const char rawBytes[] = { 5, 0, '-', '8', '2', '7', 0 };
    const int result = StoredBinaryValueToIntOrDefault(
        (const StoredBinaryValue *)rawBytes,
        FIFTYONE_DEGREES_PROPERTY_VALUE_TYPE_STRING,
        -1);
    EXPECT_EQ(-827, result);
}

TEST_F(StoredBinaryValues, StoredBinaryValue_ToInt_String_3_95) {
    const char rawBytes[] = { 5, 0, '3', '.', '9', '5', 0 };
    const int result = StoredBinaryValueToIntOrDefault(
        (const StoredBinaryValue *)rawBytes,
        FIFTYONE_DEGREES_PROPERTY_VALUE_TYPE_STRING,
        -1);
    EXPECT_EQ(3, result);
}

TEST_F(StoredBinaryValues, StoredBinaryValue_ToInt_Integer_0) {
    const char rawBytes[] = { 0, 0, 0, 0 };
    const int result = StoredBinaryValueToIntOrDefault(
        (const StoredBinaryValue *)rawBytes,
        FIFTYONE_DEGREES_PROPERTY_VALUE_TYPE_INTEGER,
        -1);
    EXPECT_EQ(0, result);
}

TEST_F(StoredBinaryValues, StoredBinaryValue_ToInt_Integer_Positive) {
    const unsigned char rawBytes[] = { 0x50, 0xBE, 0x09, 0x1B };
    const int result = StoredBinaryValueToIntOrDefault(
        (const StoredBinaryValue *)rawBytes,
        FIFTYONE_DEGREES_PROPERTY_VALUE_TYPE_INTEGER,
        -1);
    EXPECT_EQ(453623376, result);
}

TEST_F(StoredBinaryValues, StoredBinaryValue_ToInt_Integer_Negative) {
    const unsigned char rawBytes[] = { 0xB0, 0x41, 0xF6, 0xE4 };
    const int result = StoredBinaryValueToIntOrDefault(
        (const StoredBinaryValue *)rawBytes,
        FIFTYONE_DEGREES_PROPERTY_VALUE_TYPE_INTEGER,
        -1);
    EXPECT_EQ(-453623376, result);
}

TEST_F(StoredBinaryValues, StoredBinaryValue_ToInt_Float_Zero) {
    const unsigned char rawBytes[] = { 0, 0, 0, 0 }; // 0.0
    const int result = StoredBinaryValueToIntOrDefault(
        (const StoredBinaryValue *)rawBytes,
        FIFTYONE_DEGREES_PROPERTY_VALUE_SINGLE_PRECISION_FLOAT,
        -1);
    EXPECT_EQ(0, result);
}

TEST_F(StoredBinaryValues, StoredBinaryValue_ToInt_Float_Positive) {
    const unsigned char rawBytes[] = { 0xCD, 0x5A, 0x34, 0x43 };
    const int result = StoredBinaryValueToIntOrDefault(
        (const StoredBinaryValue *)rawBytes,
        FIFTYONE_DEGREES_PROPERTY_VALUE_SINGLE_PRECISION_FLOAT,
        -1);
    EXPECT_EQ(180, result);
}

TEST_F(StoredBinaryValues, StoredBinaryValue_ToInt_Float_LargePositive) {
    const unsigned char rawBytes[] = { 0x5F, 0x43, 0xC2, 0x47 };
    const int result = StoredBinaryValueToIntOrDefault(
        (const StoredBinaryValue *)rawBytes,
        FIFTYONE_DEGREES_PROPERTY_VALUE_SINGLE_PRECISION_FLOAT,
        -1);
    EXPECT_EQ(99462, result);
}

TEST_F(StoredBinaryValues, StoredBinaryValue_ToInt_Float_Negative) {
    const unsigned char rawBytes[] = { 0x9E, 0x86, 0x90, 0xC2 };
    const int result = StoredBinaryValueToIntOrDefault(
        (const StoredBinaryValue *)rawBytes,
        FIFTYONE_DEGREES_PROPERTY_VALUE_SINGLE_PRECISION_FLOAT,
        -1);
    EXPECT_EQ(-72, result);
}

TEST_F(StoredBinaryValues, StoredBinaryValue_ToInt_Float_LargeNegative) {
    const unsigned char rawBytes[] = { 0xA6, 0x0B, 0xA0, 0xC7 };
    const int result = StoredBinaryValueToIntOrDefault(
        (const StoredBinaryValue *)rawBytes,
        FIFTYONE_DEGREES_PROPERTY_VALUE_SINGLE_PRECISION_FLOAT,
        -1);
    EXPECT_EQ(-81943, result);
}

TEST_F(StoredBinaryValues, StoredBinaryValue_ToInt_Azimuth) {
    const int result = StoredBinaryValueToIntOrDefault(
        (const StoredBinaryValue *)shortValue_rawValueBytes,
        FIFTYONE_DEGREES_PROPERTY_VALUE_TYPE_AZIMUTH,
        -1);
    EXPECT_EQ((int)shortValue_azimuth, result);
}

TEST_F(StoredBinaryValues, StoredBinaryValue_ToInt_Declination) {
    const int result = StoredBinaryValueToIntOrDefault(
        (const StoredBinaryValue *)shortValue_rawValueBytes,
        FIFTYONE_DEGREES_PROPERTY_VALUE_TYPE_DECLINATION,
        -1);
    EXPECT_EQ((int)shortValue_declination, result);
}

TEST_F(StoredBinaryValues, StoredBinaryValue_ToInt_Default_minus1) {
    const int result = StoredBinaryValueToIntOrDefault(
        (const StoredBinaryValue *)shortValue_rawValueBytes,
        FIFTYONE_DEGREES_PROPERTY_VALUE_TYPE_OBJECT,
        -1);
    EXPECT_EQ(-1, result);
}

TEST_F(StoredBinaryValues, StoredBinaryValue_ToInt_Default_542) {
    const int result = StoredBinaryValueToIntOrDefault(
        (const StoredBinaryValue *)shortValue_rawValueBytes,
        FIFTYONE_DEGREES_PROPERTY_VALUE_TYPE_OBJECT,
        542);
    EXPECT_EQ(542, result);
}


// ============== StoredBinaryValueToDoubleOrDefault ==============


TEST_F(StoredBinaryValues, StoredBinaryValue_ToDouble_String_DoubleZero) {
    const char rawBytes[] = { 3, 0, '0', '0', 0 };
    const double result = StoredBinaryValueToDoubleOrDefault(
        (const StoredBinaryValue *)rawBytes,
        FIFTYONE_DEGREES_PROPERTY_VALUE_TYPE_STRING,
        -1);
    EXPECT_EQ(0, result);
}

TEST_F(StoredBinaryValues, StoredBinaryValue_ToDouble_String_x7) {
    const char rawBytes[] = { 3, 0, 'x', '7', 0 };
    const double result = StoredBinaryValueToDoubleOrDefault(
        (const StoredBinaryValue *)rawBytes,
        FIFTYONE_DEGREES_PROPERTY_VALUE_TYPE_STRING,
        -1);
    EXPECT_EQ(0, result);
}

TEST_F(StoredBinaryValues, StoredBinaryValue_ToDouble_String_7x4) {
    const char rawBytes[] = { 4, 0, '7', 'x', '4', 0 };
    const double result = StoredBinaryValueToDoubleOrDefault(
        (const StoredBinaryValue *)rawBytes,
        FIFTYONE_DEGREES_PROPERTY_VALUE_TYPE_STRING,
        -1);
    EXPECT_EQ(7, result);
}

TEST_F(StoredBinaryValues, StoredBinaryValue_ToDouble_String_15) {
    const char rawBytes[] = { 3, 0, '1', '5', 0 };
    const double result = StoredBinaryValueToDoubleOrDefault(
        (const StoredBinaryValue *)rawBytes,
        FIFTYONE_DEGREES_PROPERTY_VALUE_TYPE_STRING,
        -1);
    EXPECT_EQ(15, result);
}

TEST_F(StoredBinaryValues, StoredBinaryValue_ToDouble_String_minus827) {
    const char rawBytes[] = { 5, 0, '-', '8', '2', '7', 0 };
    const double result = StoredBinaryValueToDoubleOrDefault(
        (const StoredBinaryValue *)rawBytes,
        FIFTYONE_DEGREES_PROPERTY_VALUE_TYPE_STRING,
        -1);
    EXPECT_EQ(-827, result);
}

TEST_F(StoredBinaryValues, StoredBinaryValue_ToDouble_String_3_95) {
    const char rawBytes[] = { 5, 0, '3', '.', '9', '5', 0 };
    const double result = StoredBinaryValueToDoubleOrDefault(
        (const StoredBinaryValue *)rawBytes,
        FIFTYONE_DEGREES_PROPERTY_VALUE_TYPE_STRING,
        -1);
    EXPECT_EQ(3.95, result);
}

TEST_F(StoredBinaryValues, StoredBinaryValue_ToDouble_Integer_0) {
    const char rawBytes[] = { 0, 0, 0, 0 };
    const double result = StoredBinaryValueToDoubleOrDefault(
        (const StoredBinaryValue *)rawBytes,
        FIFTYONE_DEGREES_PROPERTY_VALUE_TYPE_INTEGER,
        -1);
    EXPECT_EQ(0, result);
}

TEST_F(StoredBinaryValues, StoredBinaryValue_ToDouble_Integer_Positive) {
    const unsigned char rawBytes[] = { 0x50, 0xBE, 0x09, 0x1B };
    const double result = StoredBinaryValueToDoubleOrDefault(
        (const StoredBinaryValue *)rawBytes,
        FIFTYONE_DEGREES_PROPERTY_VALUE_TYPE_INTEGER,
        -1);
    EXPECT_EQ(453623376, result);
}

TEST_F(StoredBinaryValues, StoredBinaryValue_ToDouble_Integer_Negative) {
    const unsigned char rawBytes[] = { 0xB0, 0x41, 0xF6, 0xE4 };
    const double result = StoredBinaryValueToDoubleOrDefault(
        (const StoredBinaryValue *)rawBytes,
        FIFTYONE_DEGREES_PROPERTY_VALUE_TYPE_INTEGER,
        -1);
    EXPECT_EQ(-453623376, result);
}

TEST_F(StoredBinaryValues, StoredBinaryValue_ToDouble_Float_Zero) {
    const unsigned char rawBytes[] = { 0, 0, 0, 0 }; // 0.0
    const double result = StoredBinaryValueToDoubleOrDefault(
        (const StoredBinaryValue *)rawBytes,
        FIFTYONE_DEGREES_PROPERTY_VALUE_SINGLE_PRECISION_FLOAT,
        -1);
    EXPECT_EQ(0, result);
}

TEST_F(StoredBinaryValues, StoredBinaryValue_ToDouble_Float_Positive) {
    const unsigned char rawBytes[] = { 0xCD, 0x5A, 0x34, 0x43 };
    const double result = StoredBinaryValueToDoubleOrDefault(
        (const StoredBinaryValue *)rawBytes,
        FIFTYONE_DEGREES_PROPERTY_VALUE_SINGLE_PRECISION_FLOAT,
        -1);
    EXPECT_EQ(180.3546905517578125, result);
}

TEST_F(StoredBinaryValues, StoredBinaryValue_ToDouble_Float_LargePositive) {
    const unsigned char rawBytes[] = { 0x5F, 0x43, 0xC2, 0x47 };
    const double result = StoredBinaryValueToDoubleOrDefault(
        (const StoredBinaryValue *)rawBytes,
        FIFTYONE_DEGREES_PROPERTY_VALUE_SINGLE_PRECISION_FLOAT,
        -1);
    EXPECT_EQ(99462.7421875, result);
}

TEST_F(StoredBinaryValues, StoredBinaryValue_ToDouble_Float_Negative) {
    const unsigned char rawBytes[] = { 0x9E, 0x86, 0x90, 0xC2 };
    const double result = StoredBinaryValueToDoubleOrDefault(
        (const StoredBinaryValue *)rawBytes,
        FIFTYONE_DEGREES_PROPERTY_VALUE_SINGLE_PRECISION_FLOAT,
        -1);
    EXPECT_EQ(-72.2629241943359375, result);
}

TEST_F(StoredBinaryValues, StoredBinaryValue_ToDouble_Float_LargeNegative) {
    const unsigned char rawBytes[] = { 0xA6, 0x0B, 0xA0, 0xC7 };
    const double result = StoredBinaryValueToDoubleOrDefault(
        (const StoredBinaryValue *)rawBytes,
        FIFTYONE_DEGREES_PROPERTY_VALUE_SINGLE_PRECISION_FLOAT,
        -1);
    EXPECT_EQ(-81943.296875, result);
}

TEST_F(StoredBinaryValues, StoredBinaryValue_ToDouble_Azimuth) {
    const double result = StoredBinaryValueToDoubleOrDefault(
        (const StoredBinaryValue *)shortValue_rawValueBytes,
        FIFTYONE_DEGREES_PROPERTY_VALUE_TYPE_AZIMUTH,
        -1);
    EXPECT_EQ(shortValue_azimuth, result);
}

TEST_F(StoredBinaryValues, StoredBinaryValue_ToDouble_Declination) {
    const double result = StoredBinaryValueToDoubleOrDefault(
        (const StoredBinaryValue *)shortValue_rawValueBytes,
        FIFTYONE_DEGREES_PROPERTY_VALUE_TYPE_DECLINATION,
        -1);
    EXPECT_EQ(shortValue_declination, result);
}

TEST_F(StoredBinaryValues, StoredBinaryValue_ToDouble_Default_minus1) {
    const double result = StoredBinaryValueToDoubleOrDefault(
        (const StoredBinaryValue *)shortValue_rawValueBytes,
        FIFTYONE_DEGREES_PROPERTY_VALUE_TYPE_OBJECT,
        -1);
    EXPECT_EQ(-1, result);
}

TEST_F(StoredBinaryValues, StoredBinaryValue_ToDouble_Default_54_2) {
    const double result = StoredBinaryValueToDoubleOrDefault(
        (const StoredBinaryValue *)shortValue_rawValueBytes,
        FIFTYONE_DEGREES_PROPERTY_VALUE_TYPE_OBJECT,
        54.2);
    EXPECT_EQ(54.2, result);
}


// ============== StoredBinaryValueToBoolOrDefault ==============


TEST_F(StoredBinaryValues, StoredBinaryValue_ToBool_String_True) {
    const char rawBytes[] = { 5, 0, 't', 'r', 'u', 'e', 0 };
    const byte result = StoredBinaryValueToBoolOrDefault(
        (const StoredBinaryValue *)rawBytes,
        FIFTYONE_DEGREES_PROPERTY_VALUE_TYPE_STRING,
        false);
    EXPECT_EQ(false, result);
}

TEST_F(StoredBinaryValues, StoredBinaryValue_ToBool_String_TrueCapitalized) {
    const char rawBytes[] = { 5, 0, 'T', 'r', 'u', 'e', 0 };
    const byte result = StoredBinaryValueToBoolOrDefault(
        (const StoredBinaryValue *)rawBytes,
        FIFTYONE_DEGREES_PROPERTY_VALUE_TYPE_STRING,
        false);
    EXPECT_EQ(true, result);
}

TEST_F(StoredBinaryValues, StoredBinaryValue_ToBool_String_TrueAllCaps) {
    const char rawBytes[] = { 5, 0, 'T', 'R', 'U', 'E', 0 };
    const byte result = StoredBinaryValueToBoolOrDefault(
        (const StoredBinaryValue *)rawBytes,
        FIFTYONE_DEGREES_PROPERTY_VALUE_TYPE_STRING,
        false);
    EXPECT_EQ(false, result);
}

TEST_F(StoredBinaryValues, StoredBinaryValue_ToBool_String_TrueMixed) {
    const char rawBytes[] = { 5, 0, 't', 'R', 'U', 'e', 0 };
    const byte result = StoredBinaryValueToBoolOrDefault(
        (const StoredBinaryValue *)rawBytes,
        FIFTYONE_DEGREES_PROPERTY_VALUE_TYPE_STRING,
        false);
    EXPECT_EQ(false, result);
}

TEST_F(StoredBinaryValues, StoredBinaryValue_ToBool_String_Trueish) {
    const char rawBytes[] = { 8, 0, 't', 'r', 'u', 'e', 'i', 's', 'h', 0 };
    const byte result = StoredBinaryValueToBoolOrDefault(
        (const StoredBinaryValue *)rawBytes,
        FIFTYONE_DEGREES_PROPERTY_VALUE_TYPE_STRING,
        false);
    EXPECT_EQ(false, result);
}

TEST_F(StoredBinaryValues, StoredBinaryValue_ToBool_String_DoubleZero) {
    const char rawBytes[] = { 3, 0, '0', '0', 0 };
    const byte result = StoredBinaryValueToBoolOrDefault(
        (const StoredBinaryValue *)rawBytes,
        FIFTYONE_DEGREES_PROPERTY_VALUE_TYPE_STRING,
        false);
    EXPECT_EQ(false, result);
}

TEST_F(StoredBinaryValues, StoredBinaryValue_ToBool_String_x7) {
    const char rawBytes[] = { 3, 0, 'x', '7', 0 };
    const byte result = StoredBinaryValueToBoolOrDefault(
        (const StoredBinaryValue *)rawBytes,
        FIFTYONE_DEGREES_PROPERTY_VALUE_TYPE_STRING,
        false);
    EXPECT_EQ(false, result);
}

TEST_F(StoredBinaryValues, StoredBinaryValue_ToBool_String_7x4) {
    const char rawBytes[] = { 4, 0, '7', 'x', '4', 0 };
    const byte result = StoredBinaryValueToBoolOrDefault(
        (const StoredBinaryValue *)rawBytes,
        FIFTYONE_DEGREES_PROPERTY_VALUE_TYPE_STRING,
        false);
    EXPECT_EQ(false, result);
}

TEST_F(StoredBinaryValues, StoredBinaryValue_ToBool_String_15) {
    const char rawBytes[] = { 3, 0, '1', '5', 0 };
    const byte result = StoredBinaryValueToBoolOrDefault(
        (const StoredBinaryValue *)rawBytes,
        FIFTYONE_DEGREES_PROPERTY_VALUE_TYPE_STRING,
        false);
    EXPECT_EQ(false, result);
}

TEST_F(StoredBinaryValues, StoredBinaryValue_ToBool_String_minus827) {
    const char rawBytes[] = { 5, 0, '-', '8', '2', '7', 0 };
    const byte result = StoredBinaryValueToBoolOrDefault(
        (const StoredBinaryValue *)rawBytes,
        FIFTYONE_DEGREES_PROPERTY_VALUE_TYPE_STRING,
        false);
    EXPECT_EQ(false, result);
}

TEST_F(StoredBinaryValues, StoredBinaryValue_ToBool_String_3_95) {
    const char rawBytes[] = { 5, 0, '3', '.', '9', '5', 0 };
    const byte result = StoredBinaryValueToBoolOrDefault(
        (const StoredBinaryValue *)rawBytes,
        FIFTYONE_DEGREES_PROPERTY_VALUE_TYPE_STRING,
        false);
    EXPECT_EQ(false, result);
}

TEST_F(StoredBinaryValues, StoredBinaryValue_ToBool_Integer_0) {
    const char rawBytes[] = { 0, 0, 0, 0 };
    const byte result = StoredBinaryValueToBoolOrDefault(
        (const StoredBinaryValue *)rawBytes,
        FIFTYONE_DEGREES_PROPERTY_VALUE_TYPE_INTEGER,
        false);
    EXPECT_EQ(false, result);
}

TEST_F(StoredBinaryValues, StoredBinaryValue_ToBool_Integer_Positive) {
    const unsigned char rawBytes[] = { 0x50, 0xBE, 0x09, 0x1B };
    const byte result = StoredBinaryValueToBoolOrDefault(
        (const StoredBinaryValue *)rawBytes,
        FIFTYONE_DEGREES_PROPERTY_VALUE_TYPE_INTEGER,
        false);
    EXPECT_EQ(true, result);
}

TEST_F(StoredBinaryValues, StoredBinaryValue_ToBool_Integer_Negative) {
    const unsigned char rawBytes[] = { 0xB0, 0x41, 0xF6, 0xE4 };
    const byte result = StoredBinaryValueToBoolOrDefault(
        (const StoredBinaryValue *)rawBytes,
        FIFTYONE_DEGREES_PROPERTY_VALUE_TYPE_INTEGER,
        false);
    EXPECT_EQ(true, result);
}

TEST_F(StoredBinaryValues, StoredBinaryValue_ToBool_Float_Zero) {
    const unsigned char rawBytes[] = { 0, 0, 0, 0 }; // 0.0
    const byte result = StoredBinaryValueToBoolOrDefault(
        (const StoredBinaryValue *)rawBytes,
        FIFTYONE_DEGREES_PROPERTY_VALUE_SINGLE_PRECISION_FLOAT,
        false);
    EXPECT_EQ(false, result);
}

TEST_F(StoredBinaryValues, StoredBinaryValue_ToBool_Float_Positive) {
    const unsigned char rawBytes[] = { 0xCD, 0x5A, 0x34, 0x43 };
    const byte result = StoredBinaryValueToBoolOrDefault(
        (const StoredBinaryValue *)rawBytes,
        FIFTYONE_DEGREES_PROPERTY_VALUE_SINGLE_PRECISION_FLOAT,
        false);
    EXPECT_EQ(true, result);
}

TEST_F(StoredBinaryValues, StoredBinaryValue_ToBool_Float_LargePositive) {
    const unsigned char rawBytes[] = { 0x5F, 0x43, 0xC2, 0x47 };
    const byte result = StoredBinaryValueToBoolOrDefault(
        (const StoredBinaryValue *)rawBytes,
        FIFTYONE_DEGREES_PROPERTY_VALUE_SINGLE_PRECISION_FLOAT,
        false);
    EXPECT_EQ(true, result);
}

TEST_F(StoredBinaryValues, StoredBinaryValue_ToBool_Float_Negative) {
    const unsigned char rawBytes[] = { 0x9E, 0x86, 0x90, 0xC2 };
    const byte result = StoredBinaryValueToBoolOrDefault(
        (const StoredBinaryValue *)rawBytes,
        FIFTYONE_DEGREES_PROPERTY_VALUE_SINGLE_PRECISION_FLOAT,
        false);
    EXPECT_EQ(true, result);
}

TEST_F(StoredBinaryValues, StoredBinaryValue_ToBool_Float_LargeNegative) {
    const unsigned char rawBytes[] = { 0xA6, 0x0B, 0xA0, 0xC7 };
    const byte result = StoredBinaryValueToBoolOrDefault(
        (const StoredBinaryValue *)rawBytes,
        FIFTYONE_DEGREES_PROPERTY_VALUE_SINGLE_PRECISION_FLOAT,
        false);
    EXPECT_EQ(true, result);
}

TEST_F(StoredBinaryValues, StoredBinaryValue_ToBool_Azimuth) {
    const byte result = StoredBinaryValueToBoolOrDefault(
        (const StoredBinaryValue *)shortValue_rawValueBytes,
        FIFTYONE_DEGREES_PROPERTY_VALUE_TYPE_AZIMUTH,
        false);
    EXPECT_EQ(true, result);
}

TEST_F(StoredBinaryValues, StoredBinaryValue_ToBool_Declination) {
    const byte result = StoredBinaryValueToBoolOrDefault(
        (const StoredBinaryValue *)shortValue_rawValueBytes,
        FIFTYONE_DEGREES_PROPERTY_VALUE_TYPE_DECLINATION,
        false);
    EXPECT_EQ(true, result);
}

TEST_F(StoredBinaryValues, StoredBinaryValue_ToBool_Default_minus1) {
    const byte result = StoredBinaryValueToBoolOrDefault(
        (const StoredBinaryValue *)shortValue_rawValueBytes,
        FIFTYONE_DEGREES_PROPERTY_VALUE_TYPE_OBJECT,
        false);
    EXPECT_EQ(false, result);
}

TEST_F(StoredBinaryValues, StoredBinaryValue_ToBool_Default_54_2) {
    const byte result = StoredBinaryValueToBoolOrDefault(
        (const StoredBinaryValue *)shortValue_rawValueBytes,
        FIFTYONE_DEGREES_PROPERTY_VALUE_TYPE_OBJECT,
        true);
    EXPECT_EQ(true, result);
}
