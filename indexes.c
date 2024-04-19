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

#include "indexes.h"
#include "fiftyone.h"

// State structure when building the index.
typedef struct iterate_method_add_state_t {
	IndexPropertyProfile* index;	// index in use or null if not available
	fiftyoneDegreesCollection* values; // collection of values
} iterateMethodAddState;

// Callback method used when iterating the available profiles.
typedef void(*iterateMethod)(
	void* state, 
	Profile* profile, 
	Exception* exception);

// Loops through the values associated with the profile setting the index at 
// the position for the property and profile to the first value index from the
// profile.
static void addProfileValuesMethod(
	void* state, 
	Profile* profile, 
	Exception* exception) {
	iterateMethodAddState* s = (iterateMethodAddState*)state;
	Item valueItem; // The current value memory
	Value* value; // The current value pointer
	DataReset(&valueItem.data);
	
	uint32_t* first = (uint32_t*)(profile + 1); // First value for the profile
	int16_t nextPropertyIndex = 0; // The next property index to find a value
	uint32_t currentIndex = s->index->propertyCount * profile->profileId;

	// For each of the values associated with the profile check to see if it
	// relates to a new property index. If it does then record the first value
	// index and advance the current index to the next pointer.
	for (uint32_t i = 0; i < profile->valueCount && EXCEPTION_OKAY; i++) {
		value = s->values->get(s->values, first[i], &valueItem, exception);
		if (value != NULL && EXCEPTION_OKAY) {
			
			// If the values skip property indexes then set the value to zero
			// and move to the next one.
			while (nextPropertyIndex < value->propertyIndex) {
				s->index->valueIndexes[currentIndex] = NULL;
				currentIndex++;
				nextPropertyIndex++;
			}

			// If this is the first value for the property index then record
			// the value.
			if (value->propertyIndex == nextPropertyIndex) {
				s->index->valueIndexes[currentIndex] = first + i;
				currentIndex++;
				nextPropertyIndex++;
				s->index->filled++;
			}
			COLLECTION_RELEASE(s->values, &valueItem);
		}
	}

	// Set any remaining values to zero.
	while (nextPropertyIndex < (int16_t)s->index->propertyCount) {
		s->index->valueIndexes[currentIndex] = NULL;
		currentIndex++;
		nextPropertyIndex++;
	}
}

static void iterateProfiles(
	fiftyoneDegreesCollection* profiles,
	fiftyoneDegreesCollection* profileOffsets,
	iterateMethodAddState* state,
	iterateMethod callback,
	Exception *exception) {
	Profile* profile; // The current profile pointer
	Item profileItem; // The current profile memory
	ProfileOffset* profileOffset; // The current profile offset pointer
	Item profileOffsetItem; // The current profile offset memory
	DataReset(&profileItem.data);
	DataReset(&profileOffsetItem.data);
	for (uint32_t i = 0; 
		i < state->index->profileCount && EXCEPTION_OKAY; 
		i++) {
		profileOffset = profileOffsets->get(
			profileOffsets,
			i,
			&profileOffsetItem,
			exception);
		if (profileOffset != NULL && EXCEPTION_OKAY) {
			profile = profiles->get(
				profiles,
				profileOffset->offset,
				&profileItem,
				exception);
			if (profile != NULL && EXCEPTION_OKAY) {
				callback(state, profile, exception);
				COLLECTION_RELEASE(profiles, &profileItem);
			}
			COLLECTION_RELEASE(profileOffsets, &profileOffsetItem);
		}
	}
}

// Gets the last profile in the collection and returns the profile id. As the
// profileOffsets collection is ordered in ascending profile id this is always
// the maximum profile id.
static uint32_t getMaxProfileId(
	fiftyoneDegreesCollection* profileOffsets,
	uint32_t profileCount,
	Exception* exception) {
	uint32_t maxProfileId = 0;
	ProfileOffset* profileOffset; // The profile offset pointer
	Item profileOffsetItem; // The profile offset memory
	DataReset(&profileOffsetItem.data);
	profileOffset = profileOffsets->get(
		profileOffsets,
		profileCount - 1,
		&profileOffsetItem,
		exception);
	if (profileOffset != NULL && EXCEPTION_OKAY) {
		maxProfileId = profileOffset->profileId;
		COLLECTION_RELEASE(profileOffsets, &profileOffsetItem);
	}
	return maxProfileId;
}

fiftyoneDegreesIndexPropertyProfile*
fiftyoneDegreesIndexPropertyProfileCreate(
	fiftyoneDegreesCollection* profiles,
	fiftyoneDegreesCollection* profileOffsets,
	fiftyoneDegreesCollection* properties,
	fiftyoneDegreesCollection* values,
	fiftyoneDegreesException* exception) {

	// Get the count of available profiles.
	uint32_t profileCount = CollectionGetCount(profileOffsets);

	uint32_t maxProfileId = getMaxProfileId(
		profileOffsets, 
		profileCount,
		exception);
	if (!EXCEPTION_OKAY) {
		return NULL;
	}

	// Allocate memory for the number of properties multiplied by the maximum
	// profile id.
	IndexPropertyProfile* index = (IndexPropertyProfile*)Malloc(
		sizeof(IndexPropertyProfile));
	if (index == NULL) {
		EXCEPTION_SET(FIFTYONE_DEGREES_STATUS_INSUFFICIENT_MEMORY);
		return NULL;
	}
	index->filled = 0;
	index->profileCount = profileCount;
	index->size = (maxProfileId + 1) * properties->count;
	index->propertyCount = properties->count;
	index->valueIndexes =(uint32_t**)Malloc(sizeof(uint32_t*) * index->size);
	if (index->valueIndexes == NULL) {
		EXCEPTION_SET(FIFTYONE_DEGREES_STATUS_INSUFFICIENT_MEMORY);
		Free(index);
		return NULL;
	}

	// For each of the profiles in the collection call add the property value
	// indexes to the index array.
	iterateMethodAddState state = { index, values };
	iterateProfiles(
		profiles, 
		profileOffsets, 
		&state, 
		addProfileValuesMethod, 
		exception);

	// Return the index or free the memory if there was an exception.
	if (EXCEPTION_OKAY) {
		return index;
	}
	else {
		Free(index->valueIndexes);
		Free(index);
		return NULL;
	}
}

void fiftyoneDegreesIndexPropertyProfileFree(
	fiftyoneDegreesIndexPropertyProfile* index) {
	Free(index->valueIndexes);
	Free(index);
}

uint32_t *fiftyoneDegreesIndexPropertyProfileLookup(
	fiftyoneDegreesIndexPropertyProfile* index,
	uint32_t profileId,
	uint32_t propertyIndex) {
	uint32_t valueIndex = 
		(profileId * index->propertyCount) + propertyIndex;
	assert(valueIndex < index->size);
	return index->valueIndexes[valueIndex];
}