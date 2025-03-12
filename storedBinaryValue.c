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

void* fiftyoneDegreesStoredBinaryValueRead(
    const fiftyoneDegreesCollectionFile * const file,
    const uint32_t offset,
    fiftyoneDegreesData * const data,
    fiftyoneDegreesException * const exception) {
    int16_t length;
    if (!data->ptr || !data->used) {
        // stored value type not known
        return fiftyoneDegreesStringRead(file, offset, data, exception);
    }
    const PropertyValueType storedValueType = *data->ptr;
    switch (storedValueType) {
        case FIFTYONE_DEGREES_PROPERTY_VALUE_TYPE_JAVASCRIPT:
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

    DataMalloc(&item->data, sizeof(uint32_t));
    *((uint32_t*)item->data.ptr) = storedValueType;
    item->data.used = sizeof(uint32_t);

    StoredBinaryValue * const result = strings->get(
        strings,
        offset,
        item,
        exception);
    if (EXCEPTION_FAILED || !result) {
        Free(item->data.ptr);
        DataReset(&item->data);
    }
    return result;
}
