#include "PseudoHeaderTests.hpp"
#include "../pseudoheader.h"

#define PSEUDO_BUFFER_SIZE 100
#define NO_PSEUDO_HEADERS 3
#define NO_HEADERS 8
#define NO_HEADERS_NO_PSEUDO 2

// Fixed set of unique headers
static const char *uniqueHeaders[NO_HEADERS] = {
	"header1",
	"header2",
	"header3",
	"header4",
	"header5",
	"header1|header2",
	"header2|header3",
	"header1|header2|header3"
};

static const char *uniqueHeadersNoPseudoHeader[NO_HEADERS_NO_PSEUDO] = {
	"header1",
	"header2"
};

void PseudoHeaderTests::SetUp() {
	Base::SetUp();
	evidence = fiftyoneDegreesEvidenceCreate(NO_HEADERS);
	evidence->pseudoEvidence = fiftyoneDegreesEvidenceCreate(NO_PSEUDO_HEADERS);
	for (int i = 0; i < NO_PSEUDO_HEADERS; i++) {
		evidence->pseudoEvidence->items[i].originalValue =
			fiftyoneDegreesMalloc(PSEUDO_BUFFER_SIZE);
		EXPECT_TRUE(evidence->pseudoEvidence->items[i].originalValue != NULL);
		memset(
			(void*)evidence->pseudoEvidence->items[i].originalValue,
			'\0',
			PSEUDO_BUFFER_SIZE);
	}
}

void PseudoHeaderTests::TearDown() {
	for (int i = 0; i < NO_PSEUDO_HEADERS; i++) {
		fiftyoneDegreesFree(
			(void *)evidence->pseudoEvidence->items[i].originalValue);
	}
	fiftyoneDegreesHeadersFree(acceptedHeaders);
	fiftyoneDegreesEvidenceFree(evidence->pseudoEvidence);
	fiftyoneDegreesEvidenceFree(evidence);
	if (strings != nullptr) {
		delete strings;
	}
	Base::TearDown();
}

/**
	 * Create an headers structure with the specified capacity and a string
	 * collection constructed by the StringCollection class from the list of
	 * header names provided, using the create method in headers.c. The
	 * expected memory allocation is calculated, and the actual memory
	 * allocation is tracked. The structure is freed automatically after each
	 * test, at which point the expected and actual memory allocation is
	 * checked for equality.
	 */
void PseudoHeaderTests::createHeaders(
	const char** headersList,
	int headersCount,
	bool expectUpperPrefixedHeaders) {
	int count = headersCount;
	strings = new StringCollection(headersList, count);
	acceptedHeaders = fiftyoneDegreesHeadersCreate(
		expectUpperPrefixedHeaders,
		strings->getState(),
		getHeaderUniqueId);
}

void PseudoHeaderTests::addEvidence(
	testKeyValuePair* evidenceList,
	int size,
	fiftyoneDegreesEvidencePrefix prefix) {
	for (int i = 0; i < size; i++) {
		fiftyoneDegreesEvidenceAddString(
			evidence,
			prefix,
			evidenceList[i].key,
			evidenceList[i].value
		);
	}
}

void PseudoHeaderTests::checkResults(const char** expectedEvidence, int size) {
	EXPECT_EQ(size, evidence->pseudoEvidence->count) <<
		"Incorrect number of pseudo evidence constructed, here it should be" <<
		size << "\n";
	for (int i = 0; i < size; i++) {
		EXPECT_EQ(FIFTYONE_DEGREES_EVIDENCE_HTTP_HEADER_STRING,
			evidence->pseudoEvidence->items[i].prefix) <<
			"Prefix should be Http Header\n";
		EXPECT_EQ(0, strcmp(
			expectedEvidence[i],
			(const char*)evidence->pseudoEvidence->items[i].originalValue)) <<
			"Pseudo Evidence is not the same where  it should be " <<
			expectedEvidence[i] << "\n";
	}
}

void PseudoHeaderTests::removePseudoEvidence(size_t bufferSize) {
	// Test if free work correctly
	fiftyoneDegreesPseudoHeadersRemoveEvidence(evidence, bufferSize);
	EXPECT_EQ(0, evidence->pseudoEvidence->count = 0);
	for (uint32_t i = 0; i < evidence->pseudoEvidence->capacity; i++) {
		EXPECT_EQ(NULL, evidence->pseudoEvidence->items[i].field) <<
			"Field should be set to NULL\n";
		EXPECT_EQ('\0',
			((const char*)
				evidence->pseudoEvidence->items[i].originalValue)[0]) <<
			"Memory should be reset to all NULL\n";
	}
}

/*
 * Check that pseudo evidence are created correctly if pseudo headers and
 * their corresponding request evidence are present in the evidence collection.
 */
TEST_F(PseudoHeaderTests, EvidenceCreationPositiveValidInput) {
	// Expected value
	const char* expectedEvidence[3] =
	{
		"{header1@value1}{header2@value2}",
		"{header2@value2}{header3@value3}",
		"{header1@value1}{header2@value2}{header3@value3}"
	};

	// Create headers
	createHeaders(uniqueHeaders, NO_HEADERS, false);

	testKeyValuePair evidenceList[3] =
	{ {"header3", "value3"}, {"header1", "value1"}, {"header2", "value2"} };
	addEvidence(evidenceList, 3, FIFTYONE_DEGREES_EVIDENCE_HTTP_HEADER_STRING);

	FIFTYONE_DEGREES_EXCEPTION_CREATE;
	fiftyoneDegreesPseudoHeadersAddEvidence(
		evidence,
		acceptedHeaders,
		PSEUDO_BUFFER_SIZE,
		exception);
	FIFTYONE_DEGREES_EXCEPTION_THROW;

	checkResults(expectedEvidence, 3);
	removePseudoEvidence(PSEUDO_BUFFER_SIZE);
}

/*
 * Check that pseudo evidence are created correctly if only part of the request
 * evidence present in the evidence collection.
 */
TEST_F(PseudoHeaderTests, EvidenceCreationPositiveValidPartialInput) {
	// Expected value
	const char* expectedEvidence[3] =
	{
		"{header2@value2}",
		"{header2@value2}{header3@value3}",
		"{header2@value2}{header3@value3}"
	};

	// Create headers
	createHeaders(uniqueHeaders, NO_HEADERS, false);

	testKeyValuePair evidenceList[2] =
	{ {"header3", "value3"}, {"header2", "value2"}};
	addEvidence(evidenceList, 2, FIFTYONE_DEGREES_EVIDENCE_HTTP_HEADER_STRING);

	FIFTYONE_DEGREES_EXCEPTION_CREATE;
	fiftyoneDegreesPseudoHeadersAddEvidence(
		evidence,
		acceptedHeaders,
		PSEUDO_BUFFER_SIZE,
		exception);
	FIFTYONE_DEGREES_EXCEPTION_THROW;

	checkResults(expectedEvidence, 3);
	removePseudoEvidence(PSEUDO_BUFFER_SIZE);
}

/*
 * Check that if the request evidence with other prefixes than 'header' will
 * not be considered when constructing the pseudo evidence.
 */
TEST_F(PseudoHeaderTests, EvidenceCreationPositiveQueryInput) {
	// Expected value
	const char* expectedEvidence[3] =
	{
		"{header2@value2}",
		"{header2@value2}{header3@value3}",
		"{header2@value2}{header3@value3}"
	};

	// Create headers
	createHeaders(uniqueHeaders, NO_HEADERS, false);

	testKeyValuePair evidenceList[2] =
	{ {"header3", "value3"}, {"header2", "value2"} };
	addEvidence(evidenceList, 2, FIFTYONE_DEGREES_EVIDENCE_HTTP_HEADER_STRING);

	testKeyValuePair queryEvidence = { "header1", "value1" };
	addEvidence(&queryEvidence, 1, FIFTYONE_DEGREES_EVIDENCE_QUERY);

	FIFTYONE_DEGREES_EXCEPTION_CREATE;
	fiftyoneDegreesPseudoHeadersAddEvidence(
		evidence,
		acceptedHeaders,
		PSEUDO_BUFFER_SIZE,
		exception);
	FIFTYONE_DEGREES_EXCEPTION_THROW;

	checkResults(expectedEvidence, 3);
	removePseudoEvidence(PSEUDO_BUFFER_SIZE);
}

/*
 * Check that if no pseuo header are present, no pseuo evidence will be created
 */
TEST_F(PseudoHeaderTests, EvidenceCreationNoPseudoHeaders) {
	// Create headers
	createHeaders(uniqueHeadersNoPseudoHeader, NO_HEADERS_NO_PSEUDO, false);

	testKeyValuePair evidenceList[3] =
	{ {"header1", "value1"}, {"header2", "value2"}, {"header3", "value3"} };
	addEvidence(evidenceList, 3, FIFTYONE_DEGREES_EVIDENCE_HTTP_HEADER_STRING);

	FIFTYONE_DEGREES_EXCEPTION_CREATE;
	fiftyoneDegreesPseudoHeadersAddEvidence(
		evidence,
		acceptedHeaders,
		PSEUDO_BUFFER_SIZE,
		exception);
	FIFTYONE_DEGREES_EXCEPTION_THROW;

	EXPECT_EQ(0, evidence->pseudoEvidence->count) <<
		"No pseudo evidence should has been added\n";
	removePseudoEvidence(PSEUDO_BUFFER_SIZE);
}

/*
 * Check that if no request evidence are present, no pseudo evidence will be
 * created.
 */
TEST_F(PseudoHeaderTests, EvidenceCreationNoRequestHeadersForPseudoHeaders) {
	// Create headers
	createHeaders(uniqueHeaders, NO_HEADERS, false);

	testKeyValuePair evidenceList[2] =
	{ {"header4", "value4"}, {"header5", "value5"} };
	addEvidence(evidenceList, 2, FIFTYONE_DEGREES_EVIDENCE_HTTP_HEADER_STRING);

	FIFTYONE_DEGREES_EXCEPTION_CREATE;
	fiftyoneDegreesPseudoHeadersAddEvidence(
		evidence,
		acceptedHeaders,
		PSEUDO_BUFFER_SIZE,
		exception);
	FIFTYONE_DEGREES_EXCEPTION_THROW;

	EXPECT_EQ(0, evidence->pseudoEvidence->count) <<
		"No pseudo evidence should has been added\n";
	removePseudoEvidence(PSEUDO_BUFFER_SIZE);
}

/*
 * Check that if the request evidence are present with blank values, no pseudo
 * evidence will be created.
 */
TEST_F(PseudoHeaderTests, EvidenceCreationBlankRequestHeadersForPseudoHeaders) {
	// Create headers
	createHeaders(uniqueHeaders, NO_HEADERS, false);

	testKeyValuePair evidenceList[2] =
	{ {"header1", ""}, {"header2", ""} };
	addEvidence(evidenceList, 2, FIFTYONE_DEGREES_EVIDENCE_HTTP_HEADER_STRING);

	FIFTYONE_DEGREES_EXCEPTION_CREATE;
	fiftyoneDegreesPseudoHeadersAddEvidence(
		evidence,
		acceptedHeaders,
		PSEUDO_BUFFER_SIZE,
		exception);
	FIFTYONE_DEGREES_EXCEPTION_THROW;

	EXPECT_EQ(0, evidence->pseudoEvidence->count) <<
		"No pseudo evidence should has been added\n";
	removePseudoEvidence(PSEUDO_BUFFER_SIZE);
}

/*
 * Check that the PseudoHeadersAddEvidence APIs handle NULL pointer input
 * correctly.
 */
TEST_F(PseudoHeaderTests, EvidenceCreationNullPointerInput) {
	// Create headers
	createHeaders(uniqueHeaders, NO_HEADERS, false);

	// Check if exception is set correctly for NULL evidence pointer
	FIFTYONE_DEGREES_EXCEPTION_CREATE;
	fiftyoneDegreesPseudoHeadersAddEvidence(
		NULL,
		acceptedHeaders,
		PSEUDO_BUFFER_SIZE,
		exception);
	EXPECT_EQ(FIFTYONE_DEGREES_STATUS_NULL_POINTER, exception->status) <<
		"Status code should be NULL_POINTER where evidence pointer is null\n";

	// Check if exception is set correctly for NULL headers pointer
	fiftyoneDegreesPseudoHeadersAddEvidence(
		evidence,
		NULL,
		PSEUDO_BUFFER_SIZE,
		exception);
	EXPECT_EQ(FIFTYONE_DEGREES_STATUS_NULL_POINTER, exception->status) <<
		"Status code should be NULL_POINTER where headers pointer is null\n";
}

/*
 * Check that pseudo evidence that has already been provided in the evidence
 * collection will not be created again.
 */
TEST_F(PseudoHeaderTests, EvidenceCreationPseudoEvidenceAlreadyIncluded) {
	// Expected value
	const char* expectedEvidence[2] =
	{
		"{header1@value1}{header2@value2}",
		"{header1@value1}{header2@value2}{header3@value3}"
	};

	// Create headers
	createHeaders(uniqueHeaders, NO_HEADERS, false);

	testKeyValuePair evidenceList[3] =
	{ 
		{"header1", "value1"}, 
		{"header2", "value2"}, 
		{"header3", "value3"}
	};
	addEvidence(evidenceList, 3, FIFTYONE_DEGREES_EVIDENCE_HTTP_HEADER_STRING);

	testKeyValuePair queryEvidence =
		{ "header2|header3", "{header2@value2}{header3@value3}" };
	addEvidence(&queryEvidence, 1, FIFTYONE_DEGREES_EVIDENCE_QUERY);

	FIFTYONE_DEGREES_EXCEPTION_CREATE;
	fiftyoneDegreesPseudoHeadersAddEvidence(
		evidence,
		acceptedHeaders,
		PSEUDO_BUFFER_SIZE,
		exception);
	FIFTYONE_DEGREES_EXCEPTION_THROW;

	checkResults(expectedEvidence, 2);
	removePseudoEvidence(PSEUDO_BUFFER_SIZE);
}

/*
 * Check that no pseudo evidence is created if all already been provied in the
 * evidence collection.
 */
TEST_F(PseudoHeaderTests, EvidenceCreationPseudoEvidenceAllAlreadyIncluded) {
	// Create headers
	createHeaders(uniqueHeaders, NO_HEADERS, false);

	testKeyValuePair evidenceList[3] =
	{
		{"header1", "value1"},
		{"header2", "value2"},
		{"header3", "value3"}
	};
	addEvidence(evidenceList, 3, FIFTYONE_DEGREES_EVIDENCE_HTTP_HEADER_STRING);

	testKeyValuePair evidenceList2[3] =
	{
		{"header1|header2", "{header1@value1}{header2@value2}"},
		{ "header2|header3", "{header2@value2}{header3@value3}" },
		{ "header1|header2|header3", "{header1@value1}{header2@value2}{header3@value3}" }
	};
	addEvidence(evidenceList2, 3, FIFTYONE_DEGREES_EVIDENCE_QUERY);

	FIFTYONE_DEGREES_EXCEPTION_CREATE;
	fiftyoneDegreesPseudoHeadersAddEvidence(
		evidence,
		acceptedHeaders,
		PSEUDO_BUFFER_SIZE,
		exception);
	FIFTYONE_DEGREES_EXCEPTION_THROW;

	EXPECT_EQ(0, evidence->pseudoEvidence->count) <<
		"No new evidence should have been added\n";
	removePseudoEvidence(PSEUDO_BUFFER_SIZE);
}

/*
 * Check that PseudoHeadersRemoveEvidence API deal with NULL pointer input
 * correctly.
 */
TEST_F(PseudoHeaderTests, EvidenceRemoveNullPointerInput) {
	// Should not crash if evidence input is NULL
	fiftyoneDegreesPseudoHeadersRemoveEvidence(NULL, 0);

	// Should not crash if evidence how a NULL pointer for pseudo evidence
	fiftyoneDegreesEvidenceKeyValuePairArray* savePseudoEvidence =
		evidence->pseudoEvidence;
	evidence->pseudoEvidence = NULL;
	fiftyoneDegreesPseudoHeadersRemoveEvidence(evidence, PSEUDO_BUFFER_SIZE);
	evidence->pseudoEvidence = savePseudoEvidence;
}