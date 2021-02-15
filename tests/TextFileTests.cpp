#include "pch.h"
#include <cstdio>
#include <string>
#include <sstream>
#include "../textfile.h"

using namespace std;

// Append to a file to indicate that it should be
// cleaned at the end of the test
#define TEMP_FILE_EXT "_tmp"

typedef struct test_state_t {
	uint32_t count;
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

#ifdef _MSC_VER
// This is a mock definition of the callback function
// required for #fiftyoneDegreesTextFileIterate
// so not all paramters are used
#pragma warning (disable: 4100)
#endif
/*
 * Callback function to count the number of records
 * being read from the file
 * @param testRecord the current record
 * @param state the test state that hold the current
 * progress of the iteration
 */
static void testCallBack(const char* testRecord, void* state) {
	((testState *)state)->count++;
}
#ifdef _MSC_VER
#pragma warning (default: 4100)
#endif

TEST(TextFileIteratorTests, NoLimitWithoutEOFNewLine) {
	string* filePath = getFileWithPath(string("testfile"));
	string testRecords[1] = { string("TestRecord1") };
	char buffer[500] = "";
	testState state = { 0 };

	createTestFile(*filePath, testRecords, 1, false);

	// Iterate through file and count the number of records
	fiftyoneDegreesTextFileIterate(filePath->c_str(), buffer, 500, &state, testCallBack);

	// Remove the test file
	deleteTestFile(*filePath);
	delete filePath;

	ASSERT_EQ(1, state.count) << "Only one record should have been read.";
}

TEST(TextFileIteratorTests, NoLimitWithEOFNewLine) {
	string* filePath = getFileWithPath(string("testfile"));
	string testRecords[1] = { string("TestRecord1") };
	char buffer[500] = "";
	testState state = { 0 };

	createTestFile(*filePath, testRecords, 1, true);

	// Iterate through file and count the number of records
	fiftyoneDegreesTextFileIterate(filePath->c_str(), buffer, 500, &state, testCallBack);

	// Remove the test file
	deleteTestFile(*filePath);
	delete filePath;

	ASSERT_EQ(1, state.count) << "Only one record should have been read.";
}

TEST(TextFileIteratorTests, Limit) {
	string* filePath = getFileWithPath(string("testfile"));
	string testRecords[3] = { string("TestRecord1"), string("TestRecord2"), string("TestRecord3") };
	char buffer[500] = "";
	testState state = { 0 };

	createTestFile(*filePath, testRecords, 3, false);

	// Iterate through file and count the number of records
	fiftyoneDegreesTextFileIterateWithLimit(filePath->c_str(), buffer, 500, 2, &state, testCallBack);

	// Remove the test file
	deleteTestFile(*filePath);
	delete filePath;

	ASSERT_EQ(2, state.count) << "Only 2 records should have been read.";
}