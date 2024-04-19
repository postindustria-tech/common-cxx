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

#ifndef FIFTYONE_DEGREES_INDEXES_H_INCLUDED
#define FIFTYONE_DEGREES_INDEXES_H_INCLUDED

 /**
  * @ingroup FiftyOneDegreesCommon
  * @defgroup FiftyOneDegreesIndexes Indexes
  *
  * A look up list for profile and property index to the first value index.
  *
  * ## Introduction
  *
  * @{
  */

#include <stdint.h>
#ifdef _MSC_VER
#pragma warning (push)
#pragma warning (disable: 5105) 
#include <windows.h>
#pragma warning (default: 5105) 
#pragma warning (pop)
#endif
#include "data.h"
#include "exceptions.h"
#include "collection.h"
#include "property.h"
#include "common.h"

/**
 * Maps the profile index and the property index to the first value index of 
 * the profile for the property. Is an array of uint32_t with entries equal to 
 * the number of properties multiplied by the number of profiles.
 */
typedef struct fiftyoneDegrees_index_property_profile{
	uint32_t* valueIndexes; // array of value indexes
	uint32_t propertyCount; // number of properties
	uint32_t profileCount; // number of profiles in profileOffsets / profiles
	uint32_t size; // number elements in the valueIndexes array
	uint32_t filled; // number of elements with values
} fiftyoneDegreesIndexPropertyProfile;

/**
 * Create an index for the profiles, properties, and values provided such that
 * given the index to a property and profile the index of the first value can
 * be returned by calling fiftyoneDegreesIndexPropertyProfileLookup.
 * @param profiles collection of variable sized profiles to be indexed
 * @param profileOffsets collection of fixed offsets to profiles to be indexed
 * @param properties collection to be indexed
 * @param values collection to be indexed
 * @param exception pointer to an exception data structure to be used if an
 * exception occurs. See exceptions.h
 * @return pointer to the index memory structure
 */
EXTERNAL fiftyoneDegreesIndexPropertyProfile*
fiftyoneDegreesIndexPropertyProfileCreate(
	fiftyoneDegreesCollection* profiles,
	fiftyoneDegreesCollection* profileOffsets,
	fiftyoneDegreesCollection* properties,
	fiftyoneDegreesCollection* values,
	fiftyoneDegreesException* exception);

/**
 * Frees an index previously created by 
 * fiftyoneDegreesIndexPropertyProfileCreate.
 * @param index to be freed
 */
EXTERNAL void fiftyoneDegreesIndexPropertyProfileFree(
	fiftyoneDegreesIndexPropertyProfile* index);

/**
 * For a given profile id and property index returns the first value index, or 
 * null if a first index can not be determined from the index. The indexes 
 * relate to the collections for profiles, properties, and values provided to 
 * the fiftyoneDegreesIndexPropertyProfileCreate method when the index was 
 * created.
 * @param index to use for the lookup
 * @param profileId the values relate to
 * @param propertyIndex in the collection of properties
 * @return the index in the list of values for the profile for the first value 
 * associated with the property
 */
EXTERNAL uint32_t fiftyoneDegreesIndexPropertyProfileLookup(
	fiftyoneDegreesIndexPropertyProfile* index,
	uint32_t profileId,
	uint32_t propertyIndex);

/**
 * @}
 */

#endif