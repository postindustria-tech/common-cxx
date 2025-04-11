/* *********************************************************************
 * This Original Work is copyright of 51 Degrees Mobile Experts Limited.
 * Copyright 2023 51 Degrees Mobile Experts Limited, Davidson House,
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

#define __STDC_FORMAT_MACROS

#include "string.h"
#include "fiftyone.h"
#include <inttypes.h>

static uint32_t getFinalByteArraySize(void *initial) {
    return (uint32_t)(sizeof(int16_t) + (*(int16_t*)initial));
}
static uint32_t getFinalFloatSize(void *initial) {
#	ifdef _MSC_VER
    UNREFERENCED_PARAMETER(initial);
#	endif
    return sizeof(fiftyoneDegreesFloat);
}
static uint32_t getFinalIntegerSize(void *initial) {
#	ifdef _MSC_VER
    UNREFERENCED_PARAMETER(initial);
#	endif
    return sizeof(int32_t);
}
static uint32_t getFinalShortSize(void *initial) {
#	ifdef _MSC_VER
    UNREFERENCED_PARAMETER(initial);
#	endif
    return sizeof(int16_t);
}

#ifndef FIFTYONE_DEGREES_MEMORY_ONLY

/**
 * Type for temporary memory keeping the value of
 * `storedValueType`: `fiftyoneDegreesPropertyValueType`
 * for "File" and/or "Partial" collections
 * between calls from `StoredBinaryValueGet` to `StoredBinaryValueRead`.
 */
typedef uint8_t PropertyValueTypeInData;

void* fiftyoneDegreesStoredBinaryValueRead(
    const fiftyoneDegreesCollectionFile * const file,
    const uint32_t offset,
    fiftyoneDegreesData * const data,
    fiftyoneDegreesException * const exception) {
    int16_t length;

    // When collection getter is called from `StoredBinaryValueRead`,
    // the latter will save `storedValueType` into item's Data.
    //
    // Otherwise -- if the data is in clear state (e.g. after DataReset),
    // the caller is assumed to have requested a "String" value.
    // (for compatibility with `StringRead`-initialized collections).

    if (data->used < sizeof(PropertyValueTypeInData)) {
        // stored value type not known,
        // => assume String
        return fiftyoneDegreesStringRead(file, offset, data, exception);
    };

    const PropertyValueType storedValueType = *(const PropertyValueTypeInData *)data->ptr;
    switch (storedValueType) {
        case FIFTYONE_DEGREES_PROPERTY_VALUE_TYPE_STRING: {
            return fiftyoneDegreesStringRead(file, offset, data, exception);
        }
        case FIFTYONE_DEGREES_PROPERTY_VALUE_TYPE_INTEGER: {
            return CollectionReadFileVariable(
                file,
                data,
                offset,
                &length,
                0,
                getFinalIntegerSize,
                exception);
        }
        case FIFTYONE_DEGREES_PROPERTY_VALUE_TYPE_AZIMUTH:
        case FIFTYONE_DEGREES_PROPERTY_VALUE_TYPE_DECLINATION: {
            return CollectionReadFileVariable(
                file,
                data,
                offset,
                &length,
                0,
                getFinalShortSize,
                exception);
        }
        case FIFTYONE_DEGREES_PROPERTY_VALUE_SINGLE_PRECISION_FLOAT: {
            return CollectionReadFileVariable(
                file,
                data,
                offset,
                &length,
                0,
                getFinalFloatSize,
                exception);
        }
        case FIFTYONE_DEGREES_PROPERTY_VALUE_TYPE_IP_ADDRESS:
        case FIFTYONE_DEGREES_PROPERTY_VALUE_TYPE_WKB_R:
        case FIFTYONE_DEGREES_PROPERTY_VALUE_TYPE_WKB: {
            return CollectionReadFileVariable(
                file,
                data,
                offset,
                &length,
                sizeof(length),
                getFinalByteArraySize,
                exception);
        }
        default: {
            EXCEPTION_SET(FIFTYONE_DEGREES_STATUS_UNSUPPORTED_STORED_VALUE_TYPE);
            return NULL;
        }
    }
}

#endif

StoredBinaryValue* fiftyoneDegreesStoredBinaryValueGet(
    fiftyoneDegreesCollection *strings,
    uint32_t offset,
    PropertyValueType storedValueType,
    fiftyoneDegreesCollectionItem *item,
    Exception *exception) {

#ifndef FIFTYONE_DEGREES_MEMORY_ONLY
    // CollectionReadFileVariable subroutine needs to know
    // - how many bytes constitute the "header" of the variable
    // - how to extract the remaining "length" of the variable from the "header"
    //
    // for that we must pass `storedValueType`

    // Allocate a memory to hold `storedValueType` on stack.
    //
    // Use an array to prevent a warning
    // > `pointer to a local variable potentially escaping scope`
    //
    // The data will either:
    // - remain unowned
    //   (and `ptr` won't be dereferenced),
    // or
    // - is already owned and considered disposable
    //   (so `storedValueType` will be copied, and no escaping will occur).

    PropertyValueTypeInData storedValueTypeInData[1] = { storedValueType };

    if (!item->data.allocated) {
        // It is assumed -- as part of the Collection-CollectionItem contract --
        // that data of the Item passed into "get" is NOT owned by that item.

        item->data.ptr = (byte *)&storedValueTypeInData[0];
    } else {
        // Since _this_ function _technically_ is NOT a "getter method"
        // we might still get an Item that owns some memory.
        //
        // Since no collection would leave Data pointing to internal memory
        // (past COLLECTION_RELEASE call -- mandatory for Item to be reused)
        // assume the Data is disposable.
        //
        // Ensure Data is of sufficient size and copy `storedValueType` into it.

        DataMalloc(&item->data, sizeof(PropertyValueTypeInData));
        *((PropertyValueTypeInData*)item->data.ptr) = storedValueTypeInData[0];
    }
    item->data.used = sizeof(PropertyValueTypeInData);

#else
    // In MEMORY_ONLY mode,
    //
    // we only need to get the pointer to beginning of the data structure
    // inside the whole body of the data file.
    //
    // `storedValueType` is not used, since we do not need to allocate
    // a sufficient (unknown before reading starts) amount of memory
    // to read the bytes into.
#	ifdef _MSC_VER
    UNREFERENCED_PARAMETER(storedValueType);
#	endif
#endif

    StoredBinaryValue * const result = strings->get(
        strings,
        offset,
        item,
        exception);
    return result;
}

/**
 * Function to compare the current String item to the
 * target search value using the IpAddress format.
 * @param value the current String item
 * @param target the target search value. This should
 * be in string readable format of an IP address.
 * @return 0 if they are equal, otherwise negative
 * for smaller and positive for bigger
 */
static int compareIpAddressToString(const VarLengthByteArray * const value, const char * const target) {
    int result = 0;
    IpAddress ipAddress;
    bool parsed = IpAddressParse(
            target,
            target + strlen(target),
            &ipAddress);
    if (parsed) {
        const int16_t valueLength = value->size;
        int16_t searchLength = 0;
        switch (ipAddress.type) {
            case IP_TYPE_IPV4:
                searchLength = IPV4_LENGTH;
            break;
            case IP_TYPE_IPV6:
                searchLength = IPV6_LENGTH;
            break;
            case IP_TYPE_INVALID:
                default:
                    break;
        }

        if (searchLength == 0) {
            result = valueLength;
        }
        else {
            // Compare length first
            const size_t compareLength = ((valueLength < searchLength)
                ? valueLength : searchLength);
            result = memcmp(&value->firstByte,
                ipAddress.value, compareLength);
            if (result == 0) {
                result = valueLength - searchLength;
            }
        }
    }
    return result;
}

static int compareWkbToString(
    const VarLengthByteArray * const binaryValue,
    const char * const target,
    Exception * const exception) {
    WkbtotResult toWktResult = {
        0,
        false,
    };

    if (!binaryValue || !exception) {
        EXCEPTION_SET(FIFTYONE_DEGREES_STATUS_NULL_POINTER);
        return -1;
    }

    int result = -1;

    {
        char buffer[REASONABLE_WKT_STRING_LENGTH];
        StringBuilder builder = { buffer, REASONABLE_WKT_STRING_LENGTH };
        StringBuilderInit(&builder);
        WriteWkbAsWktToStringBuilder(
            &binaryValue->firstByte,
            FIFTYONE_DEGREES_WKBToT_REDUCTION_NONE,
            MAX_DOUBLE_DECIMAL_PLACES,
            &builder,
            exception
            );
        StringBuilderComplete(&builder);
        toWktResult = (WkbtotResult){
            builder.added,
            builder.full,
        };
        if (EXCEPTION_OKAY && !toWktResult.bufferTooSmall) {
            result = strcmp(buffer, target);
        }
    }
    if (toWktResult.bufferTooSmall) {
        EXCEPTION_CLEAR;
        const size_t requiredSize = toWktResult.written + 1;
        char * const buffer = Malloc(requiredSize);
        StringBuilder builder = { buffer, requiredSize };
        StringBuilderInit(&builder);
        WriteWkbAsWktToStringBuilder(
            &binaryValue->firstByte,
            FIFTYONE_DEGREES_WKBToT_REDUCTION_NONE,
            MAX_DOUBLE_DECIMAL_PLACES,
            &builder,
            exception
            );
        StringBuilderComplete(&builder);
        toWktResult = (WkbtotResult){
            builder.added,
            builder.full,
        };
        if (EXCEPTION_OKAY && !toWktResult.bufferTooSmall) {
            result = strcmp(buffer, target);
        }
    }
    return result;
}

static double shortToDouble(const StoredBinaryValue * const value, const double maxAngle) {
    return (value->shortValue * maxAngle) / INT16_MAX;
}
static double toAzimuth(const StoredBinaryValue * const value) {
    return shortToDouble(value, 180);
}
static double toDeclination(const StoredBinaryValue * const value) {
    return shortToDouble(value, 90);
}
static double toDoubleFromFloat(const StoredBinaryValue * const value) {
    return FLOAT_TO_NATIVE(value->floatValue);
}

static int compareDoubleToString(
    const double testedValue,
    const char * const target) {

    const double searchValue = (float)strtod(target, NULL);
    if (errno == ERANGE)
    {
        errno = 0;
        return -1;
    }
    const double dv = testedValue - searchValue;
    return !dv ? 0 : (dv < 0 ? -1 : 1);
}

int fiftyoneDegreesStoredBinaryValueCompareWithString(
    const StoredBinaryValue * const value,
    const PropertyValueType storedValueType,
    const char * const target,
    Exception * const exception) {

    switch (storedValueType) {
        case FIFTYONE_DEGREES_PROPERTY_VALUE_TYPE_STRING: {
            const size_t targetLength = strlen(target);
            const size_t valueLength = value->stringValue.size - 1;
            const size_t cmpLength = (targetLength < valueLength) ? targetLength : valueLength;
            const int result = strncmp(&value->stringValue.value, target, cmpLength);
            if (result) {
                return result;
            }
            if (cmpLength < targetLength) {
                return -1;
            }
            return 0;
        }
        case FIFTYONE_DEGREES_PROPERTY_VALUE_TYPE_IP_ADDRESS: {
            return compareIpAddressToString(&value->byteArrayValue, target);
        }
        case FIFTYONE_DEGREES_PROPERTY_VALUE_TYPE_WKB: {
            return compareWkbToString(&value->byteArrayValue, target, exception);
        }
        case FIFTYONE_DEGREES_PROPERTY_VALUE_TYPE_INTEGER: {
            const int32_t intValue = value->intValue;
            const int32_t searchInt = strtol(target, NULL, 10);
            if (errno == ERANGE)
            {
                errno = 0;
                return -1;
            }
            return intValue - searchInt;
        }
        case FIFTYONE_DEGREES_PROPERTY_VALUE_SINGLE_PRECISION_FLOAT: {
            return compareDoubleToString(toDoubleFromFloat(value), target);
        }
        case FIFTYONE_DEGREES_PROPERTY_VALUE_TYPE_AZIMUTH: {
            return compareDoubleToString(toAzimuth(value), target);
        }
        case FIFTYONE_DEGREES_PROPERTY_VALUE_TYPE_DECLINATION: {
            return compareDoubleToString(toDeclination(value), target);
        }
        default: {
            EXCEPTION_SET(UNSUPPORTED_STORED_VALUE_TYPE);
            return -1;
        }
    }
}


int fiftyoneDegreesStoredBinaryValueToIntOrDefault(
    const fiftyoneDegreesStoredBinaryValue * const value,
    const fiftyoneDegreesPropertyValueType storedValueType,
    const int defaultValue) {

    switch (storedValueType) {
        case FIFTYONE_DEGREES_PROPERTY_VALUE_TYPE_STRING: {
            return atoi(&value->stringValue.value);
        }
        case FIFTYONE_DEGREES_PROPERTY_VALUE_TYPE_INTEGER: {
            return value->intValue;
        }
        case FIFTYONE_DEGREES_PROPERTY_VALUE_SINGLE_PRECISION_FLOAT: {
            return (int)FLOAT_TO_NATIVE(value->floatValue);
        }
        case FIFTYONE_DEGREES_PROPERTY_VALUE_TYPE_AZIMUTH: {
            return (int)toAzimuth(value);
        }
        case FIFTYONE_DEGREES_PROPERTY_VALUE_TYPE_DECLINATION: {
            return (int)toDeclination(value);
        }
        default: {
            return defaultValue;
        }
    }
}

double fiftyoneDegreesStoredBinaryValueToDoubleOrDefault(
    const fiftyoneDegreesStoredBinaryValue * const value,
    const fiftyoneDegreesPropertyValueType storedValueType,
    const double defaultValue) {

    switch (storedValueType) {
        case FIFTYONE_DEGREES_PROPERTY_VALUE_TYPE_STRING: {
            return strtod(&value->stringValue.value, NULL);
        }
        case FIFTYONE_DEGREES_PROPERTY_VALUE_TYPE_INTEGER: {
            return value->intValue;
        }
        case FIFTYONE_DEGREES_PROPERTY_VALUE_SINGLE_PRECISION_FLOAT: {
            return FLOAT_TO_NATIVE(value->floatValue);
        }
        case FIFTYONE_DEGREES_PROPERTY_VALUE_TYPE_AZIMUTH: {
            return toAzimuth(value);
        }
        case FIFTYONE_DEGREES_PROPERTY_VALUE_TYPE_DECLINATION: {
            return toDeclination(value);
        }
        default: {
            return defaultValue;
        }
    }
}

bool fiftyoneDegreesStoredBinaryValueToBoolOrDefault(
    const fiftyoneDegreesStoredBinaryValue * const value,
    const fiftyoneDegreesPropertyValueType storedValueType,
    const bool defaultValue) {

    switch (storedValueType) {
        case FIFTYONE_DEGREES_PROPERTY_VALUE_TYPE_STRING: {
            if (value->stringValue.size != 5) {
                return false;
            }
            return !strncmp(&value->stringValue.value, "True", 4);
        }
        case FIFTYONE_DEGREES_PROPERTY_VALUE_TYPE_INTEGER: {
            return value->intValue ? true : false;
        }
        case FIFTYONE_DEGREES_PROPERTY_VALUE_SINGLE_PRECISION_FLOAT: {
            return FLOAT_TO_NATIVE(value->floatValue) ? true : false;
        }
        case FIFTYONE_DEGREES_PROPERTY_VALUE_TYPE_AZIMUTH: {
            return toAzimuth(value) ? true : false;
        }
        case FIFTYONE_DEGREES_PROPERTY_VALUE_TYPE_DECLINATION: {
            return toDeclination(value) ? true : false;
        }
        default: {
            return defaultValue;
        }
    }
}
