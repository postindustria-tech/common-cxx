#include "pseudoheader.h"
#include "fiftyone.h"
#include "string.h"
#include "evidence.h"

/*
 * Return the evidence value from input request header.
 *
 * @param header the request header to compare against
 * @param evidence the evidence collection to search from
 * @param prefix the target prefix in the evidence collection
 * @return the evidence value or NULL if not found.
 */
static const char* getEvidenceValueForHeader(
    const char* header,
    const EvidenceKeyValuePairArray *evidence,
    EvidencePrefix prefix) {
    for (uint32_t i = 0; i < evidence->count; i++) {
        // The evidence for Client Hints should be pure.
        // which means from Http Header.
        if (StringCompare(
            header, evidence->items[i].field) == 0 &&
            strcmp(evidence->items[i].originalValue, "") != 0 &&
            evidence->items[i].prefix ==
            prefix) {
            return (char *)evidence->items[i].originalValue;
        }
    }
    return NULL;
}

/*
 * Construct a pseudo evidence given a pseudo header and the list of evidence
 * and return the number of characters added.
 *
 * @param buffer the buffer to write the evidence to
 * @param bufferSize the size of the buffer
 * @param acceptedHeaders the list of accepted headers
 * @param pseudoHeader the pseudo header to create evidence for
 * @param evidence the list of evidence to get actual evidence from
 * @param prefix the target prefix to look for in the evidence list
 * @return the number of characters added. Return negative number to 
 * indicate something has gone wrong.
 */
static int constructPseudoEvidence(
    char* buffer,
    size_t bufferSize,
    Headers* acceptedHeaders,
    Header pseudoHeader,
    const EvidenceKeyValuePairArray* evidence,
    EvidencePrefix prefix) {
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
            requestHeaderName, evidence, prefix);
        if (requestHeaderValue != NULL) {
            if (i != 0) {
                tempCount = Snprintf(
                    buffer + charactersAdded,
                    (int)bufferSize - charactersAdded,
                    "%c",
                    PSEUDO_HEADER_SEP);
                if (tempCount < 0 ||
                    ((size_t)(charactersAdded += tempCount) > bufferSize)) {
                    memset(buffer, '\0', bufferSize);
                    charactersAdded = -1;
                    break;
                }
            }

            // Add evidence
            tempCount = Snprintf(
                buffer + charactersAdded,
                (int)bufferSize - charactersAdded,
                "%s",
                requestHeaderValue);
            if (tempCount < 0 ||
                ((size_t)(charactersAdded += tempCount) > bufferSize)) {
                memset(buffer, '\0', bufferSize);
                charactersAdded = -1;
                break;
            }
        }
        else {
            // Not enough evidence
            memset(buffer, '\0', bufferSize);
            charactersAdded = 0;
            break;
        }
    }
    return charactersAdded;
}

/*
 * Check if an evidence is present for a uniqueHeader with specific prefix
 * @param evidence the evidence collection
 * @param uniqueHeader the target unique header to check for
 * @param acceptedPrefixes the list of accepted accepted prefixes
 * @param numberOfPrefixes number of accepted prefixes
 * @return whether the evidence for the target unique header presents in the
 * evidence collection.
 */
static bool isEvidencePresentForHeader(
    EvidenceKeyValuePairArray* evidence,
    Header* uniqueHeader,
    const EvidencePrefix* acceptedPrefixes,
    size_t numberOfPrefixes) {
    bool matchPrefix = false;
    for (uint32_t i = 0; i < evidence->count; i++) {
        matchPrefix = false;
        // Check if the prefix matches is in the check list
        for (size_t j = 0; j < numberOfPrefixes; j++) {
            if (evidence->items[i].prefix == acceptedPrefixes[j]) {
                matchPrefix = true;
                break;
            }
        }

        if (matchPrefix && StringCompare(
                STRING(uniqueHeader->name.data.ptr),
                evidence->items[i].field) == 0) {
            return true;
        }
    }
    return false;
}

void
fiftyoneDegreesPseudoHeadersAddEvidence(
    EvidenceKeyValuePairArray* evidence,
    Headers* acceptedHeaders,
    size_t bufferSize,
    const EvidencePrefix* orderOfPrecedence,
    size_t precedenceSize,
    Exception* exception) {
    Header curHeader;
    char* buffer = NULL;
    int charAdded = 0;
    if (evidence != NULL && acceptedHeaders != NULL) {
        for (uint32_t i = 0;
            i < acceptedHeaders->pseudoHeadersCount && EXCEPTION_OKAY;
            i++) {
            curHeader =
                acceptedHeaders->items[acceptedHeaders->pseudoHeaders[i]];
            if (!isEvidencePresentForHeader(
                evidence,
                &curHeader,
                orderOfPrecedence,
                precedenceSize)) {
                buffer =
                    (char*)evidence->pseudoEvidence->items[
                        evidence->pseudoEvidence->count].originalValue;
                if (buffer != NULL) {
                    for (size_t j = 0; j < precedenceSize; j++) {
                        charAdded = constructPseudoEvidence(
                            buffer,
                            bufferSize,
                            acceptedHeaders,
                            curHeader,
                            evidence,
                            orderOfPrecedence[j]);
                        // charAdded == 0 means no evidence is added due to
                        // valid reasons such as missing evidence for request
                        // headers that form the pseudo header.
                        if (charAdded > 0) {
                            evidence->pseudoEvidence->items[
                                evidence->pseudoEvidence->count].field =
                                STRING(curHeader.name.data.ptr);
                                evidence->pseudoEvidence->items[
                                    evidence->pseudoEvidence->count].prefix =
                                    orderOfPrecedence[j];
                                    evidence->pseudoEvidence->count++;
                                // Once a complete pseudo evidence is found
                                // move on the next pseudo pseudo header
                                break;
                        }
                        else if (charAdded < 0) {
                            PseudoHeadersRemoveEvidence(
                                evidence,
                                bufferSize);
                            // The reason to set exception here is because
                            // without a fully constructed pseudo evidence,
                            // Client Hints won't work.
                            EXCEPTION_SET(
                                FIFTYONE_DEGREES_STATUS_INSUFFICIENT_MEMORY);
                            break;
                        }
                    }
                }
            }
        }
    }
    else {
        EXCEPTION_SET(
            FIFTYONE_DEGREES_STATUS_NULL_POINTER);
    }
}

void fiftyoneDegreesPseudoHeadersRemoveEvidence(
    fiftyoneDegreesEvidenceKeyValuePairArray* evidence,
    size_t bufferSize) {
    if (evidence != NULL && evidence->pseudoEvidence != NULL) {
        EvidenceKeyValuePair* pair = NULL;
        for (uint32_t i = 0; i < evidence->pseudoEvidence->count; i++) {
            pair = &evidence->pseudoEvidence->items[i];
            pair->field = NULL;
            memset((void*)pair->originalValue, '\0', bufferSize);
        }
        evidence->pseudoEvidence->count = 0;
    }
}
