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
 
#include "pch.h"
#include "EvidenceTests.hpp"

void assertStringHeaderAdded(
	fiftyoneDegreesEvidenceKeyValuePair *pair,
	const char *expectedField,
	const char *expectedValue) {
	EXPECT_EQ((int)pair->prefix, (int)FIFTYONE_DEGREES_EVIDENCE_HTTP_HEADER_STRING) <<
		L"Expected 'header' prefix.";
	EXPECT_STREQ(pair->field, expectedField) <<
		L"Expected name '" << expectedField << "' not '" << pair->field << "'";
	EXPECT_TRUE(strcmp((const char*)pair->originalValue, expectedValue) == 0) <<
		L"Expected value '" << expectedValue << "' not '" << pair->originalValue << "'";
}

/*
 * Check that a single string can be added to evidence.
 */
TEST_F(Evidence, Add_SingleString) {
	CreateEvidence(1);
	fiftyoneDegreesEvidenceAddString(
		evidence,
		FIFTYONE_DEGREES_EVIDENCE_HTTP_HEADER_STRING,
		"some-header-name",
		"some-header-value");
	assertStringHeaderAdded(&evidence->items[0], "some-header-name", "some-header-value");
}

/*
 * Check that multiple strings can be added to evidence.
 */
TEST_F(Evidence, Add_MultipleStrings)
{
	CreateEvidence(2);
	fiftyoneDegreesEvidenceAddString(
		evidence,
		FIFTYONE_DEGREES_EVIDENCE_HTTP_HEADER_STRING,
		"some-header-name",
		"some-header-value");
	fiftyoneDegreesEvidenceAddString(
		evidence,
		FIFTYONE_DEGREES_EVIDENCE_HTTP_HEADER_STRING,
		"some-other-header-name",
		"some-header-value");
	assertStringHeaderAdded(&evidence->items[0], "some-header-name", "some-header-value");
	assertStringHeaderAdded(&evidence->items[1], "some-other-header-name", "some-header-value");
}

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4100)
#endif
bool onMatchIterateString(void *state, fiftyoneDegreesEvidenceKeyValuePair *pair)
{
	if (strcmp((const char*)pair->field, "some-header-name") == 0) {
		EXPECT_TRUE(strcmp((const char*)pair->originalValue, (const char*)pair->parsedValue) == 0) <<
			L"Expected parsed value to be '" << (const char*)pair->originalValue << "' not '" <<
			(const char*)pair->parsedValue << "'";
	}
	return true;
}
#ifdef _MSC_VER
#pragma warning(pop)
#endif

/*
 *Check that the parsed version of a string evidence value will be the same string.
 */
TEST_F(Evidence, Iterate_String)
{
	CreateEvidence(1);
	fiftyoneDegreesEvidenceAddString(
		evidence,
		FIFTYONE_DEGREES_EVIDENCE_HTTP_HEADER_STRING,
		"some-header-name",
		"some-header-value");
	evidence->items[0].parsedValue = NULL;
	fiftyoneDegreesEvidenceIterate(
		evidence,
		FIFTYONE_DEGREES_EVIDENCE_HTTP_HEADER_STRING,
		NULL, 
		onMatchIterateString);
}


const char* parsedValue = "already-parsed";
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4100)
#endif
bool onMatchIterateStringAlreadyParsed(void *state, fiftyoneDegreesEvidenceKeyValuePair *pair)
{
	EXPECT_TRUE(strcmp(parsedValue, (const char*)pair->parsedValue) == 0) <<
		L"Expected parsed value to be '" << parsedValue << "' not '" <<
		(const char*)pair->parsedValue << "'";
	return true;
}
#ifdef _MSC_VER
#pragma warning(pop)
#endif

/*
 * Check that an evidence value is not parsed again if it has already been parsed.
 */
TEST_F(Evidence, Iterate_String_AlreadyParsed) {
	CreateEvidence(1);
	char* parsed = (char*)malloc(sizeof(char) * (strlen(parsedValue) + 1));
	strcpy(parsed, parsedValue);
	fiftyoneDegreesEvidenceAddString(
		evidence,
		FIFTYONE_DEGREES_EVIDENCE_HTTP_HEADER_STRING,
		"some-header-name",
		"some-header-value");
	evidence->items[0].parsedValue = parsed;
	fiftyoneDegreesEvidenceIterate(
		evidence,
		FIFTYONE_DEGREES_EVIDENCE_HTTP_HEADER_STRING,
		NULL,
		onMatchIterateStringAlreadyParsed);
	free(parsed);
}