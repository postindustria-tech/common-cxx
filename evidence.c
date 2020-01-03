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

static void parsePair(
	EvidenceKeyValuePairArray *evidence, 
	EvidenceKeyValuePair *pair) {
	switch (pair->prefix) {
	case FIFTYONE_DEGREES_EVIDENCE_HTTP_HEADER_IP_ADDRESSES:
		// parseIpAddresses(evidence->Malloc, pair->originalValue);
	case FIFTYONE_DEGREES_EVIDENCE_HTTP_HEADER_STRING:
	case FIFTYONE_DEGREES_EVIDENCE_SERVER:
	case FIFTYONE_DEGREES_EVIDENCE_QUERY:
	case FIFTYONE_DEGREES_EVIDENCE_COOKIE:
	default:
		pair->parsedValue = pair->originalValue;
		break;
	}
}

fiftyoneDegreesEvidenceKeyValuePairArray*
fiftyoneDegreesEvidenceCreate(uint32_t capacity) {
	EvidenceKeyValuePairArray *evidence;
	uint32_t i;
	FIFTYONE_DEGREES_ARRAY_CREATE(EvidenceKeyValuePair, evidence, capacity);
	if (evidence != NULL) {
		for (i = 0; i < evidence->capacity; i++) {
			evidence->items[i].field = NULL;
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
		pair->originalValue = (void*)originalValue;
		pair->parsedValue = NULL;
	}
	return pair;
}

uint32_t fiftyoneDegreesEvidenceIterate(
	fiftyoneDegreesEvidenceKeyValuePairArray *evidence,
	int prefixes,
	void *state,
	fiftyoneDegreesEvidenceIterateMethod callback) {
	uint32_t i = 0, count = 0;
	bool cont = true;
	EvidenceKeyValuePair *pair;
	while (cont == true &&  i < evidence->count) {
		pair = &evidence->items[i++];
		if ((pair->prefix & prefixes) == pair->prefix) {
			if (pair->parsedValue == NULL) {
				parsePair(evidence, pair);
			}
			cont = callback(state, pair);
			count++;
		}
	}
	return count;
}

fiftyoneDegreesEvidencePrefixMap* fiftyoneDegreesEvidenceMapPrefix(
	const char *key) {
	int i;
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