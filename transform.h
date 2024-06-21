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

#ifndef FIFTYONE_DEGREES_TRANSFORM_H_INCLUDED
#define FIFTYONE_DEGREES_TRANSFORM_H_INCLUDED

#include "status.h"
#include "evidence.h"

/**
 * User-Agent Client Hints Representation Conversion Routines
 *
 * 3 common ways to represent UACH are:
 * - HTTP header map
 * - getHighEntropyValues() JavaScript API result as a JSON string
 * - oRTB device.sua field as a JSON string
 *
 * 51degrees uses HTTP header map to represent UACH and expects it to be provided as part of the evidence as
 * HTTP headers (or same name query params).  The keys in question are:
 * - Sec-CH-UA
 * - Sec-CH-UA-Platform
 * - Sec-CH-UA-Mobile
 * - Sec-CH-UA-Model
 * - Sec-CH-UA-Full-Version-List
 * - Sec-CH-UA-Platform-Version
 *
 * 2 routines are provided to convert from corresponding JSON string representations
 * into HTTP header map:
 * - fiftyoneDegreesHeadersFromGHEV()
 * - fiftyoneDegreesHeadersFromSUA()
 *
 * Each routine takes a string as input and a pointer to a header map (evidence key value pair array) used as outptut.
 * The routines expect to work with the preallocated evidence array and use fiftyoneDegreesEvidenceAddStringUnique function for non-destructive and non-duplicating evidence output.
 * This means if f.e. sec-ch-ua was already present in the evidence earlier, then it is not going to be
 * added again or overwritten.
 * This is for a frequent case when low-entropy hints have already been determined and provided.
 *
 * Example Usage:
 *
 * // Converting a string:
 * const char *ghev = "";
 * char *buffer = fiftyoneDegreesAlloc(
 * {
 *  fiftyoneDegreesEvidenceKeyValuePairArray* evidence = fiftyoneDegreesEvidenceCreate(7);
 *  fiftyoneDegreesStatusCode statusCode = fiftyoneDegreesHeadersFromGHEV(ghev, evidence, buffer, bufferSize);
 * }
 *
 */

/**
 * Convert User-Agent Client Hints in getHighEntropyValue() API result JSON string format to
 * HTTP header representation.
 * @param ghev JSON string with the getHighEntropyValue() API result
 * @param[out] outEvidence preallocated output evidence array with remaining capacity enough to hold the produced evidence
 * @param buffer preallocated working memory buffer with length that allows to hold the converted evidence and lifetime at least as long as the output evidence object is intended to be in use, needed to store HTTP header values from the JSON string chunks
 * @param length length of th buffer
 * @return `FIFTYONE_DEGREES_STATUS_SUCCESS` if conversion succeeded, error code otherwise
 */

EXTERNAL fiftyoneDegreesStatusCode fiftyoneDegreesHeadersFromGHEV(const char *ghev,
                                                                  fiftyoneDegreesEvidenceKeyValuePairArray *outEvidence,
                                                                  char *buffer,
                                                                  size_t length);
/**
 * Convert User-Agent Client Hints in oRTB device.sua JSON string format to
 * HTTP header representation.
 * @param sua JSON string with the oRTB device.sua
 * @param[out] outEvidence preallocated output evidence array with remaining capacity enough to hold the produced evidence
 * @param buffer preallocated working memory buffer with length that allows to hold the converted evidence and lifetime at least as long as the output evidence object is intended to be in use, needed to store HTTP header values from the JSON string chunks
 * @param length length of th buffer
 * @return `FIFTYONE_DEGREES_STATUS_SUCCESS` if conversion succeeded, error code otherwise
 */

EXTERNAL fiftyoneDegreesStatusCode fiftyoneDegreesHeadersFromSUA(const char *sua,
                                                                  fiftyoneDegreesEvidenceKeyValuePairArray *outEvidence,
                                                                  char *buffer,
                                                                  size_t length);


#endif /* FIFTYONE_DEGREES_TRANSFORM_H_INCLUDED */
