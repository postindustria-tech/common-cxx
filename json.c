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

#include "json.h"

// Add one character to the buffer if space is available. Always advance 
// charsAdded.
static void addChar(fiftyoneDegreesJson* s, char a) {
	if (s->bufferLength > 0) {
		*s->buffer = a;
		s->buffer++;
		s->bufferLength--;
	}
	s->charsAdded++;
}

// Add two characters to the buffer if space is available.
static void addTwo(fiftyoneDegreesJson* s, char a, char b) {
	addChar(s, a);
	addChar(s, b);
}

// Adds a string of characters escaping special characters.
static void addStringEscape(
	fiftyoneDegreesJson* s,
	const char* value,
	size_t valueLength) {
	for (int i = 0; i < valueLength; i++) {
		switch (value[i]) {
		case '\"':
			addTwo(s, '\\', '\"');
			break;
		case '\b':
			addTwo(s, '\\', 'b');
			break;
		case '\f':
			addTwo(s, '\\', 'f');
			break;
		case '\n':
			addTwo(s, '\\', 'n');
			break;
		case '\r':
			addTwo(s, '\\', 'r');
			break;
		case '\t':
			addTwo(s, '\\', 't');
			break;
		default:
			addChar(s, value[i]);
			break;
		}
	}
}

// Adds a string including surrounding double quotes and escaping special 
// characters.
static void addString(
	fiftyoneDegreesJson* s,
	const char* value,
	size_t length) {
	addChar(s, '\"');
	addStringEscape(s, value, length);
	addChar(s, '\"');
}

// Adds a comma separator.
static void addSeparator(fiftyoneDegreesJson* s) {
	addChar(s, ',');
}

void fiftyoneDegreesJsonDocumentStart(fiftyoneDegreesJson* s) {
	addChar(s, '{');
}

void fiftyoneDegreesJsonDocumentEnd(fiftyoneDegreesJson* s) {
	addChar(s, '}');
}

void fiftyoneDegreesJsonPropertySeparator(fiftyoneDegreesJson* s) {
	addSeparator(s);
}

void fiftyoneDegreesJsonPropertyStart(fiftyoneDegreesJson* s) {
	fiftyoneDegreesString* name;
	fiftyoneDegreesCollectionItem stringItem;
	fiftyoneDegreesException* exception = s->exception;

	// Check that the property is populated.
	if (s->property == NULL) {
		FIFTYONE_DEGREES_EXCEPTION_SET(
			FIFTYONE_DEGREES_STATUS_NULL_POINTER)
		return;
	}

	// Get the property name as a string.
	fiftyoneDegreesDataReset(&stringItem.data);
	name = fiftyoneDegreesStringGet(
		s->strings,
		s->property->nameOffset,
		&stringItem,
		exception);
	if (name != NULL && FIFTYONE_DEGREES_EXCEPTION_OKAY) {

		// Add the property name to the JSON buffer considering whether 
		// it's a list or single value property.
		const char* value = FIFTYONE_DEGREES_STRING(name);
		addString(s, value, strlen(value));
		addChar(s, ':');
		if (s->property->isList) {
			addChar(s, '[');
		}

		// Release the string.
		FIFTYONE_DEGREES_COLLECTION_RELEASE(s->strings, &stringItem);
	}
}

void fiftyoneDegreesJsonPropertyEnd(fiftyoneDegreesJson* s) {
	if (s->property->isList) {
		addChar(s, ']');
	}
}

void fiftyoneDegreesJsonPropertyValues(fiftyoneDegreesJson* s) {
	const char* value;
	fiftyoneDegreesException* exception = s->exception;

	// Check that the values is populated.
	if (s->values == NULL) {
		FIFTYONE_DEGREES_EXCEPTION_SET(
			FIFTYONE_DEGREES_STATUS_NULL_POINTER)
			return;
	}

	for (uint32_t i = 0; i < s->values->count; i++) {
		if (i > 0) {
			addSeparator(s);
		}
		value = FIFTYONE_DEGREES_STRING(
			(fiftyoneDegreesString*)s->values->items[i].data.ptr);
		if (value != NULL) {
			addString(s, value, strlen(value));
		}
	}
}