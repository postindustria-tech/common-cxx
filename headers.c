/* *********************************************************************
 * This Original Work is copyright of 51 Degrees Mobile Experts Limited.
 * Copyright 2019 51 Degrees Mobile Experts Limited, 5 Charlotte Close,
 * Caversham, Reading, Berkshire, United Kingdom RG4 7BY.
 *
 * This Original Work is licensed under the European Union Public Licence (EUPL) 
 * v.1.2 and is subject to its terms as set out below.
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

#include "headers.h"

#include "fiftyone.h"

/* HTTP header prefix used when processing collections of parameters. */
#define HTTP_PREFIX_UPPER "HTTP_"

static bool doesHeaderExist(Headers *headers, Item *item) {
	uint32_t i;
	String *compare, *test;
	compare = (String*)(item->data.ptr);

	if (compare == NULL) {
		return false;
	}

	for (i = 0; i < headers->count; i++) {
		test = (String*)headers->items[i].name.data.ptr;
		if (test != NULL &&
			_stricmp(
				&compare->value,
				&test->value) == 0) {
			return true;
		}
	}
	return false;
}

static void addUniqueHeaders(
	Headers *headers,
	void *state,
	HeadersGetMethod get) {
	uint32_t i, uniqueId;
	Item *nameItem;
	Header *header;
	for (i = 0; i < headers->capacity; i++) {
		header = &headers->items[headers->count];
		nameItem = &header->name;
		DataReset(&nameItem->data);
		nameItem->collection = NULL;
		nameItem->handle = NULL;
		uniqueId = get(state, i, nameItem);
		if (((String*)nameItem->data.ptr)->size > 1 &&
			doesHeaderExist(headers, nameItem) == false) {
			header->uniqueId = uniqueId;
			headers->count++;
		}
		else {
			assert(nameItem->collection != NULL);
			COLLECTION_RELEASE(nameItem->collection, nameItem);
		}
	}
}

static uint32_t countHeaders(
	void *state,
	fiftyoneDegreesHeadersGetMethod get) {
	Item name;
	uint32_t count = 0;
	DataReset(&name.data);
	while (get(state, count, &name) >= 0) {
		COLLECTION_RELEASE(name.collection, &name);
		count++;
	}
	return count;
}

fiftyoneDegreesHeaders* fiftyoneDegreesHeadersCreate(
	bool expectUpperPrefixedHeaders,
	void *state,
	fiftyoneDegreesHeadersGetMethod get) {
	fiftyoneDegreesHeaders *headers;
	uint32_t count = countHeaders(state, get);
	FIFTYONE_DEGREES_ARRAY_CREATE(fiftyoneDegreesHeader, headers, count);
	if (headers != NULL) {
		headers->expectUpperPrefixedHeaders = expectUpperPrefixedHeaders;
		addUniqueHeaders(headers, state, get);
	}
	return headers;
}

int fiftyoneDegreesHeaderGetIndex(
	fiftyoneDegreesHeaders *headers,
	const char* httpHeaderName,
	size_t length) {
	uint32_t i;
	String *compare;

	// Check if header is from a Perl or PHP wrapper in the form of HTTP_*
	// and if present skip these characters.
	if (headers->expectUpperPrefixedHeaders == true &&
		length > sizeof(HTTP_PREFIX_UPPER) &&
		StringCompareLength(
			httpHeaderName,
			HTTP_PREFIX_UPPER,
			sizeof(HTTP_PREFIX_UPPER) - 1) == 0) {
		length -= sizeof(HTTP_PREFIX_UPPER) - 1;
		httpHeaderName += sizeof(HTTP_PREFIX_UPPER) - 1;
	}

	// Perform a case insensitive compare of the remaining characters.
	for (i = 0; i < headers->count; i++) {
		compare = (String*)headers->items[i].name.data.ptr;
		if ((size_t)((size_t)compare->size - 1) == length &&
			compare != NULL &&
			StringCompareLength(
				httpHeaderName, 
				&compare->value, 
				length) == 0) {
			return i;
		}
	}

	return -1;
}

fiftyoneDegreesHeader* fiftyoneDegreesHeadersGetHeaderFromUniqueId(
	fiftyoneDegreesHeaders *headers,
	uint32_t uniqueId) {
	uint32_t i;
	for (i = 0; i < headers->count; i++) {
		if (headers->items[i].uniqueId == uniqueId) {
			return headers->items + i;
		}
	}
	return (Header*)NULL;
}

void fiftyoneDegreesHeadersFree(fiftyoneDegreesHeaders *headers) {
	uint32_t i;
	if (headers != NULL) {
		for (i = 0; i < headers->count; i++) {
			COLLECTION_RELEASE(headers->items[i].name.collection,
				&headers->items[i].name);
		}
		Free(headers);
		headers = NULL;
	}
}

bool fiftyoneDegreesHeadersIsHttp(
	void *state,
	fiftyoneDegreesEvidenceKeyValuePair *pair) {
	return HeaderGetIndex(
		(Headers*)state,
		pair->field, 
		strlen(pair->field)) >= 0;
}

/**
 * SIZE CALCULATION METHODS
 */

size_t fiftyoneDegreesHeadersSize(int count) {
	return sizeof(Headers) + // Headers structure
		(count * sizeof(Header)); // Header names
}