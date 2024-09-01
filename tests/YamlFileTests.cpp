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

#include "pch.h"
#include <cstdio>
#include <string>
#include <sstream>
#include "../yamlfile.h"
#include "../fiftyone.h"

using namespace std;

// Append to a file to indicate that it should be
// cleaned at the end of the test
#define TEMP_FILE_EXT "_tmp"

typedef struct test_key_value_pair_t {
	char key[100];
	char value[100];
} testKeyValuePair;

typedef struct test_document_t {
	uint32_t count;
	testKeyValuePair pairs[3];
} testDocument;

typedef struct test_state_t {
	uint32_t count;
	testDocument documents[4];
} testState;

/*
 * Create a file name used for testing. The file
 * name will be pointed to the current path and
 * appended with '_tmp' suffix.
 * @param fileName plain file name to be created
 * @return the string object of the newly constructed
 * file name.
 */
static string* getFileWithPath(string fileName) {
	stringstream ss = stringstream("");
	ss << "./" << fileName << TEMP_FILE_EXT;
	return new string(ss.str());
}

/*
 * Create a file at the given path and given records
 * The file will end without a newline
 * @param fileNamePath path to the test file
 * @param testRecords the records will be written to
 * the file
 * @param size the number of records being written
 * @param withEOFNewLine whether a new line should
 * be appended to the end of the file
 */
static void createTestFile(string fileNamePath, string* testRecords, uint16_t size, bool withEOFNewLine)
{
	// Create a new file
	FILE* fp = fopen(fileNamePath.c_str(), "w+");

	if (fp != NULL) {
		for (int i = 0; i < size; i++) {
			if (i != 0) {
				// Write a new line if not first
				// The end result will always be
				// a file without EOF newline
				fwrite("\n", 1, 1, fp);
			}
			fwrite(testRecords[i].c_str(), testRecords[i].size(), 1, fp);
		}
	}

	if (withEOFNewLine) {
		fwrite("\n", 1, 1, fp);
	}

	// Create a new file
	fclose(fp);
}

/*
 * Close and delete a file
 * @param fileName path to the file to delete
 */
static void deleteTestFile(string fileName)
{
	remove(fileName.c_str());
}

/*
 * Callback function to count the number of records
 * being read from the file
 * @param testRecord the current record
 * @param state the test state that hold the current
 * progress of the iteration
 */
static void testCallBack(KeyValuePair *pairs, uint16_t size, void* state) {
	testState *tState = (testState*)state;
	for (int i = 0; i < size; i++) {
		testDocument *document = &tState->documents[tState->count];
		strcpy(document->pairs[document->count].key, pairs[i].key);
		strcpy(document->pairs[document->count].value, pairs[i].value);
		document->count++;
	}
	tState->count++;
}

static void testYamlFileIterator(bool eofNewLine, int limit, testDocument *expected, size_t expectedCount) {
	string* filePath = getFileWithPath(string("testfile"));
	string testRecords[11] =
	{
		string("---"),
		string("header.TestKey1: TestRecord1"),
		string("header.TestKey2: TestRecord2"),
		string("---"),
		string("---"),
		string("header.TestKey3: TestRecord3"),
		string("header.TestKey4: TestRecord4"),
		string("header.TestKey5: TestRecord5"),
		string("---"),
		string("header.TestKey6: TestRecord6"),
		string("..."),
	};
	char buffer[500] = "";
	testState state = { 0 };

	createTestFile(*filePath, testRecords, 11, eofNewLine);

	char key[4][100];
	char value[4][100];
	KeyValuePair pairs[4] = {
		{key[0], 100, value[0], 100},
		{key[1], 100, value[1], 100},
		{key[2], 100, value[2], 100},
		{key[3], 100, value[3], 100}
	};

	// Iterate through file and count the number of records
	fiftyoneDegreesYamlFileIterateWithLimit(
		filePath->c_str(),
		buffer,
		500,
		pairs,
		4,
		limit,
		&state,
		testCallBack);

	// Remove the test file
	deleteTestFile(*filePath);
	delete filePath;

	ASSERT_EQ(expectedCount, state.count) << expectedCount << " records should have been read.";
	for (uint32_t i = 0; i < state.count; i++) {
		ASSERT_EQ(expected[i].count, state.documents[i].count);
		for (uint32_t j = 0; j < state.documents[i].count; j++) {
			ASSERT_EQ(0, strcmp(expected[i].pairs[j].key, state.documents[i].pairs[j].key)) <<
				"Key is not as expected. Expected " << expected[i].pairs[j].key <<
				" but get " << state.documents[i].pairs[j].key;
			ASSERT_EQ(0, strcmp(expected[i].pairs[j].value, state.documents[i].pairs[j].value)) <<
				"Key is not as expected. Expected " << expected[i].pairs[j].value <<
				" but get " << state.documents[i].pairs[j].value;
		}
	}
}

TEST(YamlFileIteratorTests, NoLimitWithoutEOFNewLine) {
	testDocument expected[4] = {
		{
			2,
			{
				{"header.TestKey1", "TestRecord1"},
				{"header.TestKey2", "TestRecord2"}
			}
		},
		{
			0,
			{}
		},
		{
			3,
			{
				{"header.TestKey3", "TestRecord3"},
				{"header.TestKey4", "TestRecord4"},
				{"header.TestKey5", "TestRecord5"}
			}
		},
		{
			1,
			{
				{"header.TestKey6", "TestRecord6"}
			}
		}
	};
	testYamlFileIterator(false, -1, expected, 4);
}

TEST(YamlFileIteratorTests, NoLimitWithEOFNewLine) {
	testDocument expected[4] = {
		{
			2,
			{
				{"header.TestKey1", "TestRecord1"},
				{"header.TestKey2", "TestRecord2"}
			}
		},
		{
			0,
			{}
		},
		{
			3,
			{
				{"header.TestKey3", "TestRecord3"},
				{"header.TestKey4", "TestRecord4"},
				{"header.TestKey5", "TestRecord5"}
			}
		},
		{
			1,
			{
				{"header.TestKey6", "TestRecord6"}
			}
		}
	};
	testYamlFileIterator(true, -1, expected, 4);
}

TEST(YamlFileIteratorTests, Limit) {
	testDocument expected[3] = {
		{
			2,
			{
				{"header.TestKey1", "TestRecord1"},
				{"header.TestKey2", "TestRecord2"}
			}
		},
		{
			0,
			{}
		},
		{
			3,
			{
				{"header.TestKey3", "TestRecord3"},
				{"header.TestKey4", "TestRecord4"},
				{"header.TestKey5", "TestRecord5"}
			}
		}
	};
	testYamlFileIterator(false, 3, expected, 3);
}