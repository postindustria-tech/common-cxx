/* *********************************************************************
 * This Original Work is copyright of 51 Degrees Mobile Experts Limited.
 * Copyright 2019 51 Degrees Mobile Experts Limited, 5 Charlotte Close,
 * Caversham, Reading, Berkshire, United Kingdom RG4 7BY.
 *
 * This Original Work is licensed under the European Union Public Licence (EUPL) 
 * v.1.2 and is subject to its terms as set out below.
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

typedef struct value_search_t {
	fiftyoneDegreesCollection *strings;
	const char *valueName;
} valueSearch;

static fiftyoneDegreesString* getString(
	Collection *strings,
	uint32_t offset,
	Item *item,
	Exception *exception) {
	return StringGet(strings, offset, item, exception);
}

static int compareValueByName(void *state, Item *item, Exception *exception) {
	int result = 0;
	Item name;
	String *value;
	valueSearch *search = (valueSearch*)state;
	DataReset(&name.data);
	value = ValueGetName(
		search->strings,
		(Value*)item->data.ptr,
		&name,
		exception);
	if (value != NULL && EXCEPTION_OKAY) {
		result = strcmp(&value->value, search->valueName);
		COLLECTION_RELEASE(search->strings, &name);
	}
	return result;
}

fiftyoneDegreesString* fiftyoneDegreesValueGetName(
	fiftyoneDegreesCollection *strings,
	fiftyoneDegreesValue *value,
	fiftyoneDegreesCollectionItem *item,
	fiftyoneDegreesException *exception) {
	return getString(strings, value->nameOffset, item, exception);
}

fiftyoneDegreesString* fiftyoneDegreesValueGetDescription(
	fiftyoneDegreesCollection *strings,
	fiftyoneDegreesValue *value,
	fiftyoneDegreesCollectionItem *item,
	fiftyoneDegreesException *exception) {
	return getString(
		strings,
		value->descriptionOffset,
		item,
		exception);
}

fiftyoneDegreesString* fiftyoneDegreesValueGetUrl(
	fiftyoneDegreesCollection *strings,
	fiftyoneDegreesValue *value,
	fiftyoneDegreesCollectionItem *item,
	fiftyoneDegreesException *exception) {
	return getString(strings, value->urlOffset, item, exception);
}

fiftyoneDegreesValue* fiftyoneDegreesValueGet(
	fiftyoneDegreesCollection *values,
	uint32_t valueIndex,
	fiftyoneDegreesCollectionItem *item,
	fiftyoneDegreesException *exception) {
	return (Value*)values->get(
		values, 
		valueIndex, 
		item, 
		exception);
}

long fiftyoneDegreesValueGetIndexByName(
	fiftyoneDegreesCollection *values,
	fiftyoneDegreesCollection *strings,
	fiftyoneDegreesProperty *property,
	const char *valueName,
	fiftyoneDegreesException *exception) {
	Item item;
	valueSearch search;
	long index;
	DataReset(&item.data);
	search.valueName = valueName;
	search.strings = strings;
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

fiftyoneDegreesValue* fiftyoneDegreesValueGetByName(
	fiftyoneDegreesCollection *values,
	fiftyoneDegreesCollection *strings,
	fiftyoneDegreesProperty *property,
	const char *valueName,
	fiftyoneDegreesCollectionItem *item,
	fiftyoneDegreesException *exception) {
	valueSearch search;
	Value *value = NULL;
	search.valueName = valueName;
	search.strings = strings;
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