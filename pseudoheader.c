#include "pseudoheader.h"
#include "string.h"

/*
 * Return the evidence value from input request header.
 *
 * @param header the request header to compare against
 * @param evidence the evidence collection to search from
 * @returns the evidence value or NULL if not found.
 */
static const char* getEvidenceValueForHeader(
    const char* header,
    fiftyoneDegreesEvidenceKeyValuePairArray *evidence) {
    for (uint32_t i = 0; i < evidence->count; i++) {
        // The evidence for Client Hints should be pure.
        // which means from Http Header.
        if (fiftyoneDegreesStringCompare(
            header, evidence->items[i].field) == 0 &&
            strcmp(evidence->items[i].originalValue, "") != 0 &&
            evidence->items[i].prefix ==
            FIFTYONE_DEGREES_EVIDENCE_HTTP_HEADER_STRING) {
            return (char *)evidence->items[i].originalValue;
        }
    }
    return NULL;
}

/*
 * Append string to the pseudo-header evidence buffer. This is used exclusively
 * by the pseudo-header evidence construction method.
 */
#define appendString(b, z, c, t, s) \
t = snprintf(b + c, z - c, "%s", s); \
if (t < 0 || (c += t) > z) { \
    memset(b, '\0', z); \
    c = -1; \
    break; \
}

/*
 * Construct a pseudo evidence given a pseudo header and the list of evidence
 * and return the number of characters added.
 *
 * @param buffer the buffer to write the evidence to
 * @param bufferSize the size of the buffer
 * @param pseudoHeader the pseudo header to create evidence for
 * @param evidence the list of evidence to get actual evidence from
 * @return the number of evidence added. Return negative number to 
 * indicate something has gone wrong.
 */
static int constructPseudoEvidence(
    char* buffer,
    size_t bufferSize,
    fiftyoneDegreesHeaders* acceptedHeaders,
    fiftyoneDegreesHeader pseudoHeader,
    fiftyoneDegreesEvidenceKeyValuePairArray* evidence) {
    int tempCount = 0, charactersAdded = 0;
    // Iterate through the request headers and construct the evidence
    // If bufferSize = 0; then don't add to the buffer
    const char *requestHeaderName = NULL, *requestHeaderValue = NULL;
    for (uint32_t i = 0; i < pseudoHeader.requestHeaderCount; i++) {
        // Get the evidence and add it to the buffer
        requestHeaderName = FIFTYONE_DEGREES_STRING(
            acceptedHeaders->items[pseudoHeader.requestHeaders[i]]
                .name.data.ptr);
        requestHeaderValue = getEvidenceValueForHeader(
            requestHeaderName, evidence);
        if (requestHeaderValue != NULL) {
            // Add open bracket
            appendString(
                buffer,
                (int)bufferSize,
                charactersAdded,
                tempCount,
                "{")
            // Add header and value
            appendString(
                buffer,
                (int)bufferSize,
                charactersAdded,
                tempCount,
                requestHeaderName)
            // Add @
            appendString(
                buffer,
                (int)bufferSize,
                charactersAdded,
                tempCount,
                "@")
            // Add evidence
            appendString(
                buffer,
                (int)bufferSize,
                charactersAdded,
                tempCount,
                requestHeaderValue)
                // Add open bracket
            appendString(
                buffer,
                (int)bufferSize,
                charactersAdded,
                tempCount,
                "}")
        }
    }
    return charactersAdded;
}

/*
 * Check if an evidence is present for a uniqueHeader
 */
static bool isEvidencePresentForHeader(
    fiftyoneDegreesEvidenceKeyValuePairArray* evidence,
    fiftyoneDegreesHeader* uniqueHeader) {
    for (uint32_t i = 0; i < evidence->count; i++) {
        if ((evidence->items[i].prefix == FIFTYONE_DEGREES_EVIDENCE_QUERY ||
            evidence->items[i].prefix ==
                FIFTYONE_DEGREES_EVIDENCE_HTTP_HEADER_STRING) && 
            fiftyoneDegreesStringCompare(
                FIFTYONE_DEGREES_STRING(uniqueHeader->name.data.ptr),
                evidence->items[i].field) == 0) {
            return true;
        }
    }
    return false;
}

void
fiftyoneDegreesPseudoHeadersAddEvidence(
    fiftyoneDegreesEvidenceKeyValuePairArray* evidence,
    fiftyoneDegreesHeaders* acceptedHeaders,
    size_t bufferSize,
    fiftyoneDegreesException* exception) {
    fiftyoneDegreesHeader curHeader;
    char* buffer = NULL;
    int charAdded = 0;
    if (evidence != NULL && acceptedHeaders != NULL) {
        for (uint32_t i = 0; i < acceptedHeaders->pseudoHeadersCount; i++) {
            curHeader = acceptedHeaders->items[acceptedHeaders->pseudoHeaders[i]];
            if (!isEvidencePresentForHeader(evidence, &curHeader)) {
                buffer =
                    (char*)evidence->pseudoEvidence->items[
                        evidence->pseudoEvidence->count].originalValue;
                if (buffer != NULL) {
                    charAdded = constructPseudoEvidence(
                        buffer,
                        bufferSize,
                        acceptedHeaders,
                        curHeader,
                        evidence);
                    // charAdded == 0 means no evidence is added due to valid
                    // reasons such as missing evidence for request headers
                    // that form the pseudo header.
                    if (charAdded > 0) {
                        evidence->pseudoEvidence->items[
                            evidence->pseudoEvidence->count].field =
                            FIFTYONE_DEGREES_STRING(curHeader.name.data.ptr);
                        evidence->pseudoEvidence->items[
                            evidence->pseudoEvidence->count].prefix =
                            FIFTYONE_DEGREES_EVIDENCE_HTTP_HEADER_STRING;
                        evidence->pseudoEvidence->count++;
                    }
                    else if (charAdded < 0) {
                        // The reason to set exception here is because without
                        // a fully constructed pseudo evidence, Client Hints
                        // won't work.
                        FIFTYONE_DEGREES_EXCEPTION_SET(
                            FIFTYONE_DEGREES_STATUS_INSUFFICIENT_BUFFER);
                        break;
                    }
                }
            }
        }
    }
    else {
        FIFTYONE_DEGREES_EXCEPTION_SET(
            FIFTYONE_DEGREES_STATUS_NULL_POINTER);
    }
}

void fiftyoneDegreesPseudoHeadersRemoveEvidence(
    fiftyoneDegreesEvidenceKeyValuePairArray* evidence,
    size_t bufferSize) {
    if (evidence != NULL) {
        fiftyoneDegreesEvidenceKeyValuePair* pair = NULL;
        for (uint32_t i = 0; i < evidence->pseudoEvidence->count; i++) {
            pair = &evidence->pseudoEvidence->items[i];
            pair->field = NULL;
            memset((void*)pair->originalValue, '\0', bufferSize);
        }
        evidence->pseudoEvidence->count = 0;
    }
}
