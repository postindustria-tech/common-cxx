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

#include "evidence.h"
#include "fiftyone.h"

typedef struct evidence_iterate_state_t {
	fiftyoneDegreesEvidenceKeyValuePairArray *evidence;
	EvidencePrefix prefix;
	void *state;
	fiftyoneDegreesEvidenceIterateMethod callback;
} evidenceIterateState;

static EvidencePrefixMap _map[] = {
	{ "server.", sizeof("server.") - 1, FIFTYONE_DEGREES_EVIDENCE_SERVER },
	{ "header.", sizeof("header.") - 1, FIFTYONE_DEGREES_EVIDENCE_HTTP_HEADER_STRING },
	{ "query.", sizeof("query.") - 1, FIFTYONE_DEGREES_EVIDENCE_QUERY },
	{ "cookie.", sizeof("cookie.") - 1, FIFTYONE_DEGREES_EVIDENCE_COOKIE }
};

static void parsePair(EvidenceKeyValuePair *pair) {
	switch (pair->prefix) {
	case FIFTYONE_DEGREES_EVIDENCE_HTTP_HEADER_IP_ADDRESSES:
	case FIFTYONE_DEGREES_EVIDENCE_HTTP_HEADER_STRING:
	case FIFTYONE_DEGREES_EVIDENCE_SERVER:
	case FIFTYONE_DEGREES_EVIDENCE_QUERY:
	case FIFTYONE_DEGREES_EVIDENCE_COOKIE:
	default:
		pair->parsedValue = pair->originalValue;
		pair->parsedLength = strlen(pair->parsedValue);
		break;
	}
}

// If a string comparison of the pair field and the header indicates a match
// then set the header to avoid a string comparison in future iterations.
static void setPairHeader(EvidenceKeyValuePair* pair, Header* header) {
	if (pair->fieldLength == header->length &&
		StringCompareLength(pair->field, header->name, header->length) == 0) {
		pair->header = header;
	}
}

/*
 * Iterate through an evidence collection and perform callback on the evidence
 * whose prefix matches the input prefixes.
 *
 * @param evidence the evidence collection to process
 * @param prefixes the accepted evidence prefixes
 * @param state the state object to hold the current state of the process
 * @param callback the method to call back when a matched evidence is found.
 * @return number of evidence processed.
 */
static uint32_t evidenceIterate(
	fiftyoneDegreesEvidenceKeyValuePairArray* evidence,
	int prefixes,
	void* state,
	fiftyoneDegreesEvidenceIterateMethod callback) {
	uint32_t i = 0, count = 0;
	EvidenceKeyValuePair* pair;
	bool cont = true;
	while (cont && i < evidence->count) {
		pair = &evidence->items[i++];
		if ((pair->prefix & prefixes) == pair->prefix) {
			if (pair->parsedValue == NULL) {
				parsePair(pair);
			}
			cont = callback(state, pair);
			count++;
		}
	}
	return count;
}

/**
 * Finds the evidence pair that matches the header. Returns null if a pair does
 * not exist.
 */
static EvidenceKeyValuePair* findHeaderEvidence(
	EvidenceKeyValuePairArray* evidence,
	int prefixes,
	Header* header) {
	EvidenceKeyValuePair* pair;

	// For each of the evidence pairs available.
	for (uint32_t i = 0; i < evidence->count; i++) {
		pair = &evidence->items[i];

		// Check that the prefix is one that is being considered.
		if ((pair->prefix & prefixes) == pair->prefix) {

			// If the header has been assigned to the pair check to see if this
			// one is a match.
			if (pair->header == NULL) {
				setPairHeader(pair, header);
			}

			// If the pair's header and the required header are the same.
			if (pair->header == header) {

				// Ensure the parsed value is populated before returning.
				if (pair->parsedValue == NULL) {
					parsePair(pair);
				}
				return pair;
			}
		}
	}
	return NULL;
}

// Copies the pair parsed value to the buffer checking that there are 
// sufficient bytes remaining in the buffer for the parsed value. Returns NULL
// if there is insufficient bytes remaining in the buffer, or the new pointer 
// in buffer for the next character.
static char* addPairValueToBuffer(
	char* buffer, 
	size_t length, 
	EvidenceKeyValuePair* pair) {
	if (length >= pair->parsedLength &&
		memcpy(
			buffer, 
			(char*)pair->parsedValue, 
			pair->parsedLength) == buffer) {
		return buffer + pair->parsedLength;
	}
	return NULL;
}

// For the header finds the corresponding evidence in the array of evidence. If
// found then copies the parsed value into the buffer considering the remaining
// length available. Returns the next character in the buffer to add more 
// characters, or NULL if either not found or insufficient buffer length 
// remaining.
static char* addHeaderValueToBuffer(
	fiftyoneDegreesEvidenceKeyValuePairArray* evidence,
	int prefixes,
	fiftyoneDegreesHeader* header,
	char* buffer,
	size_t length) {
	char* current = buffer;

	// Get the evidence that corresponds to the header. If it doesn't exist
	// then there is no evidence for the header and a call back will not be
	// possible.
	EvidenceKeyValuePair* pair = findHeaderEvidence(
		evidence, 
		prefixes, 
		header);
	if (pair == NULL) {
		return NULL;
	}

	// Copy the value of the evidence pair in to the buffer advancing the 
	// current character in the buffer.
	current = addPairValueToBuffer(buffer, length, pair);

	// If the buffer is not sufficiently large to hold the pair value then
	// return.
	if (current == NULL) {
		return NULL;
	}

	return current;
}

// Adds the character value checking sufficient capacity in the buffer is 
// available. Returns the next character in the buffer to add more  characters, 
// or NULL if either not found or insufficient buffer length remaining.
static char* addCharacter(char* buffer, size_t length, char value) {
	if (length == 0) {
		return NULL;
	}
	*buffer = value;
	return buffer + 1;
}

// Assembles a pseudo header in the buffer. If this can not be achieved returns 
// true to indicate that processing should continue. If a pseudo header can be
// created then returns the result of the callback which might decide not to 
// continue processing.
static bool processPseudoHeader(
	EvidenceKeyValuePairArray* evidence,
	int prefixes,
	Header* header,
	char* buffer,
	size_t length,
	void* state,
	fiftyoneDegreesEvidenceIterateForHeadersMethod callback) {
	char* current = buffer;
	size_t remaining = length;

	// For each of the headers that form the pseudo header.
	for (uint32_t i = 0; i < header->segmentHeaders->count; i++) {

		// Add the header evidence that forms the segment if available updating
		// the current buffer position if available.
		current = addHeaderValueToBuffer(
			evidence, 
			prefixes, 
			header->segmentHeaders->items[i], 
			current, 
			remaining);

		// If the pseudo header wasn't found, or insufficient space was 
		// available to copy it, then return.
		if (current == NULL) {
			return true;
		}

		// Update the remaining capacity of the buffer.
		remaining = length - (current - buffer);

		// Add the pseudo header separator.
		current = addCharacter(current, remaining, PSEUDO_HEADER_SEP);
		
		// If there was insufficient space for the separator then return.
		if (current == NULL) {
			return true;
		}

		// Update the remaining capacity of the buffer.
		remaining = length - (current - buffer);
	}

	// Switch the last pseudo header separator to a null terminating character.
	*(current - 1) = '\0';

	// A full header has been formed so call the callback with the buffer and
	// the number of characters populated.
	return callback(state, header, (const char*)buffer, length - remaining);
}

// Finds the header in the evidence, and if available calls the callback. 
// Returns true if further processing should continue, otherwise false to stop
// further processing.
static bool processHeader(
	EvidenceKeyValuePairArray* evidence,
	int prefixes,
	Header* header,
	void* state,
	fiftyoneDegreesEvidenceIterateForHeadersMethod callback) {

	// Get the evidence that corresponds to the header. If it doesn't exist
	// then there is no evidence for the header and a call back will not be
	// possible.
	EvidenceKeyValuePair* pair = findHeaderEvidence(
		evidence,
		prefixes,
		header);
	if (pair == NULL) {
		return true;
	}

	// A full header has been formed so call the callback with the buffer and
	// the number of characters populated.
	return callback(
		state, 
		header, 
		(const char*)pair->parsedValue, 
		pair->parsedLength);
}

fiftyoneDegreesEvidenceKeyValuePairArray*
fiftyoneDegreesEvidenceCreate(uint32_t capacity) {
	EvidenceKeyValuePairArray *evidence;
	uint32_t i;
	FIFTYONE_DEGREES_ARRAY_CREATE(EvidenceKeyValuePair, evidence, capacity);
	if (evidence != NULL) {
		for (i = 0; i < evidence->capacity; i++) {
			evidence->items[i].field = NULL;
			evidence->items[i].fieldLength = 0;
			evidence->items[i].header = NULL;
			evidence->items[i].originalValue = NULL;
			evidence->items[i].parsedValue = NULL;
			evidence->items[i].prefix = FIFTYONE_DEGREES_EVIDENCE_IGNORE;
		}
	}
	return evidence;
}

void fiftyoneDegreesEvidenceFree(
	fiftyoneDegreesEvidenceKeyValuePairArray *evidence) {
	Free(evidence);
}

fiftyoneDegreesEvidenceKeyValuePair* fiftyoneDegreesEvidenceAddString(
	fiftyoneDegreesEvidenceKeyValuePairArray *evidence,
	fiftyoneDegreesEvidencePrefix prefix,
	const char *field,
	const char *originalValue) {
	EvidenceKeyValuePair *pair = NULL;
	if (evidence->count < evidence->capacity) {
		pair = &evidence->items[evidence->count++];
		pair->prefix = prefix;
		pair->field = field;
		pair->fieldLength = strlen(field);
		pair->originalValue = (void*)originalValue;
		pair->parsedValue = NULL;
		pair->header = NULL;
	}
	return pair;
}

uint32_t fiftyoneDegreesEvidenceIterate(
	fiftyoneDegreesEvidenceKeyValuePairArray *evidence,
	int prefixes,
	void *state,
	fiftyoneDegreesEvidenceIterateMethod callback) {
	return evidenceIterate(
		evidence,
		prefixes,
		state,
		callback);
}

fiftyoneDegreesEvidencePrefixMap* fiftyoneDegreesEvidenceMapPrefix(
	const char *key) {
	uint32_t i;
	size_t length = strlen(key);
	EvidencePrefixMap *map;
	for (i = 0; i < sizeof(_map) / sizeof(EvidencePrefixMap); i++) {
		map = &_map[i];
		if (map->prefixLength < length &&
			strncmp(map->prefix, key, map->prefixLength) == 0) {
			return map;
		}
	}
	return NULL;
}

EXTERNAL const char* fiftyoneDegreesEvidencePrefixString(
	fiftyoneDegreesEvidencePrefix prefix) {
	uint32_t i;
	EvidencePrefixMap* map;
	for (i = 0; i < sizeof(_map) / sizeof(EvidencePrefixMap); i++) {
		map = &_map[i];
		if (map->prefixEnum == prefix) {
			return map->prefix;
		}
	}
	return NULL;
}

bool fiftyoneDegreesEvidenceIterateForHeaders(
	fiftyoneDegreesEvidenceKeyValuePairArray* evidence,
	int prefixes,
	fiftyoneDegreesHeaderPtrs* headers,
	char* buffer,
	size_t length,
	void* state,
	fiftyoneDegreesEvidenceIterateForHeadersMethod callback) {
	Header* header;

	// For each of the headers process as either a standard header, or a pseudo
	// header.
	for (uint32_t i = 0; i < headers->count; i++) {
		header = headers->items[i];

		// Try and process the header as a standard header.
		if (processHeader(
			evidence,
			prefixes,
			header,
			state,
			callback) == false) {
			return true;
		}

		// If the header is a pseudo header then attempt to assemble a complete
		// value from the evidence and process it.
		if (header->segmentHeaders != NULL &&
			header->segmentHeaders->count > 0) {
			if (processPseudoHeader(
				evidence,
				prefixes,
				header,
				buffer,
				length,
				state,
				callback) == false) {
				return true;
			}
		}
	}

	return false;
}