/* *********************************************************************
 * This Original Work is copyright of 51 Degrees Mobile Experts Limited.
 * Copyright 2025 51 Degrees Mobile Experts Limited, Davidson House,
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

#ifndef FIFTYONE_DEGREES_FILE_OFFSET_H_INCLUDED
#define FIFTYONE_DEGREES_FILE_OFFSET_H_INCLUDED

/**
 * @ingroup FiftyOneDegreesCommon
 * @defgroup FiftyOneDegreesFile FileOffset
 *
 * FileOffset offset types:
 * - signed FileOffset -- for interop with native API (fseek/ftell)
 * - unsigned UFileOffset -- as present in data file
 *
 * if FIFTYONE_DEGREES_LARGE_DATA_FILE_SUPPORT macro is defined,
 * both types will be 64-bit.
 *
 * @{
 */

#include <stdint.h>

typedef
#ifdef FIFTYONE_DEGREES_LARGE_DATA_FILE_SUPPORT
uint64_t
#else
uint32_t
#endif
fiftyoneDegreesUFileOffset; /**< Type for collection start offset (in file). [unsigned] */

typedef
#ifdef FIFTYONE_DEGREES_LARGE_DATA_FILE_SUPPORT
int64_t
#else
long
#endif
fiftyoneDegreesFileOffset; /**< Type for file offset in API. [signed] */

/**
 * @}
 */
#endif
