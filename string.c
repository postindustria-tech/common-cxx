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

static uint32_t getFinalStringSize(void *initial) {
	return (uint32_t)(sizeof(int16_t) + (*(int16_t*)initial));
}

#ifndef FIFTYONE_DEGREES_MEMORY_ONLY

void* fiftyoneDegreesStringRead(
	const fiftyoneDegreesCollectionFile *file,
	uint32_t offset,
	fiftyoneDegreesData *data,
	fiftyoneDegreesException *exception) {
	int16_t length;
	return CollectionReadFileVariable(
		file, 
		data, 
		offset,
		&length, 
		sizeof(int16_t),
		getFinalStringSize,
		exception);
}

#endif

fiftyoneDegreesString* fiftyoneDegreesStringGet(
	fiftyoneDegreesCollection *strings,
	uint32_t offset, 
	fiftyoneDegreesCollectionItem *item,
	fiftyoneDegreesException *exception) {
	return (String*)strings->get(
		strings,
		offset,
		item,
		exception);
}

int fiftyoneDegreesStringCompare(const char *a, const char *b) {
	for (; *a != '\0' && *b != '\0'; a++, b++) {
		int d = tolower(*a) - tolower(*b);
		if (d != 0) {
			return d;
		}
	}
	if (*a == '\0' && *b != '\0') return -1;
	if (*a != '\0' && *b == '\0') return 1;
	assert(*a == '\0' && *b == '\0');
	return 0;
}

int fiftyoneDegreesStringCompareLength(
	char const *a, 
	char const *b, 
	size_t length) {
	size_t i;
	for (i = 0; i < length; a++, b++, i++) {
		int d = tolower(*a) - tolower(*b);
		if (d != 0) {
			return d;
		}
	}
	return 0;
}

const char *fiftyoneDegreesStringSubString(const char *a, const char *b) {
	int d;
	const char *a1, *b1;
	for (; *a != '\0' && *b != '\0'; a++) {
		d = tolower(*a) - tolower(*b);
		if (d == 0) {
			a1 = a + 1;
			b1 = b + 1;
			for (; *a1 != '\0' && *b1 != '\0'; a1++, b1++) {
				d = tolower(*a1) - tolower(*b1);
				if (d != 0) {
					break;
				}
			}
			if (d == 0 && *b1 == '\0') {
				return (char *)a;
			}
		}
	}
	return NULL;
}

/**
 * Add IPv4 address (raw bytes) to string builder (as text)
 * @param ipAddress raw bytes of IPv4
 * @param stringBuilder destination
 */
static void getIpv4RangeString(
	const unsigned char ipAddress[FIFTYONE_DEGREES_IPV4_LENGTH],
	StringBuilder * const stringBuilder) {
	StringBuilderAddInteger(stringBuilder, ipAddress[0]);
	StringBuilderAddChar(stringBuilder, '.');
	StringBuilderAddInteger(stringBuilder, ipAddress[1]);
	StringBuilderAddChar(stringBuilder, '.');
	StringBuilderAddInteger(stringBuilder, ipAddress[2]);
	StringBuilderAddChar(stringBuilder, '.');
	StringBuilderAddInteger(stringBuilder, ipAddress[3]);
}

/**
 * Add IPv6 address (raw bytes) to string builder (as text)
 * @param ipAddress raw bytes of IPv6
 * @param stringBuilder destination
 */
static void getIpv6RangeString(
	const unsigned char ipAddress[FIFTYONE_DEGREES_IPV6_LENGTH],
	StringBuilder * const stringBuilder) {
	const char separator = ':';
	const char *hex = "0123456789abcdef";
	for (int i = 0; i < FIFTYONE_DEGREES_IPV6_LENGTH; i += 2) {
		for (int j = 0; j < 2; j++) {
			StringBuilderAddChar(stringBuilder, hex[(((int)ipAddress[i + j]) >> 4) & 0x0F]);
			StringBuilderAddChar(stringBuilder, hex[((int)ipAddress[i + j]) & 0x0F]);
		}
		if (i != FIFTYONE_DEGREES_IPV6_LENGTH - 2) {
			StringBuilderAddChar(stringBuilder, separator);
		}
	}
}

void fiftyoneDegreesStringBuilderAddIpAddress(
	StringBuilder * const stringBuilder,
	const String * const ipAddress,
	const IpType type,
	Exception * const exception) {
	int32_t ipLength =
		type == IP_TYPE_IPV4 ?
		FIFTYONE_DEGREES_IPV4_LENGTH :
		FIFTYONE_DEGREES_IPV6_LENGTH;
	// Get the actual length of the byte array
	int32_t actualLength = ipAddress->size - 1;

	// Make sure the ipAddress item and everything is in correct
	// format
	if (ipAddress->value == FIFTYONE_DEGREES_STRING_IP_ADDRESS
		&& ipLength == actualLength
		&& type != IP_TYPE_INVALID) {

		if (type == IP_TYPE_IPV4) {
			getIpv4RangeString(
				(unsigned char *)&ipAddress->trail.secondValue,
				stringBuilder);
		}
		else {
			getIpv6RangeString(
				(unsigned char *)&ipAddress->trail.secondValue,
				stringBuilder);
		}
		}
	else {
		EXCEPTION_SET(INCORRECT_IP_ADDRESS_FORMAT);
	}
}

fiftyoneDegreesStringBuilder* fiftyoneDegreesStringBuilderInit(
	fiftyoneDegreesStringBuilder* builder) {
	builder->current = builder->ptr;
	builder->remaining = builder->length;
	builder->added = 0;
	builder->full = false;
	return builder;
}

fiftyoneDegreesStringBuilder* fiftyoneDegreesStringBuilderAddChar(
	fiftyoneDegreesStringBuilder* builder,
	char const value) {
	if (builder->remaining > 1) {
		*builder->current = value;
		builder->current++;
		builder->remaining--;
	}
	else {
		builder->full = true;
	}
	builder->added++;
	return builder;
}

fiftyoneDegreesStringBuilder* fiftyoneDegreesStringBuilderAddInteger(
	fiftyoneDegreesStringBuilder* builder,
	int64_t const value) {
    // 64-bit INT_MIN is  -9,223,372,036,854,775,807 => 21 characters
	char temp[22];
	if (snprintf(temp, sizeof(temp), "%" PRId64, value) > 0) {
		StringBuilderAddChars(
			builder,
			temp,
			strlen(temp));
	}
	return builder;
}

/**
 * Max. number of decimal places to be printed for a double.
 */
#define MAX_DOUBLE_DECIMAL_PLACES 15

StringBuilder* fiftyoneDegreesStringBuilderAddDouble(
	fiftyoneDegreesStringBuilder * const builder,
	const double value,
	const uint8_t decimalPlaces) {

	const int digitPlaces = MAX_DOUBLE_DECIMAL_PLACES < decimalPlaces
		? MAX_DOUBLE_DECIMAL_PLACES : decimalPlaces;
	int remDigits = digitPlaces;

	int intPart = (int)value;
	double fracPart = value - intPart;

	if (fracPart < 0) {
		fracPart = -fracPart;
	}
	if (remDigits <= 0 && fracPart >= 0.5) {
		intPart++;
	}

	if (!fracPart || remDigits <= 0) {
		if (fracPart >= 0.5) {
			intPart++;
		}
		StringBuilderAddInteger(builder, intPart);
		return builder;
	}

	char floatTail[MAX_DOUBLE_DECIMAL_PLACES + 2];
	floatTail[0] = '.';
	char *digits = floatTail + 1;

	char *nextDigit = digits;
	while (remDigits > 0 && fracPart) {
		remDigits--;
		fracPart *= 10;
		*nextDigit = (char)fracPart;
		fracPart -= *nextDigit;
		if (!remDigits && fracPart >= 0.5) {
			// find last non-9
			while (nextDigit > floatTail && *nextDigit == 9) {
				--nextDigit;
				++remDigits;
			}
			if (nextDigit > floatTail) {
				(*nextDigit)++;
				++nextDigit;
				break;
			} else {
				// tail collapsed into 1
				StringBuilderAddInteger(builder, ++intPart);
				return builder;
			}
		}
		++nextDigit;
	}
	*nextDigit = '\0';

	int digitsToAdd = digitPlaces - remDigits;
	for (nextDigit = digits + digitsToAdd - 1;
		nextDigit >= digits;
		--nextDigit) {

		if (*nextDigit) {
			break;
		}
		--digitsToAdd;
		}
	if (digitsToAdd <= 0) {
		return builder;
	}
	for (; nextDigit >= digits; --nextDigit) {
		*nextDigit += '0';
	}

	StringBuilderAddInteger(builder, intPart);
	StringBuilderAddChars(builder, floatTail, digitsToAdd + 1);
	return builder;
}

#undef MAX_DOUBLE_DECIMAL_PLACES

fiftyoneDegreesStringBuilder* fiftyoneDegreesStringBuilderAddChars(
	fiftyoneDegreesStringBuilder* builder,
	const char * const value,
	size_t const length) {
	if (length < builder->remaining &&
		memcpy(builder->current, value, length) == builder->current) {
		builder->remaining -= length;
		builder->current += length;
	}
	else {
		builder->full = true;
	}
	builder->added += length;
	return builder;
}

StringBuilder* fiftyoneDegreesStringBuilderAddStringValue(
	fiftyoneDegreesStringBuilder* builder,
	const fiftyoneDegreesString* value,
	uint8_t decimalPlaces,
	fiftyoneDegreesException *exception) {

	switch (value->value) {
		case FIFTYONE_DEGREES_STRING_COORDINATE: {
			StringBuilderAddDouble(
				builder,
				FLOAT_TO_NATIVE(value->trail.coordinate.lat),
				decimalPlaces);
			StringBuilderAddChar(builder, ',');
			StringBuilderAddDouble(
				builder,
				FLOAT_TO_NATIVE(value->trail.coordinate.lon),
				decimalPlaces);
			break;
		}
		case FIFTYONE_DEGREES_STRING_IP_ADDRESS: {
			// Get the actual address size
			const uint16_t addressSize = value->size - 1;
			// Get the type of the IP address
			fiftyoneDegreesIpType type;
			switch (addressSize) {
				case FIFTYONE_DEGREES_IPV4_LENGTH: {
					type = IP_TYPE_IPV4;
					break;
				}
				case FIFTYONE_DEGREES_IPV6_LENGTH: {
					type = IP_TYPE_IPV6;
					break;
				}
				default: {
					type = IP_TYPE_INVALID;
					break;
				}
			}
			// Get the string representation of the IP address
			StringBuilderAddIpAddress(
				builder,
				value,
				type,
				exception);
			break;
		}
		case FIFTYONE_DEGREES_STRING_WKB: {
			fiftyoneDegreesWriteWkbAsWktToStringBuilder(
				(const unsigned char *)&(value->trail.secondValue),
				decimalPlaces,
				builder,
				exception);
			break;
		}
		default: {
			// discard NUL-terminator
			if (value->size > 1) {
				StringBuilderAddChars(
					builder,
					&(value->value),
					value->size - 1);
			}
			break;
		}
	}

	return builder;
}

fiftyoneDegreesStringBuilder* fiftyoneDegreesStringBuilderComplete(
	fiftyoneDegreesStringBuilder* builder) {

	// Always ensures that the string is null terminated even if that means 
	// overwriting the last character to turn it into a null.
	if (builder->remaining >= 1) {
		*builder->current = '\0';
		builder->current++;
		builder->remaining--;
		builder->added++;
	}
	else {
        if (builder->ptr && builder->length > 0) {
            *(builder->ptr + builder->length - 1) = '\0';
        }
		builder->full = true;
	}
	return builder;
}
