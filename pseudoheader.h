#ifndef FIFTYONE_DEGREES_PSEUDO_HEADER_H_INCLUDED
#define FIFTYONE_DEGREES_PSEUDO_HEADER_H_INCLUDED

#include "dataset.h"
#include "evidence.h"
#include "headers.h"


#ifdef __cplusplus
#define EXTERNAL extern "C"
#else
#define EXTERNAL
#endif

#define FIFTYONE_DEGREES_PSEUDO_HEADER_SEP '\x1F' /** unit separator of headers
                                                    and headers' values that
                                                    form pseudo header and
                                                    its evidence */

/**
 * Iterate through pseudo-headers passed in supplied parameters, construct
 * their coresponding evidence. The new evidence should be prefixed with
 * the prefix of the evidence that form it. The pseudo evidence pointed by the
 * evidence collection, should have pre-allocated the memory to hold the new
 * constructured evidence. No new evidence should be constructed if evidence
 * has already been provided in the evidence collection or there is not enough
 * values to form one.
 *
 * @param evidence pointer to the evidence that contains the real headers
 * and will be updated with the pseudo-headers.
 * @param acceptedHeaders the list of headers accepted by the
 * engine
 * @param bufferSize the size of the buffer allocated to hold the new evidence
 * pointed by the orignalValue in each pre-allocated pseudoEvidence item of
 * the evidence collection.
 * @param orderOfPrecedence of the accepted prefixes
 * @param precedenceSize the number of accepted prefixes
 * @param exception pointer to an exception data structure to be used if an
 * exception occurs. See exceptions.h.
 */
EXTERNAL void fiftyoneDegreesPseudoHeadersAddEvidence(
    fiftyoneDegreesEvidenceKeyValuePairArray* evidence,
    fiftyoneDegreesHeaders* acceptedHeaders,
    size_t bufferSize,
    const fiftyoneDegreesEvidencePrefix* orderOfPrecedence,
    size_t precedenceSize,
    fiftyoneDegreesException* exception);

/**
 * Iterate through the evidence collection and reset the pseudo-headers
 * evidence. Mainly set the field and value pointers to NULL.
 *
 * @param evidence pointer to the evidence colletection
 * @param bufferSize the size of the buffer allocated to hold the new evidence
 * pointed by the orignalValue in each pre-allocated pseudoEvidence item of
 * the evidence collection.
 */
EXTERNAL void fiftyoneDegreesPseudoHeadersRemoveEvidence(
    fiftyoneDegreesEvidenceKeyValuePairArray* evidence,
    size_t bufferSize);

#endif
