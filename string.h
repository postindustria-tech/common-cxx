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

#ifndef FIFTYONE_DEGREES_STRING_H_INCLUDED
#define FIFTYONE_DEGREES_STRING_H_INCLUDED

/**
 * @ingroup FiftyOneDegreesCommon
 * @defgroup FiftyOneDegreesString String
 *
 * String structures containing the string and length.
 *
 * ## Introduction
 *
 * The String structure allows a string and its length to be stored in one
 * structure. This avoids unnecessary calls to strlen. Both the string and its
 * length are allocated in a single operation, so the size of the actual
 * structure (when including the string terminator) is
 * sizeof(#fiftyoneDegreesString) + length. This means that the string itself
 * starts at "value" and continues into the rest of the allocated memory.
 *
 * ## Get
 *
 * Getting a const char * from a #fiftyoneDegreesString structure can be done
 * by casting a reference to the "value" field:
 * ```
 * (const char*)&string->value
 * ```
 * However, this can be simplified by using the #FIFTYONE_DEGREES_STRING macro
 * which also performs a NULL check on the structure to avoid a segmentation
 * fault.
 *
 * ## Compare
 *
 * This file contains two case insensitive string comparison methods as
 * standards like `stricmp` vary across compilers.
 *
 * **fiftyoneDegreesStringCompare** : compares two strings case insensitively
 *
 * **fiftyoneDegreesStringCompareLength** : compares two strings case
 * insensitively up to the length required. Any characters after this point are
 * ignored
 *
 * @{
 */

#include <stdint.h>
#include <ctype.h>
#include "exceptions.h"
#include "collection.h"
#include "float.h"
#include "common.h"
#include "ip.h"
#include "propertyValueType.h"

/**
 * Macro used to check for NULL before returning the string as a const char *.
 * @param s pointer to the #fiftyoneDegreesString
 * @return const char * string or NULL
 */
#define FIFTYONE_DEGREES_STRING(s) \
	(const char*)(s == NULL ? NULL : &((fiftyoneDegreesString*)s)->value)

/** 
 * String structure containing its value and size which maps to the string 
 * byte format used in data files.
 *
 * @example
 * String:
 * 			Short – length – 10
 * 			Byte value – first character of string – '5'
 */
#pragma pack(push, 1)
typedef struct fiftyone_degrees_string_t {
	int16_t size; /**< Size of the string in memory (starting from 'value') */
	char value; /**< The first character of the string */
} fiftyoneDegreesString;
#pragma pack(pop)

/**
 * Structure containing raw bytes and size from data files.
 *
 * @example
 * String:
 * 			Short – length – 10
 * 			Byte value – first character of string – '5'
 * @example
 * Byte array:
 * 			Short – length – 3
 * 			Byte[] – bytes – [ 1, 2 ]
 * @example
 * IP (v4) address:
 * 			Short – length – 5
 * 			Byte[] – IP – [ 1, 2, 3, 4 ]
 * @example
 * WKB (value of  POINT(2.0 4.0)):
 * 			Short – length - 21
 * 			Byte[] – value – [
 * 				0 (endianness),
 * 				0, 0, 0, 1 (2D point),
 * 				128, 0, 0, 0, 0, 0, 0, 0 (2.0 float),
 * 				128, 16, 0, 0, 0, 0, 0, 0 (4.0 float)
 * 			]
 */
#pragma pack(push, 1)
typedef struct fiftyone_degrees_var_length_byte_array_t {
	int16_t size; /**< Size of the byte array in memory (starting from 'firstByte') */
	unsigned char firstByte; /**< The first byte of the array */
} fiftyoneDegreesVarLengthByteArray;
#pragma pack(pop)

/**
 * "Packed" value that can be present inside "strings" of dataset.
 */
#pragma pack(push, 1)
typedef union fiftyone_degrees_stored_binary_value_t {
	fiftyoneDegreesString stringValue; /**< String value (ASCII or UTF-8) */
	fiftyoneDegreesVarLengthByteArray byteArrayValue; /**< Byte array value (e.g. IP or WKB) */
	fiftyoneDegreesFloat floatValue; /**< single precision floating point value */
	int32_t intValue; /**< Integer value */
} fiftyoneDegreesStoredBinaryValue;
#pragma pack(pop)

/** String buffer for building strings with memory checks */
typedef struct fiftyone_degrees_string_builder_t {
	char* const ptr; /**< Pointer to the memory used by the buffer */
	size_t const length; /**< Length of buffer */
	char* current; /**</ Current position to add characters in the buffer */
	size_t remaining; /**< Remaining characters in the buffer */
	size_t added; /**< Characters added to the buffer or that would be 
					  added if the buffer were long enough */
	bool full; /**< True if the buffer is full, otherwise false */
} fiftyoneDegreesStringBuilder;

#ifndef FIFTYONE_DEGREES_MEMORY_ONLY

/**
 * Reads a binary value from the source file at the offset within the string
 * structure.
 * @param file collection to read from
 * @param offset of the string in the collection
 * @param data to store the new string in
 * @param exception pointer to an exception data structure to be used if an
 * exception occurs. See exceptions.h.
 * @return a pointer to the string collection item or NULL if can't be found
 * @note expects `data` to contain `fiftyoneDegreesPropertyValueType`
 * matching the stored value type of the property this value belongs to.
 */
EXTERNAL void* fiftyoneDegreesStoredBinaryValueRead(
	const fiftyoneDegreesCollectionFile *file,
	uint32_t offset,
	fiftyoneDegreesData *data,
	fiftyoneDegreesException *exception);

// /**
//  * Reads a string from the source file at the offset within the string
//  * structure.
//  * @param file collection to read from
//  * @param offset of the string in the collection
//  * @param data to store the new string in
//  * @param exception pointer to an exception data structure to be used if an
//  * exception occurs. See exceptions.h.
//  * @return a pointer to the string collection item or NULL if can't be found
//  */
// EXTERNAL void* fiftyoneDegreesStringRead(
// 	const fiftyoneDegreesCollectionFile *file,
// 	uint32_t offset,
// 	fiftyoneDegreesData *data,
// 	fiftyoneDegreesException *exception);

#endif

/**
 * Gets the binary value at the required offset from the collection provided.
 * @param strings collection to get the string from
 * @param offset of the string in the collection
 * @param storedValueType format of byte array representation
 * @param item to store the string in
 * @param exception pointer to an exception data structure to be used if an
 * exception occurs. See exceptions.h.
 * @return a pointer to binary value or NULL if the offset is not valid
 */
EXTERNAL fiftyoneDegreesStoredBinaryValue* fiftyoneDegreesStoredBinaryValueGet(
	fiftyoneDegreesCollection *strings,
	uint32_t offset,
	fiftyoneDegreesPropertyValueType storedValueType,
	fiftyoneDegreesCollectionItem *item,
	fiftyoneDegreesException *exception);

// /**
//  * Gets the string at the required offset from the collection provided.
//  * @param strings collection to get the string from
//  * @param offset of the string in the collection
//  * @param item to store the string in
//  * @param exception pointer to an exception data structure to be used if an
//  * exception occurs. See exceptions.h.
//  * @return a pointer to string of NULL if the offset is not valid
//  */
// EXTERNAL fiftyoneDegreesString* fiftyoneDegreesStringGet(
// 	fiftyoneDegreesCollection *strings,
// 	uint32_t offset,
// 	fiftyoneDegreesCollectionItem *item,
// 	fiftyoneDegreesException *exception);

/**
 * Case insensitively compare two strings up to the length requested.
 * @param a string to compare
 * @param b other string to compare
 * @param length of the strings to compare
 * @return 0 if same
 */
EXTERNAL int fiftyoneDegreesStringCompareLength(
	char const *a, 
	char const *b, 
	size_t length);

/**
 * Case insensitively compare two strings.
 * @param a string to compare
 * @param b other string to compare
 * @return 0 if same
 */
EXTERNAL int fiftyoneDegreesStringCompare(const char *a, const char *b);

/**
 * Case insensitively searching a first occurrence of a
 * substring.
 * @param a string to search
 * @param b substring to be searched for
 * @return pointer to the first occurrence or NULL if not found
 */
EXTERNAL const char *fiftyoneDegreesStringSubString(const char *a, const char *b);

/**
 * Initializes the buffer.
 * @param builder to initialize
 * @return pointer to the builder passed
 */
EXTERNAL fiftyoneDegreesStringBuilder* fiftyoneDegreesStringBuilderInit(
	fiftyoneDegreesStringBuilder* builder);

/**
 * Adds the character to the buffer.
 * @param builder to add the character to
 * @param value character to add
 * @return pointer to the builder passed
 */
EXTERNAL fiftyoneDegreesStringBuilder* fiftyoneDegreesStringBuilderAddChar(
	fiftyoneDegreesStringBuilder* builder,
	char const value);

/**
 * Adds the integer to the buffer.
 * @param builder to add the character to
 * @param value integer to add
 * @return pointer to the buffer passed
 */
EXTERNAL fiftyoneDegreesStringBuilder* fiftyoneDegreesStringBuilderAddInteger(
	fiftyoneDegreesStringBuilder* builder,
	int64_t const value);

/**
 * Adds the double to the buffer.
 * @param builder to add the character to
 * @param value floating-point number to add
 * @param decimalPlaces precision (places after decimal dot)
 * @return pointer to the buffer passed
 */
EXTERNAL fiftyoneDegreesStringBuilder* fiftyoneDegreesStringBuilderAddDouble(
	fiftyoneDegreesStringBuilder* builder,
	double value,
	uint8_t decimalPlaces);

/**
 * Adds the string to the buffer.
 * @param builder to add the character to
 * @param value of chars to add
 * @param length of chars to add
 * @return pointer to the buffer passed
 */
EXTERNAL fiftyoneDegreesStringBuilder* fiftyoneDegreesStringBuilderAddChars(
	fiftyoneDegreesStringBuilder* builder,
	const char* value,
	size_t length);

/**
 * Adds an the IP (as string) from byte "string".
 * @param builder to add the IP to
 * @param ipAddress binary (packed) "string" with IP to add
 * @param type type of IP inside
 * @param exception pointer to exception struct
 */
EXTERNAL void fiftyoneDegreesStringBuilderAddIpAddress(
	fiftyoneDegreesStringBuilder* builder,
	const fiftyoneDegreesVarLengthByteArray *ipAddress,
	fiftyoneDegreesIpType type,
	fiftyoneDegreesException *exception);

/**
 * Adds a potentially packed value as a proper string to the buffer.
 * @param builder to add the character to
 * @param value from data file to add
 * @param decimalPlaces precision for numbers (places after decimal dot)
 * @param exception pointer to exception struct
 * @return pointer to the buffer passed
 */
EXTERNAL fiftyoneDegreesStringBuilder* fiftyoneDegreesStringBuilderAddStringValue(
	fiftyoneDegreesStringBuilder* builder,
	const fiftyoneDegreesStoredBinaryValue* value,
	fiftyoneDegreesPropertyValueType valueType,
	uint8_t decimalPlaces,
	fiftyoneDegreesException *exception);

/**
 * Adds a null terminating character to the buffer.
 * @param builder to terminate
 * @return pointer to the buffer passed
 */
EXTERNAL fiftyoneDegreesStringBuilder* fiftyoneDegreesStringBuilderComplete(
	fiftyoneDegreesStringBuilder* builder);

/**
 * @}
 */

#endif
