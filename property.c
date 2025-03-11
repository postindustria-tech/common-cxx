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

#include "property.h"
#include "fiftyone.h"

MAP_TYPE(Collection)

#ifndef FIFTYONE_DEGREES_GET_STRING_DEFINED
#define FIFTYONE_DEGREES_GET_STRING_DEFINED
static fiftyoneDegreesString* getString(
	Collection *stringsCollection,
	uint32_t offset,
	Item *item,
	Exception *exception) {
	return &StoredBinaryValueGet(
		stringsCollection,
		offset,
		FIFTYONE_DEGREES_PROPERTY_VALUE_TYPE_STRING, // metadata are strings
		item,
		exception)->stringValue;
}
#endif

fiftyoneDegreesString* fiftyoneDegreesPropertyGetName(
	fiftyoneDegreesCollection *stringsCollection,
	fiftyoneDegreesProperty *property,
	fiftyoneDegreesCollectionItem *item,
	fiftyoneDegreesException *exception) {
	return getString(
		stringsCollection,
		property->nameOffset,
		item,
		exception);
}

static int comparePropertyTypeRecordByName(void *state, Item *item, long curIndex, Exception *exception) {
#	ifdef _MSC_VER
	UNREFERENCED_PARAMETER(curIndex);
	UNREFERENCED_PARAMETER(exception);
#	endif
	const uint32_t searchNameOffset = *(uint32_t*)state;
	const PropertyTypeRecord * const nextRecord = (PropertyTypeRecord*)item->data.ptr;
	const long long result = (long long)nextRecord->nameOffset - (long long)searchNameOffset;
	return !result ? 0 : (result < 0 ? -1 : 1);
}

PropertyValueType fiftyoneDegreesPropertyGetStoredType(
	fiftyoneDegreesCollection * const propertyTypesCollection,
	Property * const property,
	Exception * const exception) {

	PropertyValueType result = FIFTYONE_DEGREES_PROPERTY_VALUE_TYPE_STRING; // overwritten later

	Item item;
	DataReset(&item.data);
	CollectionBinarySearch(
		propertyTypesCollection,
		&item,
		0,
		CollectionGetCount(propertyTypesCollection),
		(void*)&property->nameOffset,
		comparePropertyTypeRecordByName,
		exception);
	if (EXCEPTION_OKAY) {
		result = ((PropertyTypeRecord*)item.data.ptr)->storedValueType;
		COLLECTION_RELEASE(propertyTypesCollection, &item);
	}
	return result;
}

PropertyValueType fiftyoneDegreesPropertyGetStoredTypeByIndex(
	fiftyoneDegreesCollection * const propertyTypesCollection,
	const uint32_t propertyOffset,
	Exception * const exception) {

	PropertyValueType result = FIFTYONE_DEGREES_PROPERTY_VALUE_TYPE_STRING; // overwritten later

	Item item;
	DataReset(&item.data);
	const PropertyTypeRecord * const record = (PropertyTypeRecord*)propertyTypesCollection->get(
		propertyTypesCollection,
		propertyOffset,
		&item,
		exception);
	if (EXCEPTION_OKAY) {
		result = record->storedValueType;
		COLLECTION_RELEASE(propertyTypesCollection, &item);
	}
	return result;
}

fiftyoneDegreesString* fiftyoneDegreesPropertyGetDescription(
	fiftyoneDegreesCollection *stringsCollection,
	fiftyoneDegreesProperty *property,
	fiftyoneDegreesCollectionItem *item,
	fiftyoneDegreesException *exception) {
	return getString(
		stringsCollection, 
		property->descriptionOffset,
		item, 
		exception);
}

fiftyoneDegreesString* fiftyoneDegreesPropertyGetCategory(
	fiftyoneDegreesCollection *stringsCollection,
	fiftyoneDegreesProperty *property,
	fiftyoneDegreesCollectionItem *item,
	fiftyoneDegreesException *exception) {
	return getString(
		stringsCollection, 
		property->categoryOffset, 
		item, 
		exception);
}

fiftyoneDegreesString* fiftyoneDegreesPropertyGetUrl(
	fiftyoneDegreesCollection *stringsCollection,
	fiftyoneDegreesProperty *property,
	fiftyoneDegreesCollectionItem *item,
	fiftyoneDegreesException *exception) {
	return getString(
		stringsCollection,
		property->urlOffset, 
		item,
		exception);
}

fiftyoneDegreesProperty* fiftyoneDegreesPropertyGet(
	fiftyoneDegreesCollection *properties,
	uint32_t index,
	fiftyoneDegreesCollectionItem *item,
	fiftyoneDegreesException *exception) {
	return (fiftyoneDegreesProperty*)properties->get(
		properties,
		index,
		item,
		exception);
}

fiftyoneDegreesProperty* fiftyoneDegreesPropertyGetByName(
	fiftyoneDegreesCollection *properties,
	fiftyoneDegreesCollection *strings,
	const char *requiredPropertyName,
	fiftyoneDegreesCollectionItem *item,
	fiftyoneDegreesException *exception) {
	Item propertyNameItem;
	String *name;
	Property *property = NULL;
	uint32_t i = 0;
	DataReset(&propertyNameItem.data);
	uint32_t propertiesCount = CollectionGetCount(properties);
	while (i < propertiesCount && property == NULL && EXCEPTION_OKAY) {
		
		// Get the property for this index.
		property = (Property*)properties->get(
			properties, 
			i++, 
			item, 
			exception);
		if (property != NULL && EXCEPTION_OKAY) {
			
			// Get the property name as a string for the property at this
			// index.
			name = PropertyGetName(
				strings,
				property,
				&propertyNameItem,
				exception);
			if (name != NULL) {

				// If the property name for this index doesn't match then
				// release the property and set the property pointer back to
				// zero.
				if (EXCEPTION_OKAY &&
					strcmp(&name->value, requiredPropertyName) != 0) {
					property = NULL;
					COLLECTION_RELEASE(properties, item);
				}

				// Release the property name as this is not needed again.
				COLLECTION_RELEASE(properties, &propertyNameItem);
			}
		}
	}
	return property;
}

byte fiftyoneDegreesPropertyGetValueType(
	fiftyoneDegreesCollection *properties,
	uint32_t index,
	fiftyoneDegreesException *exception) {
	byte result = 0;
	Item item;
	Property *property;
	DataReset(&item.data);
	property = (Property*)properties->get(properties, index, &item, exception);
	if (property != NULL && EXCEPTION_OKAY) {
		result = property->valueType;
		COLLECTION_RELEASE(properties, &item);
	}
	return result;
}