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

#ifndef FIFTYONE_DEGREES_MEMORY_ONLY

typedef uint8_t PropertyValueTypeInData;

void* fiftyoneDegreesStoredBinaryValueRead(
    const fiftyoneDegreesCollectionFile * const file,
    const uint32_t offset,
    fiftyoneDegreesData * const data,
    fiftyoneDegreesException * const exception) {
    int16_t length;
    if (data->used < sizeof(PropertyValueTypeInData)) {
        // stored value type not known
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

    PropertyValueTypeInData storedValueTypeInData[1] = { storedValueType };
    if (item->data.allocated) {
        DataMalloc(&item->data, sizeof(uint32_t));
        *((PropertyValueTypeInData*)item->data.ptr) = storedValueTypeInData[0];
    } else {
        item->data.ptr = (byte *)&storedValueTypeInData[0];
    }
    item->data.used = sizeof(PropertyValueTypeInData);

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
            const float floatValue = fiftyoneDegreesFloatToNative(
                *(fiftyoneDegreesFloatInternal *)&value->floatValue);
            const float searchFloat = (float)strtod(target, NULL);
            if (errno == ERANGE)
            {
                errno = 0;
                return -1;
            }
            const float dv = floatValue - searchFloat;
            return !dv ? 0 : (dv < 0 ? -1 : 1);
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
            return value->intValue;
        }
        case FIFTYONE_DEGREES_PROPERTY_VALUE_SINGLE_PRECISION_FLOAT: {
            return FLOAT_TO_NATIVE(value->floatValue);
        }
        default: {
            return defaultValue;
        }
    }
}
