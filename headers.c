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

static bool doesHeaderExist(Headers *headers, const char *headerName) {
	uint32_t i;
	const char *test;

	if (headerName == NULL) {
		return false;
	}

	for (i = 0; i < headers->count; i++) {
		test = headers->items[i].name;
		if (test != NULL &&
			_stricmp(
				headerName,
				test) == 0) {
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
static StatusCode addUniqueHeaders(
	Headers *headers,
	void *state,
	HeadersGetMethod get,
	uint32_t uniqueHeadersCount) {
	size_t nameSize;
	uint32_t i, pIndex, uniqueId;
	Item nameItem;
	Header *header;
	for (i = 0, pIndex = 0; i < uniqueHeadersCount; i++) {
		header = &headers->items[headers->count];
		DataReset(&nameItem.data);
		nameItem.collection = NULL;
		nameItem.handle = NULL;
		uniqueId = get(state, i, &nameItem);
		nameSize = ((String*)nameItem.data.ptr) == NULL ?
			0 : ((String*)nameItem.data.ptr)->size;
		if (nameSize > 1 &&
			doesHeaderExist(headers, STRING(nameItem.data.ptr)) == false) {
			header->uniqueId = uniqueId;
			header->name = Malloc(sizeof(char) * nameSize);
			if (header->name == NULL) {
				return INSUFFICIENT_MEMORY;
			}
			header->nameLength = (int16_t)nameSize - 1;
			memcpy(
				(void*)header->name,
				&((String*)nameItem.data.ptr)->value,
				((String*)nameItem.data.ptr)->size);
			// Check if header is pseudo header then add it to the list
			if (HeadersIsPseudo(header->name)) {
				headers->pseudoHeaders[pIndex++] = headers->count;
			}
			else {
				header->requestHeaders = NULL;
			}
			header->requestHeaderCount = 0;
			headers->count++;
		}
		else {
			assert(nameItem.collection != NULL);
		}
		COLLECTION_RELEASE(nameItem.collection, &nameItem);
	}
	return SUCCESS;
}

static uint32_t countRequestHeaders(const char* pseudoHeaders) {
	uint32_t count;
	const char* tmp = pseudoHeaders;
	// Count start from 1 as there be at least one header name
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
	bool found;
	Header* curPseudoHeader = NULL;
	const char* requestHeaderName = NULL;
	const char* tmp = NULL;
	const char* curHeaderName = NULL;
	size_t headerLength = 0;
	int noOfRequestHeaders = 0;
	for (uint32_t i = 0; i < headers->pseudoHeadersCount; i++) {
		curPseudoHeader = &headers->items[headers->pseudoHeaders[i]];
		requestHeaderName = curPseudoHeader->name;
		// Calculate the size of request headers array
		if ((noOfRequestHeaders = countRequestHeaders(requestHeaderName)) > 1) {
			// Allocate the memory for the request headers array
			curPseudoHeader->requestHeaders = 
				(uint32_t*)Malloc(noOfRequestHeaders * sizeof(uint32_t));
			if (curPseudoHeader->requestHeaders != NULL) {
				// Iterate through each request header and find a match
				while (requestHeaderName != NULL) {
					found = false;
					// Find the position of the next '\x1F'
					tmp = strchr(requestHeaderName, PSEUDO_HEADER_SEP);
					// Check if there is no more '\x1F' and this is the last
					// request header
					headerLength = 
						tmp == NULL ?
						strlen(requestHeaderName) :
						(size_t)(tmp - requestHeaderName);
					if (headerLength > 0) {
						for (uint32_t j = 0; j < headers->count && found == false; j++) {
							curHeaderName = headers->items[j].name;
							if (headerLength == strlen(curHeaderName) &&
								StringCompareLength(
									curHeaderName,
									requestHeaderName,
									headerLength) == 0) {
								curPseudoHeader->requestHeaders[
									curPseudoHeader->requestHeaderCount++] = j;
								// Found a match
								found = true;
							}
						}
						if (found == false) {
							// The header was not found, so add it to the header structure.
							char* name = Malloc(sizeof(char) * (headerLength + 1));
							if (name == NULL) {
								return INSUFFICIENT_MEMORY;
							}
							memcpy(
								name,
								requestHeaderName,
								headerLength);
							name[headerLength] = '\0';
							headers->items[headers->count].name = name;
							headers->items[headers->count].nameLength = (int16_t)headerLength;
							headers->items[headers->count].requestHeaderCount = 0;
							headers->items[headers->count].requestHeaders = NULL;
							headers->items[headers->count].uniqueId =
								(uint32_t)(uint64_t)headers->items[headers->count].name; // todo is this really unique?
							curPseudoHeader->requestHeaders[
								curPseudoHeader->requestHeaderCount++] = headers->count;
							headers->count++;
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

static int splitPseudoHeaders(
	const char* pseudoHeaderName,
	char** headers) {
	size_t headerLength;
	const char* tmp = NULL;
	int i = 0;
	if (headers != NULL) {
		// Iterate through each request header and find a match
		while (pseudoHeaderName != NULL) {
			// Find the position of the next '\x1F'
			tmp = strchr(pseudoHeaderName, PSEUDO_HEADER_SEP);
			// Check if there is no more '\x1F' and this is the last
			// request header
			headerLength =
				tmp == NULL ?
				strlen(pseudoHeaderName) :
				(size_t)(tmp - pseudoHeaderName);
			headers[i] = Malloc(sizeof(char) * (headerLength + 1));
			if (headers[i] == NULL) {
				return i;
			}
			memcpy(headers[i], pseudoHeaderName, headerLength);
			headers[i][headerLength] = '\0';
			i++;
			// Update the cursor position in the pseudo header name
			// Set to NULL if it is the last header
			pseudoHeaderName = tmp == NULL ? NULL : tmp + 1;
		}
	}
	return i;
}

static bool tryAddPseudoHeader(
	const char* header,
	char** headers,
	int headersCount,
	Exception *exception) {
	for (int i = 0; i < headersCount; i++) {
		if (StringCompare(header, headers[i]) == 0) {
			return false;
		}
	}
	headers[headersCount] = Malloc(sizeof(char) * (strlen(header) + 1));
	if (headers[headersCount] == NULL) {
		EXCEPTION_SET(INSUFFICIENT_MEMORY);
		return false;
	}
	memcpy(headers[headersCount], header, strlen(header) + 1);
	return true;
}

static int countTotalRequestHeaders(
	void* state,
	HeadersGetMethod get,
	int headersCount) {
	Item name;
	int i;
	int total = 0;
	for (i = 0; i < headersCount; i++) {
		if (get(state, i, &name) >= 0) {
			// Check if name is pseduo header
			if (HeadersIsPseudo(
				STRING(name.data.ptr))) {
				total += countRequestHeaders(STRING(name.data.ptr));
			}
		}
		COLLECTION_RELEASE(name.collection, &name);
	}
	return total;
}

static int countUnavailablePseudoHeaders(
	void* state,
	HeadersGetMethod get,
	int headersCount,
	Exception *exception) {
	Item name;
	Item otherName;
	int i, j, k;
	char** tmpHeaders;
	char** unavailable =  Malloc(sizeof(char*) * countTotalRequestHeaders(state, get, headersCount));
	if (unavailable == NULL) {
		EXCEPTION_SET(INSUFFICIENT_MEMORY);
		return 0;
	}
	bool found;
	int unavailableCount = 0;
	int pseudoCount, splitCount;
	DataReset(&name.data);
	DataReset(&otherName.data);
	for (i = 0; i < headersCount; i++) {
		if (get(state, i, &name) >= 0) {
			// Check if name is pseduo header
			if (HeadersIsPseudo(
				STRING(name.data.ptr))) {
				pseudoCount = countRequestHeaders(STRING(name.data.ptr));
				tmpHeaders = Malloc(sizeof(char*) * pseudoCount);
				if (tmpHeaders == NULL) {
					EXCEPTION_SET(INSUFFICIENT_MEMORY);
					break;
				}
				splitCount = splitPseudoHeaders(STRING(name.data.ptr), tmpHeaders);
				if (splitCount < pseudoCount) {
					EXCEPTION_SET(INSUFFICIENT_MEMORY);
				}
				for (j = 0; j < splitCount; j++) {
					if (EXCEPTION_OKAY) {
						found = false;

						for (k = 0; k < headersCount && found == false; k++) {
							if (get(state, k, &otherName) >= 0) {
								if (StringCompare(STRING(otherName.data.ptr), tmpHeaders[j]) == 0) {
									found = true;
								}
							}
							COLLECTION_RELEASE(otherName.collection, &otherName);

						}
						if (found == false) {
							if (tryAddPseudoHeader(tmpHeaders[j], unavailable, unavailableCount, exception)) {
								unavailableCount++;
							}
						}
					}
					Free(tmpHeaders[j]);
				}
				Free(tmpHeaders);
			}
		}
		COLLECTION_RELEASE(name.collection, &name);
	}
	for (i = 0; i < unavailableCount; i++) {
		Free(unavailable[i]);
	}
	Free(unavailable);
	return unavailableCount;
}

fiftyoneDegreesHeaders* fiftyoneDegreesHeadersCreate(
	bool expectUpperPrefixedHeaders,
	void *state,
	fiftyoneDegreesHeadersGetMethod get) {
	EXCEPTION_CREATE;
	Headers *headers;
	headerCounts counts = countHeaders(state, get);
	int unavailablePseudoHeaders =
		countUnavailablePseudoHeaders(state, get, counts.uniqueHeadersCount, exception);
	if (EXCEPTION_FAILED) {
		return NULL;
	}
	FIFTYONE_DEGREES_ARRAY_CREATE(
		fiftyoneDegreesHeader,
		headers,
		counts.uniqueHeadersCount + unavailablePseudoHeaders);
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

		if (addUniqueHeaders(headers, state, get, counts.uniqueHeadersCount)
			!= SUCCESS) {
			HeadersFree(headers);
			headers = NULL;
		}
		if (headers != NULL && updatePseudoHeaders(headers) != SUCCESS) {
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
	const char *compare;
	size_t compareLength;

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
		compare = headers->items[i].name;
		compareLength = (size_t)headers->items[i].nameLength;
		if (compareLength - 1 == length &&
			compare != NULL &&
			StringCompareLength(
				httpHeaderName, 
				compare, 
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
			Free((void*)headers->items[i].name);
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