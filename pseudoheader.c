#include "pseudoheader.h"
#include "fiftyone.h"
#include "string.h"
#include "evidence.h"

/*
 * Return the evidence value from input request header.
 *
 * @param segment the header segment to get evidence for
 * @param evidence the evidence collection to search from
 * @param prefix the target prefix in the evidence collection
 * @return the evidence value or NULL if not found.
 */
static const char* getEvidenceValueForHeader(
    HeaderSegment* segment,
    const EvidenceKeyValuePairArray *evidence,
    EvidencePrefix prefix) {
    size_t length;
    EvidenceKeyValuePair *pair;
    for (uint32_t i = 0; i < evidence->count; i++) {
        pair = &evidence->items[i];
        if (pair->prefix == prefix) {
            length = strlen(pair->field);
            if (length == segment->length &&
                StringCompareLength(
                    pair->field,
                    segment->segment, 
                    length) == 0) {
                return (char*)evidence->items[i].originalValue;
            }
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
 * @param header the pseudo header to create evidence for
 * @param evidence the list of evidence to get actual evidence from
 * @param prefix the target prefix to look for in the evidence list
 * @return the number of characters added. Return negative number to 
 * indicate something has gone wrong.
 */
static size_t constructPseudoEvidence(
    char* buffer,
    size_t bufferSize,
    Header* header,
    const EvidenceKeyValuePairArray* evidence,
    EvidencePrefix prefix) {
    uint32_t i;
    size_t added;
    HeaderSegment* segment;
    const char* value;
    char* current = buffer;
    char* max = buffer + bufferSize;

    // Use the segments from the header to construct the evidence.
    for (i = 0; i < header->segments->count; i++) {
        
        // Get the evidence for the segment.
        segment = &header->segments->items[i];
        value = getEvidenceValueForHeader(segment, evidence, prefix);

        // If there is no evidence value then the header can't be constructed
        // so return.
        if (value == NULL) {
            memset(buffer, '\0', 1);
            return 0;
        }

        // If this is a subsequent segment value then add the separator.
        if (i != 0) {
            *current = PSEUDO_HEADER_SEP;
            current++;
        }

        // Add the value to the buffer.
        added = Snprintf(current, max - current, "%s", value);
        if (added < 0 || added >= (size_t)(max - current)) {
            memset(buffer, '\0', bufferSize);
            return 0;
        }
        current += added;
    }

    return current - buffer;
}

/*
 * Check if an evidence is present for a uniqueHeader with specific prefix
 * @param evidence the evidence collection
 * @param header the target unique header to check for
 * @param acceptedPrefixes the list of accepted prefixes
 * @param numberOfPrefixes number of accepted prefixes
 * @return whether the evidence for the target unique header presents in the
 * evidence collection.
 */
static bool isEvidencePresentForHeader(
    EvidenceKeyValuePairArray* evidence,
    Header* header,
    const EvidencePrefix* acceptedPrefixes,
    size_t numberOfPrefixes) {
    bool matchPrefix = false;
    size_t length;
    EvidenceKeyValuePair* pair;
    for (uint32_t i = 0; i < evidence->count; i++) {
        pair = &evidence->items[i];
        matchPrefix = false;

        // Check if the prefix matches is in the check list
        for (size_t j = 0; j < numberOfPrefixes; j++) {
            if (pair->prefix == acceptedPrefixes[j]) {
                matchPrefix = true;
                break;
            }
        }

        // Compare the field name to the header name if the prefix matches.
        if (matchPrefix) {
            length = strlen(pair->field);
            if (length == header->nameLength &&
                StringCompare(header->name, pair->field) == 0) {
                return true;
            }
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
    Header* header;
    char* buffer = NULL;
    size_t charAdded;
    uint32_t i;
    if (evidence != NULL && acceptedHeaders != NULL) {
        for (i = 0;
            i < acceptedHeaders->count && EXCEPTION_OKAY;
            i++) {
            header = &acceptedHeaders->items[i];
            if (isEvidencePresentForHeader(
                evidence,
                header,
                orderOfPrecedence,
                precedenceSize) == false) {
                buffer = (char*)evidence->pseudoEvidence->items[
                    evidence->pseudoEvidence->count].originalValue;
                if (buffer != NULL) {
                    for (size_t j = 0; j < precedenceSize; j++) {
                        charAdded = constructPseudoEvidence(
                            buffer,
                            bufferSize,
                            header,
                            evidence,
                            orderOfPrecedence[j]);
                        // charAdded == 0 means no evidence is added due to
                        // valid reasons such as missing evidence for request
                        // headers that form the pseudo header.
                        if (charAdded > 0) {
                            evidence->pseudoEvidence->items[
                                evidence->pseudoEvidence->count].field =
                                header->name;
                            evidence->pseudoEvidence->items[
                                evidence->pseudoEvidence->count].prefix =
                                orderOfPrecedence[j];
                            evidence->pseudoEvidence->count++;
                            // Once a complete pseudo evidence is found
                            // move on the next pseudo header
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
