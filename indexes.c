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

typedef struct map_t {
	uint32_t availableProperty; // available property index
	int16_t propertyIndex; // index in the properties collection
} map;


typedef void(*iterateProfileValuesMethod)(
	IndexPropertyProfile* index, // index in use or null if not available
	map* propertyIndexes, // property indexes in ascending order
	fiftyoneDegreesCollection* values, // collection of values
	Profile* profile,
	Exception* exception);

// Gets the index of the profile id in the property profile index.
static uint32_t getProfileIndex(
	IndexPropertyProfile* index, 
	uint32_t profileId) {
	return profileId - index->minProfileId;
}

// Loops through the values associated with the profile setting the index at 
// the position for the property and profile to the first value index from the
// profile.
static void addProfileValuesMethod(
	IndexPropertyProfile* index, // index in use or null if not available
	map* propertyIndexes, // property indexes in ascending order
	fiftyoneDegreesCollection* values, // collection of values
	Profile* profile, 
	Exception* exception) {
	uint32_t valueIndex;
	Item valueItem; // The current value memory
	Value* value; // The current value pointer
	DataReset(&valueItem.data);
	
	uint32_t* first = (uint32_t*)(profile + 1); // First value for the profile
	uint32_t base = getProfileIndex(index, profile->profileId) * 
		index->availablePropertyCount;

	// For each of the values associated with the profile check to see if it
	// relates to a new property index. If it does then record the first value
	// index and advance the current index to the next pointer.
	for (uint32_t i = 0, p = 0;
		i < profile->valueCount &&
		p < index->availablePropertyCount &&
		EXCEPTION_OKAY;
		i++) {
		value = values->get(values, *(first + i), &valueItem, exception);
		if (value != NULL && EXCEPTION_OKAY) {

			// If the value doesn't relate to the next property index then 
			// move to the next property index.
			while (propertyIndexes[p].propertyIndex < value->propertyIndex &&
				p < index->availablePropertyCount) {
				p++;
			}

			// If the value relates to the next property index being sought 
			// then record the first value in the profile associated with the
			// property.
			if (p < index->availablePropertyCount &&
				value->propertyIndex == propertyIndexes[p].propertyIndex) {
				valueIndex = base + propertyIndexes[p].availableProperty;
				index->valueIndexes[valueIndex] = i;
				p++;
				index->filled++;
			}
			COLLECTION_RELEASE(values, &valueItem);
		}
	}
}

static void iterateProfiles(
	fiftyoneDegreesCollection* profiles,
	fiftyoneDegreesCollection* profileOffsets,
	IndexPropertyProfile* index, // index in use or null if not available
	map* propertyIndexes, // property indexes in ascending order
	fiftyoneDegreesCollection* values, // collection of values
	Exception *exception) {
	Profile* profile; // The current profile pointer
	Item profileItem; // The current profile memory
	ProfileOffset* profileOffset; // The current profile offset pointer
	Item profileOffsetItem; // The current profile offset memory
	DataReset(&profileItem.data);
	DataReset(&profileOffsetItem.data);
	for (uint32_t i = 0; 
		i < index->profileCount && EXCEPTION_OKAY;
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
				addProfileValuesMethod(
					index,
					propertyIndexes,
					values,
					profile,
					exception);
				COLLECTION_RELEASE(profiles, &profileItem);
			}
			COLLECTION_RELEASE(profileOffsets, &profileOffsetItem);
		}
	}
}

// As the profileOffsets collection is ordered in ascending profile id the 
// first and last entries are the min and max available profile ids.
static uint32_t getProfileId(
	fiftyoneDegreesCollection* profileOffsets,
	uint32_t index,
	Exception* exception) {
	uint32_t profileId = 0;
	ProfileOffset* profileOffset; // The profile offset pointer
	Item profileOffsetItem; // The profile offset memory
	DataReset(&profileOffsetItem.data);
	profileOffset = profileOffsets->get(
		profileOffsets,
		index,
		&profileOffsetItem,
		exception);
	if (profileOffset != NULL && EXCEPTION_OKAY) {
		profileId = profileOffset->profileId;
		COLLECTION_RELEASE(profileOffsets, &profileOffsetItem);
	}
	return profileId;
}

static int comparePropertyIndexes(const void* a, const void* b) {
	return ((map*)a)->propertyIndex - ((map*)b)->propertyIndex;
}

// Build an ascending ordered array of the property indexes.
static map* createPropertyIndexes(
	PropertiesAvailable* available,
	Exception* exception) {
	map* index = (map*)Malloc(sizeof(map) * available->count);
	if (index == NULL) {
		EXCEPTION_SET(FIFTYONE_DEGREES_STATUS_INSUFFICIENT_MEMORY);
		return NULL;
	}
	for (uint32_t i = 0; i < available->count; i++) {
		index[i].availableProperty = i;
		index[i].propertyIndex = (int16_t)available->items[i].propertyIndex;
	}
	qsort(index, available->count, sizeof(map*), comparePropertyIndexes);
	return index;
}


// Adds the value index to the array of value indexes in state.
static bool profileValuesAddValueCallback(void* state, uint32_t valueIndex) {
	IndexProfileValuesPtr index = (IndexProfileValuesPtr)state;
	index->items[index->count++] = valueIndex;
	assert(index->count <= index->capacity);
	return true;
}

// Adds all the value indexes to the index array for the profile. The index
// will contain sufficient capacity from a prior call to countValues.
static uint32_t profileValuesAddValues(
	fiftyoneDegreesProfile* profile,
	fiftyoneDegreesPropertiesAvailable* available,
	fiftyoneDegreesCollection* values,
	IndexProfileValuesPtr profileIndex,
	fiftyoneDegreesException* exception) {
	return ProfileIterateValueIndexes(
		profile,
		available,
		values,
		(void*)profileIndex,
		profileValuesAddValueCallback,
		exception);
}

// Does nothing other than count the value indexes.
static bool profileValuesCountCallback(void* state, uint32_t valueIndex) {
	*(uint32_t*)state = valueIndex;
	return true;
}

// Counts the value indexes for the profile that relate to the available 
// properties.
static uint32_t profileValuesCountValues(
	fiftyoneDegreesProfile* profile,
	fiftyoneDegreesPropertiesAvailable* available,
	fiftyoneDegreesCollection* values,
	fiftyoneDegreesException* exception) {
	uint32_t lastValueIndex; // Needed to prevent compiler warning.
	return ProfileIterateValueIndexes(
		profile,
		available,
		values,
		&lastValueIndex,
		profileValuesCountCallback,
		exception);
}

// Creates the initial memory needed for the profile values index. Sets the 
// entries to null to handle situations where the profile id is not valid.
static IndexAllProfileValues* profileValuesCreateIndex(
	fiftyoneDegreesCollection* profileOffsets,
	Exception* exception) {
	IndexAllProfileValues* index;

	// Get the minimum and maximum profile id.
	uint32_t minProfileId = getProfileId(profileOffsets, 0, exception);
	uint32_t maxProfileId = getProfileId(
		profileOffsets,
		CollectionGetCount(profileOffsets) - 1,
		exception);

	// Create the index with sufficient capacity for all possible profile ids.
	FIFTYONE_DEGREES_ARRAY_CREATE(
		IndexProfileValuesPtr,
		index,
		maxProfileId - minProfileId + 1);
	if (index == NULL || EXCEPTION_FAILED) {
		return NULL;
	}
	index->minProfileId = minProfileId;
	index->maxProfileId = maxProfileId;
	index->profileCount = CollectionGetCount(profileOffsets);

	// Set all the possible items to NULL and zero to ensure invalid profile 
	// ids can be identified.
	for (uint32_t i = 0; i < index->capacity; i++) {
		index->items[i] = NULL;
	}

	return index;
}

fiftyoneDegreesIndexPropertyProfile*
fiftyoneDegreesIndexPropertyProfileCreate(
	fiftyoneDegreesCollection* profiles,
	fiftyoneDegreesCollection* profileOffsets,
	fiftyoneDegreesPropertiesAvailable* available,
	fiftyoneDegreesCollection* values,
	fiftyoneDegreesException* exception) {

	// Create the ordered list of property indexes.
	map* propertyIndexes = createPropertyIndexes(available, exception);
	if (propertyIndexes == NULL) {
		return NULL;
	}

	// Allocate memory for the index and set the fields.
	IndexPropertyProfile* index = (IndexPropertyProfile*)Malloc(
		sizeof(IndexPropertyProfile));
	if (index == NULL) {
		EXCEPTION_SET(FIFTYONE_DEGREES_STATUS_INSUFFICIENT_MEMORY);
		return NULL;
	}
	index->filled = 0;
	index->profileCount = CollectionGetCount(profileOffsets);
	index->minProfileId = getProfileId(profileOffsets, 0, exception);
	if (!EXCEPTION_OKAY) {
		Free(index);
		Free(propertyIndexes);
		return NULL;
	}
	index->maxProfileId = getProfileId(
		profileOffsets,
		index->profileCount - 1,
		exception);
	if (!EXCEPTION_OKAY) {
		Free(index);
		Free(propertyIndexes);
		return NULL;
	}
	index->availablePropertyCount = available->count;
	index->size = (index->maxProfileId - index->minProfileId + 1) * 
		available->count;
	
	// Allocate memory for the values index and set the fields.
	index->valueIndexes =(uint32_t*)Malloc(sizeof(uint32_t) * index->size);
	if (index->valueIndexes == NULL) {
		EXCEPTION_SET(FIFTYONE_DEGREES_STATUS_INSUFFICIENT_MEMORY);
		Free(index);
		Free(propertyIndexes);
		return NULL;
	}

	// For each of the profiles in the collection call add the property value
	// indexes to the index array.
	iterateProfiles(
		profiles, 
		profileOffsets, 
		index, 
		propertyIndexes,
		values,
		exception);
	Free(propertyIndexes);

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

uint32_t fiftyoneDegreesIndexPropertyProfileLookup(
	fiftyoneDegreesIndexPropertyProfile* index,
	uint32_t profileId,
	uint32_t availablePropertyIndex) {
	uint32_t valueIndex = 
		(getProfileIndex(index, profileId) * index->availablePropertyCount) + 
		availablePropertyIndex;
	assert(valueIndex < index->size);
	return index->valueIndexes[valueIndex];
}

fiftyoneDegreesIndexAllProfileValues* fiftyoneDegreesIndexProfileValuesCreate(
	fiftyoneDegreesCollection* profiles,
	fiftyoneDegreesCollection* profileOffsets,
	fiftyoneDegreesPropertiesAvailable* available,
	fiftyoneDegreesCollection* values,
	fiftyoneDegreesException* exception) {
	Item profileItem;
	Profile* profile;
	uint32_t count;
	int profileIndex;
	DataReset(&profileItem.data);

	// Create the array that forms the index for all profiles.
	IndexAllProfileValues* index = profileValuesCreateIndex(
		profileOffsets, 
		exception);
	if (index == NULL || EXCEPTION_FAILED) {
		return NULL;
	}

	// For each of the profile ids in the profile offsets.
	for (uint32_t i = 0; i < index->profileCount; i++) {

		// Get the next profile based on the index.
		profile = (Profile*)ProfileGetByIndex(
			profileOffsets,
			profiles,
			i,
			&profileItem,
			exception);
		if (profile == NULL || EXCEPTION_FAILED) {
			IndexProfileValuesFree(index);
			return NULL;
		}

		// Count the number of required property values that are going to be 
		// needed for this profile.
		count = profileValuesCountValues(
			profile, 
			available,
			values,
			exception);
		if (EXCEPTION_FAILED) {
			IndexProfileValuesFree(index);
			return NULL;
		}

		// Allocate an array of sufficient size to store all the value indexes.
		profileIndex = profile->profileId - index->minProfileId;
		FIFTYONE_DEGREES_ARRAY_CREATE(
			IndexProfileValue,
			index->items[profileIndex],
			count);
		if (index->items[profileIndex] == NULL || EXCEPTION_FAILED) {
			IndexProfileValuesFree(index);
			return NULL;
		}
		assert(index->items[profileIndex]->capacity == count);

		// Add the value indexes for each of the available properties for the
		// profile.
		profileValuesAddValues(
			profile, 
			available, 
			values, 
			index->items[profileIndex],
			exception);
		assert(index->items[profileIndex]->capacity ==
			index->items[profileIndex]->count);
		index->count++;

		// Release the profile.
		COLLECTION_RELEASE(profiles, &profileItem);
	}

	return index;
}

void fiftyoneDegreesIndexProfileValuesFree(
	fiftyoneDegreesIndexAllProfileValues* index) {
	for (uint32_t i = 0; i < index->capacity; i++) {
		if (index->items[i] != NULL) {
			Free(index->items[i]);
		}
	}
	Free(index);
}

uint32_t fiftyoneDegreesIndexProfileValuesIterate(
	fiftyoneDegreesIndexAllProfileValues* index,
	uint32_t profileId, 
	void* state, 
	fiftyoneDegreesIndexProfileValuesMethod callback) {
	IndexProfileValuesPtr profileIndex;
	bool cont = true;
	uint32_t count = 0;

	// Check that the profile id is valid considering the min and max ids.
	if (profileId >= index->minProfileId && profileId <= index->maxProfileId) {
		
		// Get the profile values index and check it is valid.
		profileIndex = index->items[profileId - index->minProfileId];
		if (profileIndex != NULL) {

			// Callback with all the available value indexes for the profile.
			for (uint32_t i = 0; cont && i < profileIndex->count; i++) {
				cont = callback(profileIndex->items[i], state);
				count++;
			}
		}
	}
	return count;
}