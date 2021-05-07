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

/**
 * Check if a header is a pseudo header.
 */
bool fiftyoneDegreesHeadersIsPseudo(const char *headerName) {
	return strchr(headerName, PSEUDO_HEADER_SEP) != NULL;
}

/**
 * This also construct the list of pseudo-headers indices.
 */
static void addUniqueHeaders(
	Headers *headers,
	void *state,
	HeadersGetMethod get) {
	uint32_t i, pIndex, uniqueId;
	Item *nameItem;
	Header *header;
	for (i = 0, pIndex = 0; i < headers->capacity; i++) {
		header = &headers->items[headers->count];
		nameItem = &header->name;
		DataReset(&nameItem->data);
		nameItem->collection = NULL;
		nameItem->handle = NULL;
		uniqueId = get(state, i, nameItem);
		if (((String*)nameItem->data.ptr)->size > 1 &&
			doesHeaderExist(headers, nameItem) == false) {
			header->uniqueId = uniqueId;
			// Check if header is pseudo header then add it to the list
			if (HeadersIsPseudo(
				STRING(nameItem->data.ptr))) {
				headers->pseudoHeaders[pIndex++] = headers->count;
			}
			else {
				header->requestHeaders = NULL;
			}
			header->requestHeaderCount = 0;
			headers->count++;
		}
		else {
			assert(nameItem->collection != NULL);
			COLLECTION_RELEASE(nameItem->collection, nameItem);
		}
	}
}

static uint32_t countRequestHeaders(const char* pseudoHeaders) {
	uint32_t count;
	const char* tmp = pseudoHeaders;
	// Count start from 1 as there be at list one headr name
	for (count = 1;
		(tmp = strchr(tmp, PSEUDO_HEADER_SEP)) != NULL;
		tmp++, count++) {}
	return count;
}

static void freePseudoHeaders(Headers* headers) {
	// Free the list of request headers in each pseudo header
	for (uint32_t i = 0; i < headers->pseudoHeadersCount; i++) {
		if (headers->items[headers->pseudoHeaders[i]].requestHeaders != NULL) {
			Free(headers->items[headers->pseudoHeaders[i]].requestHeaders);
		}
	}
}

/**
 * Iterate through the pseudo headers in the headers collection and update
 * the indices to actual headers that form them.
 */
static StatusCode updatePseudoHeaders(Headers* headers) {
	Header* curPseudoHeader = NULL;
	const char* requestHeaderName = NULL;
	const char* tmp = NULL;
	const char* curHeaderName = NULL;
	size_t headerLength = 0;
	int noOfRequestHeaders = 0;
	for (uint32_t i = 0; i < headers->pseudoHeadersCount; i++) {
		curPseudoHeader = &headers->items[headers->pseudoHeaders[i]];
		requestHeaderName = STRING(curPseudoHeader->name.data.ptr);
		// Calculate the size of request headers array
		if ((noOfRequestHeaders = countRequestHeaders(requestHeaderName)) > 0) {
			// Allocate the memory for the request headers array
			curPseudoHeader->requestHeaders = 
				(uint32_t*)Malloc(noOfRequestHeaders * sizeof(uint32_t));
			if (curPseudoHeader->requestHeaders != NULL) {
				// Iterate through each request header and find a match
				while (requestHeaderName != NULL) {
					// Find the position of the next '\x1F'
					tmp = strchr(requestHeaderName, PSEUDO_HEADER_SEP);
					// Check if there is no more '\x1F' and this is the last
					// request header
					headerLength = 
						tmp == NULL ?
						strlen(requestHeaderName) :
						(size_t)(tmp - requestHeaderName);
					for (uint32_t j = 0; j < headers->count; j++) {
						curHeaderName = STRING(headers->items[j].name.data.ptr);
						if (headerLength == strlen(curHeaderName) &&
							StringCompareLength(
								curHeaderName,
								requestHeaderName,
								headerLength) == 0) {
							curPseudoHeader->requestHeaders[
								curPseudoHeader->requestHeaderCount++] = j;
							// Found a match
							break;
						}
					}
					// Update the cursor position in the pseudo header name
					// Set to NULL if it is the last header
					requestHeaderName = tmp == NULL ? NULL : tmp + 1;
				}
			}
			else {
				freePseudoHeaders(headers);
				return INSUFFICIENT_MEMORY;
			}
		}
	}
	return SUCCESS;
}

typedef struct header_counts_t {
	uint32_t uniqueHeadersCount;
	uint32_t pseudoHeadersCount;
} headerCounts;

static headerCounts countHeaders(
	void *state,
	HeadersGetMethod get) {
	Item name;
	headerCounts counts = { 0, 0 };
	DataReset(&name.data);
	while (get(state, counts.uniqueHeadersCount, &name) >= 0) {
		// Check if name is pseduo header
		if (HeadersIsPseudo(
			STRING(name.data.ptr))) {
			counts.pseudoHeadersCount++;
		}
		COLLECTION_RELEASE(name.collection, &name);
		counts.uniqueHeadersCount++;
	}
	return counts;
}

fiftyoneDegreesHeaders* fiftyoneDegreesHeadersCreate(
	bool expectUpperPrefixedHeaders,
	void *state,
	fiftyoneDegreesHeadersGetMethod get) {
	Headers *headers;
	headerCounts counts = countHeaders(state, get);
	FIFTYONE_DEGREES_ARRAY_CREATE(
		fiftyoneDegreesHeader,
		headers,
		counts.uniqueHeadersCount);
	if (headers != NULL) {
		headers->expectUpperPrefixedHeaders = expectUpperPrefixedHeaders;
		headers->pseudoHeadersCount = counts.pseudoHeadersCount;
		if (headers->pseudoHeadersCount > 0) {
			// Allocate space for pseudo headers
			headers->pseudoHeaders =
				(uint32_t*)Malloc(counts.pseudoHeadersCount * sizeof(uint32_t));
			if (headers->pseudoHeadersCount > 0
				&& headers->pseudoHeaders == NULL) {
				Free(headers);
				headers = NULL;
				return headers;
			}
		}
		else {
			headers->pseudoHeaders = NULL;
		}

		addUniqueHeaders(headers, state, get);
		if (updatePseudoHeaders(headers) != SUCCESS) {
			HeadersFree(headers);
			headers = NULL;
		}
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
		freePseudoHeaders(headers);
		if (headers->pseudoHeaders != NULL) {
			Free(headers->pseudoHeaders);
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