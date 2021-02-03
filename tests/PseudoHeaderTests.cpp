#include "PseudoHeaderTests.hpp"
#include "../pseudoheader.h"

// Fixed set of unique headers
static testString uniqueHeaders[8] = {
	{
		(uint16_t)strlen("header1"),
		"header1"
	},
	{
		(uint16_t)strlen("header2"),
		"header2"
	},
	{
		(uint16_t)strlen("header3"),
		"header3"
	},
	{
		(uint16_t)strlen("header4"),
		"header4"
	},
	{
		(uint16_t)strlen("header5"),
		"header5"
	},
	{
		(uint16_t)strlen("header1|header2"),
		"header1|header2"
	},
	{
		(uint16_t)strlen("header2|header3"),
		"header2|header3"
	},
	{
		(uint16_t)strlen("header1|header2|header3"),
		"header1|header2|header3"
	}
};

static testString uniqueHeadersNoPseudoHeader[2] = {
	{
		(uint16_t)strlen("header1"),
		"header1"
	},
	{
		(uint16_t)strlen("header2"),
		"header2"
	}
};

char pseudoEvidenceValue[3][100];
static fiftyoneDegreesCollection stringCollection;

// A dummy collection release method
static void collectionItemRelease(fiftyoneDegreesCollectionItem* item) {
	return;
}

// Return a unique header. Used when creating unique headers list.
static long getUniqueHeader(
	void* state,
	uint32_t index,
	fiftyoneDegreesCollectionItem* nameItem) {
	testHeaders* headers = (testHeaders*)state;
	if (index < headers->size) {
		nameItem->data.ptr = (byte*)&headers->headers[index];
		nameItem->collection = &stringCollection;
		stringCollection.release = collectionItemRelease;
		return index + 1000;
	}
	else {
		return -1;
	}
}

PseudoHeaderTests::PseudoHeaderTests() {
	evidence = fiftyoneDegreesEvidenceCreate(6);
	evidence->pseudoEvidence = fiftyoneDegreesEvidenceCreate(3);
	evidence->pseudoEvidence->items[0].originalValue = pseudoEvidenceValue[0];
	evidence->pseudoEvidence->items[1].originalValue = pseudoEvidenceValue[1];
	evidence->pseudoEvidence->items[2].originalValue = pseudoEvidenceValue[2];

	// Create normal headers
	testHeaders headers = { 
		sizeof(uniqueHeaders) / sizeof(testString),
		uniqueHeaders
	};
	acceptedHeaders = fiftyoneDegreesHeadersCreate(
		false,
		&headers,
		getUniqueHeader);

	// Create no pseudo headers
	headers = {
		sizeof(uniqueHeadersNoPseudoHeader) / sizeof(testString),
		uniqueHeadersNoPseudoHeader
	};
	acceptedHeadersNoPseudoHeaders = fiftyoneDegreesHeadersCreate(
		false,
		&headers,
		getUniqueHeader);
}

PseudoHeaderTests::~PseudoHeaderTests() {
	fiftyoneDegreesHeadersFree(acceptedHeaders);
	fiftyoneDegreesEvidenceFree(evidence->pseudoEvidence);
	fiftyoneDegreesEvidenceFree(evidence);
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

TEST_F(PseudoHeaderTests, EvidenceCreationPositiveValidInput) {
	// Expected value
	const char* expectedEvidence[3] =
	{
		"{header1@value1}{header2@value2}",
		"{header2@value2}{header3@value3}",
		"{header1@value1}{header2@value2}{header3@value3}"
	};

	testKeyValuePair evidenceList[3] =
	{ {"header1", "value1"}, {"header2", "value2"}, {"header3", "value3"} };
	addEvidence(evidenceList, 3, FIFTYONE_DEGREES_EVIDENCE_HTTP_HEADER_STRING);

	FIFTYONE_DEGREES_EXCEPTION_CREATE;
	fiftyoneDegreesPseudoHeadersAddEvidence(
		evidence,
		acceptedHeaders,
		100,
		exception);
	FIFTYONE_DEGREES_EXCEPTION_THROW;

	checkResults(expectedEvidence, 3);
	removePseudoEvidence(100);
}

TEST_F(PseudoHeaderTests, EvidenceCreationPositiveValidInputReverseOrder) {
	// Expected value
	const char* expectedEvidence[3] =
	{
		"{header1@value1}{header2@value2}",
		"{header2@value2}{header3@value3}",
		"{header1@value1}{header2@value2}{header3@value3}"
	};

	testKeyValuePair evidenceList[3] =
	{ {"header3", "value3"}, {"header2", "value2"}, {"header1", "value1"} };
	addEvidence(evidenceList, 3, FIFTYONE_DEGREES_EVIDENCE_HTTP_HEADER_STRING);

	FIFTYONE_DEGREES_EXCEPTION_CREATE;
	fiftyoneDegreesPseudoHeadersAddEvidence(
		evidence,
		acceptedHeaders,
		100,
		exception);
	FIFTYONE_DEGREES_EXCEPTION_THROW;

	checkResults(expectedEvidence, 3);
	removePseudoEvidence(100);
}

TEST_F(PseudoHeaderTests, EvidenceCreationPositiveValidPartialInput) {
	// Expected value
	const char* expectedEvidence[3] =
	{
		"{header2@value2}",
		"{header2@value2}{header3@value3}",
		"{header2@value2}{header3@value3}"
	};

	testKeyValuePair evidenceList[2] =
	{ {"header3", "value3"}, {"header2", "value2"}};
	addEvidence(evidenceList, 2, FIFTYONE_DEGREES_EVIDENCE_HTTP_HEADER_STRING);

	FIFTYONE_DEGREES_EXCEPTION_CREATE;
	fiftyoneDegreesPseudoHeadersAddEvidence(
		evidence,
		acceptedHeaders,
		100,
		exception);
	FIFTYONE_DEGREES_EXCEPTION_THROW;

	checkResults(expectedEvidence, 3);
	removePseudoEvidence(100);
}

TEST_F(PseudoHeaderTests, EvidenceCreationPositiveQueryInput) {
	// Expected value
	const char* expectedEvidence[3] =
	{
		"{header2@value2}",
		"{header2@value2}{header3@value3}",
		"{header2@value2}{header3@value3}"
	};

	testKeyValuePair evidenceList[2] =
	{ {"header3", "value3"}, {"header2", "value2"} };
	addEvidence(evidenceList, 2, FIFTYONE_DEGREES_EVIDENCE_HTTP_HEADER_STRING);

	testKeyValuePair queryEvidence = { "header1", "value1" };
	addEvidence(&queryEvidence, 1, FIFTYONE_DEGREES_EVIDENCE_QUERY);

	FIFTYONE_DEGREES_EXCEPTION_CREATE;
	fiftyoneDegreesPseudoHeadersAddEvidence(
		evidence,
		acceptedHeaders,
		100,
		exception);
	FIFTYONE_DEGREES_EXCEPTION_THROW;

	checkResults(expectedEvidence, 3);
	removePseudoEvidence(100);
}

TEST_F(PseudoHeaderTests, EvidenceCreationNoPseudoHeaders) {
	testKeyValuePair evidenceList[3] =
	{ {"header1", "value1"}, {"header2", "value2"}, {"header3", "value3"} };
	addEvidence(evidenceList, 3, FIFTYONE_DEGREES_EVIDENCE_HTTP_HEADER_STRING);

	FIFTYONE_DEGREES_EXCEPTION_CREATE;
	fiftyoneDegreesPseudoHeadersAddEvidence(
		evidence,
		acceptedHeadersNoPseudoHeaders,
		100,
		exception);
	FIFTYONE_DEGREES_EXCEPTION_THROW;

	EXPECT_EQ(0, evidence->pseudoEvidence->count) <<
		"No pseudo evidence should has been added\n";
	removePseudoEvidence(100);
}

TEST_F(PseudoHeaderTests, EvidenceCreationNoRequestHeadersForPseudoHeaders) {
	testKeyValuePair evidenceList[2] =
	{ {"header4", "value4"}, {"header5", "value5"} };
	addEvidence(evidenceList, 2, FIFTYONE_DEGREES_EVIDENCE_HTTP_HEADER_STRING);

	FIFTYONE_DEGREES_EXCEPTION_CREATE;
	fiftyoneDegreesPseudoHeadersAddEvidence(
		evidence,
		acceptedHeaders,
		100,
		exception);
	FIFTYONE_DEGREES_EXCEPTION_THROW;

	EXPECT_EQ(0, evidence->pseudoEvidence->count) <<
		"No pseudo evidence should has been added\n";
	removePseudoEvidence(100);
}

TEST_F(PseudoHeaderTests, EvidenceCreationBlankRequestHeadersForPseudoHeaders) {
	testKeyValuePair evidenceList[2] =
	{ {"header1", ""}, {"header2", ""} };
	addEvidence(evidenceList, 2, FIFTYONE_DEGREES_EVIDENCE_HTTP_HEADER_STRING);

	FIFTYONE_DEGREES_EXCEPTION_CREATE;
	fiftyoneDegreesPseudoHeadersAddEvidence(
		evidence,
		acceptedHeaders,
		100,
		exception);
	FIFTYONE_DEGREES_EXCEPTION_THROW;

	EXPECT_EQ(0, evidence->pseudoEvidence->count) <<
		"No pseudo evidence should has been added\n";
	removePseudoEvidence(100);
}

TEST_F(PseudoHeaderTests, EvidenceCreationNullPointerInput) {
	// Check if exception is set correctly for NULL evidence pointer
	FIFTYONE_DEGREES_EXCEPTION_CREATE;
	fiftyoneDegreesPseudoHeadersAddEvidence(
		NULL,
		acceptedHeaders,
		100,
		exception);
	EXPECT_EQ(FIFTYONE_DEGREES_STATUS_NULL_POINTER, exception->status) <<
		"Status code should be NULL_POINTER where evidence pointer is null\n";

	// Check if exception is set correctly for NULL headers pointer
	fiftyoneDegreesPseudoHeadersAddEvidence(
		evidence,
		NULL,
		100,
		exception);
	EXPECT_EQ(FIFTYONE_DEGREES_STATUS_NULL_POINTER, exception->status) <<
		"Status code should be NULL_POINTER where headers pointer is null\n";
}

TEST_F(PseudoHeaderTests, EvidenceCreationPseudoEvidenceAlreadyIncluded) {
	// Expected value
	const char* expectedEvidence[2] =
	{
		"{header1@value1}{header2@value2}",
		"{header1@value1}{header2@value2}{header3@value3}"
	};

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
		100,
		exception);
	FIFTYONE_DEGREES_EXCEPTION_THROW;

	checkResults(expectedEvidence, 2);
	removePseudoEvidence(100);
}

TEST_F(PseudoHeaderTests, EvidenceCreationPseudoEvidenceAllAlreadyIncluded) {
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
		100,
		exception);
	FIFTYONE_DEGREES_EXCEPTION_THROW;

	EXPECT_EQ(0, evidence->pseudoEvidence->count) <<
		"No new evidence should have been added\n";
	removePseudoEvidence(100);
}

TEST_F(PseudoHeaderTests, EvidenceRemoveNullPointerInput) {
	// This should not crash
	fiftyoneDegreesPseudoHeadersRemoveEvidence(NULL, 0);
}