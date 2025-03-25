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

#include "value.h"
#include "fiftyone.h"

MAP_TYPE(Value);
MAP_TYPE(Collection);
MAP_TYPE(CollectionItem);

typedef struct value_search_t {
	Collection *strings;
	const char *valueName;
	PropertyValueType valueType;
} valueSearch;

#ifdef _MSC_VER
// Not all parameters are used for this implementation of
// #fiftyoneDegreesCollentionItemComparer
#pragma warning (disable: 4100)
#endif
static int compareValueByName(void *state, Item *item, long curIndex, Exception *exception) {
	int result = 0;
	Item name;
	StoredBinaryValue *value;
	valueSearch *search = (valueSearch*)state;
	DataReset(&name.data);
	value = ValueGetContent(
		search->strings,
		(Value*)item->data.ptr,
		search->valueType,
		&name,
		exception);
	if (value != NULL && EXCEPTION_OKAY) {
		result = StoredBinaryValueCompareWithString(
			value,
			search->valueType,
			search->valueName,
			exception);
		COLLECTION_RELEASE(search->strings, &name);
	}
	return result;
}
#ifdef _MSC_VER
#pragma warning (default: 4100)
#endif

StoredBinaryValue* fiftyoneDegreesValueGetContent(
	Collection *strings,
	Value *value,
	PropertyValueType storedValueType,
	CollectionItem *item,
	Exception *exception) {

	return StoredBinaryValueGet(strings, value->nameOffset, storedValueType, item, exception);
}

String* fiftyoneDegreesValueGetName(
	Collection *strings,
	Value *value,
	CollectionItem *item,
	Exception *exception) {
	return &StoredBinaryValueGet(
		strings,
		value->nameOffset,
		FIFTYONE_DEGREES_PROPERTY_VALUE_TYPE_STRING, // legacy contract
		item,
		exception)->stringValue;
}

String* fiftyoneDegreesValueGetDescription(
	Collection *strings,
	Value *value,
	CollectionItem *item,
	Exception *exception) {
	return &StoredBinaryValueGet(
		strings,
		value->descriptionOffset,
		FIFTYONE_DEGREES_PROPERTY_VALUE_TYPE_STRING, // description is string
		item,
		exception)->stringValue;
}

String* fiftyoneDegreesValueGetUrl(
	Collection *strings,
	Value *value,
	CollectionItem *item,
	Exception *exception) {
	return &StoredBinaryValueGet(
		strings,
		value->urlOffset,
		FIFTYONE_DEGREES_PROPERTY_VALUE_TYPE_STRING, // URL is string
		item,
		exception)->stringValue;
}

Value* fiftyoneDegreesValueGet(
	Collection *values,
	uint32_t valueIndex,
	CollectionItem *item,
	Exception *exception) {
	return (Value*)values->get(
		values, 
		valueIndex, 
		item, 
		exception);
}

long fiftyoneDegreesValueGetIndexByName(
	Collection *values,
	Collection *strings,
	Property *property,
	const char *valueName,
	Exception *exception) {

	return fiftyoneDegreesValueGetIndexByNameAndType(
		values,
		strings,
		property,
		FIFTYONE_DEGREES_PROPERTY_VALUE_TYPE_STRING, // legacy contract
		valueName,
		exception);
}

long fiftyoneDegreesValueGetIndexByNameAndType(
	Collection *values,
	Collection *strings,
	Property *property,
	fiftyoneDegreesPropertyValueType storedValueType,
	const char *valueName,
	Exception *exception) {
	Item item;
	valueSearch search;
	long index;
	DataReset(&item.data);
	search.valueName = valueName;
	search.strings = strings;
	search.valueType = storedValueType;
	index = CollectionBinarySearch(
		values,
		&item,
		property->firstValueIndex,
		property->lastValueIndex,
		(void*)&search,
		compareValueByName,
		exception);
	if (EXCEPTION_OKAY) {
		COLLECTION_RELEASE(values, &item);
	}
	return index;
}

Value* fiftyoneDegreesValueGetByName(
	Collection *values,
	Collection *strings,
	Property *property,
	const char *valueName,
	CollectionItem *item,
	Exception *exception) {

	return fiftyoneDegreesValueGetByNameAndType(
		values,
		strings,
		property,
		FIFTYONE_DEGREES_PROPERTY_VALUE_TYPE_STRING, // legacy contract
		valueName,
		item,
		exception);
}

Value* fiftyoneDegreesValueGetByNameAndType(
	Collection * const values,
	Collection * const strings,
	Property * const property,
	const fiftyoneDegreesPropertyValueType storedValueType,
	const char * const valueName,
	CollectionItem * const item,
	Exception * const exception) {
	valueSearch search;
	Value *value = NULL;
	search.valueName = valueName;
	search.strings = strings;
	search.valueType = storedValueType;
	if (
		(int)property->firstValueIndex != -1 &&
		CollectionBinarySearch(
			values,
			item,
			property->firstValueIndex,
			property->lastValueIndex,
			(void*)&search,
			compareValueByName,
			exception) >= 0 &&
		EXCEPTION_OKAY) {
		value = (Value*)item->data.ptr;
	}
	return value;
}