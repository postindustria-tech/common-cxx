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
#include "Base.hpp"
#include <stdio.h>
#include <sys/stat.h>

#include "../exceptions.h"
#include "../file.h"

#ifndef _MSC_VER
#define _rmdir rmdir
#endif

static const char *tempPath = "tmp";
static const char *fileName = "tempfile";
static const char someData[] = "some data";

/**
 * File test class used to test the functionality of file.c.
 */
class File : public Base {
public:
	File() : Base() {}

protected:
	fiftyoneDegreesFilePool pool;

	/**
	 * Calls the base setup method to enable memory leak checking and memory
	 * allocation checking.
	 */
	void SetUp() {
		Base::SetUp();
		createFile(fileName);
	}

	/**
	 * Releases the file pool structure if one was created. Then calls the base
	 * teardown method to check for memory leaks and compare expected and
	 * actual memory allocations.
	 */
	void TearDown() {
		removeFile(fileName);
		Base::TearDown();
	}

	/**
	 * Initialises a file pool structure with the specified concurrency using
	 * the init method in file.c. The expected memory allocation is calculated,
	 * and the actual memory allocation is tracked. The structure is freed
	 * automatically after each test, at which point the expected and actual
	 * memory allocation is checked for equality.
	 * @param concurrency the concurrency of the file pool
	 */
	void InitPool(int16_t concurrency) {
		fiftyoneDegreesStatusCode status;
		FIFTYONE_DEGREES_EXCEPTION_CREATE
		status = fiftyoneDegreesFilePoolInit(
			&pool,
			fileName, 
			concurrency, 
			exception);
		EXPECT_TRUE(FIFTYONE_DEGREES_EXCEPTION_OKAY);
		EXPECT_EQ(FIFTYONE_DEGREES_STATUS_SUCCESS, status) <<
			"The file pool was not successfully initialised.";
	}

	/**
	 * Write a file to the file path provided using some sample data. The path
	 * to the file must exist. This method checks that the 51Degrees file
	 * operation was successful.
	 * @param file path to the file to create (including the file name)
	 */
	static void createFile(const char *file) {
		EXPECT_EQ(FIFTYONE_DEGREES_STATUS_SUCCESS,
			fiftyoneDegreesFileWrite(file, someData, strlen(someData))) <<
			"Data was not written to the file.";
	}

	/**
	 * Create a directory at the path provided. The path to the directory must
	 * exist. This method checks that the 51Degrees file operation was
	 * successful.
	 * @param dir directory path to create
	 */
	static void createDir(const char *dir) {
		fiftyoneDegreesStatusCode status =
			fiftyoneDegreesFileCreateDirectory(dir);

		EXPECT_TRUE(
			status == FIFTYONE_DEGREES_STATUS_SUCCESS ||
			status == FIFTYONE_DEGREES_STATUS_FILE_EXISTS_ERROR) <<
			"The directory could not be created";
	}

	/**
	 * Delete the file at the file path provided. The file must exist. This
	 * method checks that the 51Degrees file operation was successful.
	 * @param file path to the file to remove (including the file name)
	 */
	static void removeFile(const char *file) {
		EXPECT_EQ(FIFTYONE_DEGREES_STATUS_SUCCESS,
			fiftyoneDegreesFileDelete(file)) <<
			"The file was not deleted.";
	}

	/**
	 * Delete the directory at the path provided. The directory must exist.
	 * This method checks that the file operation was successful.
	 * @param dir directory path to delete
	 */
	static void removeDir(const char *dir) {
		EXPECT_EQ(0, _rmdir(dir)) <<
			"The temporary directory was not removed.";
	}
};

/**
 * Check that a file pool can be initialised and released without error.
 */
TEST_F(File, PoolInit) {
	int16_t concurrency = 4;
	InitPool(concurrency);
	fiftyoneDegreesFilePoolRelease(&pool);
}

/**
 * Check that a temp file can be successfully created in the same directory as
 * the master file.
 */
TEST_F(File, TempFileCreateEmptyPath) {
	const char *paths[] = { "" };
	char tempFileName[FIFTYONE_DEGREES_FILE_MAX_PATH];
	tempFileName[0] = '\0';
	EXPECT_EQ(FIFTYONE_DEGREES_STATUS_SUCCESS,
		fiftyoneDegreesFileCreateTempFile(fileName, paths, 1, tempFileName)) <<
		"A temporary file was not created.";
	EXPECT_NE('\0', tempFileName[0]) <<
		"The temporary file's path was not returned.";
	removeFile(tempFileName);
}

/**
 * Check that a temp file can be successfully created in the same directory as
 * the master file when no directories are passed to the method.
 */
TEST_F(File, TempFileCreateDefaultPath) {
	char tempFileName[FIFTYONE_DEGREES_FILE_MAX_PATH];
	tempFileName[0] = '\0';
	EXPECT_EQ(FIFTYONE_DEGREES_STATUS_SUCCESS,
		fiftyoneDegreesFileCreateTempFile(fileName, NULL, 0, tempFileName)) <<
		"A temporary file was not created.";
	EXPECT_NE('\0', tempFileName[0]) <<
		"The temporary file's path was not returned.";
	removeFile(tempFileName);
}

/**
 * Check that a temp file cannot be created in a directory which does not
 * exist.
 */
TEST_F(File, TempFileCreateInvalidPath) {
	const char *paths[] = { tempPath };
	char tempFileName[FIFTYONE_DEGREES_FILE_MAX_PATH];
	tempFileName[0] = '\0';
	EXPECT_NE(FIFTYONE_DEGREES_STATUS_SUCCESS,
		fiftyoneDegreesFileCreateTempFile(fileName, paths, 1, tempFileName)) <<
		"A missing directory was not reported.";
	EXPECT_EQ('\0', tempFileName[0]) <<
		"The temporary file's path was not returned.";
}

/**
 * Check that a temp file can be successfully created in a directory passed to
 * the method.
 */
TEST_F(File, TempFileCreateValidPath) {
	const char *paths[] = { tempPath };
	char tempFileName[FIFTYONE_DEGREES_FILE_MAX_PATH];
	tempFileName[0] = '\0';
	createDir(tempPath);
	EXPECT_EQ(FIFTYONE_DEGREES_STATUS_SUCCESS,
		fiftyoneDegreesFileCreateTempFile(fileName, paths, 1, tempFileName)) <<
		"A temporary file was not created.";
	EXPECT_NE('\0', tempFileName[0]) <<
		"The temporary file's path was not returned.";
	removeFile(tempFileName);
	removeDir(paths[0]);
}

/**
 * Check that a file can be copied successfully, and that all the data has been
 * copied.
 */
TEST_F(File, FileCopy) {
	FILE *orig, *copy;
	const char *copiedFileName = "copiedfile";
	EXPECT_EQ(FIFTYONE_DEGREES_STATUS_SUCCESS,
			fiftyoneDegreesFileCopy(fileName, copiedFileName));
	fiftyoneDegreesFileOpen(fileName, &orig);
	EXPECT_EQ(FIFTYONE_DEGREES_STATUS_SUCCESS,
		fiftyoneDegreesFileOpen(copiedFileName, &copy));
	fseek(orig, 0, SEEK_END);
	fseek(copy, 0, SEEK_END);
	EXPECT_EQ(ftell(orig), ftell(copy));
	fclose(copy);
	fclose(orig);
	removeFile(copiedFileName);
}

/**
 * Check that a directory can be created, and returns the correct status.
 */
TEST_F(File, DirectoryCreate) {
	struct stat dir;

	ASSERT_EQ(FIFTYONE_DEGREES_STATUS_SUCCESS,
		fiftyoneDegreesFileCreateDirectory(tempPath)) <<
		"The method did not report the operation successful.";
	ASSERT_TRUE(
		(stat(tempPath, &dir) == 0 &&
			(dir.st_mode & S_IFMT) == S_IFDIR)) <<
		"The directory was not created.";
	removeDir(tempPath);
}

/**
 * Check that an existing temporary file can be found by the get existing temp
 * file method, and that the path returned is correct.
 */
TEST_F(File, TempExists) {
	const char *paths[] = { tempPath };
	char tempFileName[FIFTYONE_DEGREES_FILE_MAX_PATH];
	char foundFileName[FIFTYONE_DEGREES_FILE_MAX_PATH];
	tempFileName[0] = '\0';
	// Make the temp directory to use.
	createDir(tempPath);
	EXPECT_EQ(FIFTYONE_DEGREES_STATUS_SUCCESS,
		fiftyoneDegreesFileCreateTempFile(fileName, paths, 1, tempFileName)) <<
		"A temporary file was not created.";

	EXPECT_TRUE(fiftyoneDegreesFileGetExistingTempFile(
		fileName,
		paths,
		1,
		-1,
		foundFileName)) <<
		"The existing temporary file was not found.";
	EXPECT_STREQ(tempFileName, foundFileName) <<
		"The matching file name was not written to the memory passed in.";

	removeFile(tempFileName);
	removeDir(tempPath);
}

/**
 * Check that an existing temporary file can be found and deleted by the
 * delete existing temp file method, and that the number of deleted files
 * returned is correct.
 */
TEST_F(File, TempExists_Delete) {
	const char *paths[] = { tempPath };
	char tempFileName1[FIFTYONE_DEGREES_FILE_MAX_PATH];
	char tempFileName2[FIFTYONE_DEGREES_FILE_MAX_PATH];
	char foundFileName[FIFTYONE_DEGREES_FILE_MAX_PATH];
	tempFileName1[0] = '\0';
	tempFileName2[0] = '\0';
	// Make the temp directory to use.
	createDir(tempPath);
	EXPECT_EQ(FIFTYONE_DEGREES_STATUS_SUCCESS,
		fiftyoneDegreesFileCreateTempFile(fileName, paths, 1, tempFileName1)) <<
		"A temporary file was not created.";
	EXPECT_EQ(FIFTYONE_DEGREES_STATUS_SUCCESS,
		fiftyoneDegreesFileCreateTempFile(fileName, paths, 1, tempFileName2)) <<
		"A temporary file was not created.";

	EXPECT_TRUE(fiftyoneDegreesFileGetExistingTempFile(
		fileName,
		paths,
		1,
		-1,
		foundFileName)) <<
		"The existing temporary file was not found.";

	EXPECT_EQ(2,
		fiftyoneDegreesFileDeleteUnusedTempFiles(fileName, paths, 1, -1)) <<
		"2 temporary files should have been deleted.";

	EXPECT_FALSE(fiftyoneDegreesFileGetExistingTempFile(
		fileName,
		paths,
		1,
		-1,
		foundFileName)) <<
		"The existing temporary file was not deleted.";

	removeDir(tempPath);
}

#ifndef __APPLE__

/**
 * Check that an existing temporary file (which is in use) can be found and is
 * not deleted by the delete existing temp file method, and that the number of
 * deleted files returned is correct.
 */
TEST_F(File, TempExists_DeleteInUse) {
	const char *paths[] = { tempPath };
	char tempFileName1[FIFTYONE_DEGREES_FILE_MAX_PATH];
	char tempFileName2[FIFTYONE_DEGREES_FILE_MAX_PATH];
	char foundFileName[FIFTYONE_DEGREES_FILE_MAX_PATH];
	tempFileName1[0] = '\0';
	tempFileName2[0] = '\0';
	// Make the temp directory to use.
	createDir(tempPath);
	EXPECT_EQ(FIFTYONE_DEGREES_STATUS_SUCCESS,
		fiftyoneDegreesFileCreateTempFile(fileName, paths, 1, tempFileName1)) <<
		"A temporary file was not created.";
	EXPECT_EQ(FIFTYONE_DEGREES_STATUS_SUCCESS,
		fiftyoneDegreesFileCreateTempFile(fileName, paths, 1, tempFileName2)) <<
		"A temporary file was not created.";

	EXPECT_TRUE(fiftyoneDegreesFileGetExistingTempFile(
		fileName,
		paths,
		1,
		-1,
		foundFileName)) <<
		"The existing temporary file was not found.";

	FILE *handle;
	EXPECT_EQ(FIFTYONE_DEGREES_STATUS_SUCCESS,
		fiftyoneDegreesFileOpen(foundFileName, &handle)) <<
		"The temp file could not be opened.";

	EXPECT_EQ(1,
		fiftyoneDegreesFileDeleteUnusedTempFiles(fileName, paths, 1, -1)) <<
		"1 temporary file should have been deleted.";

	EXPECT_TRUE(fiftyoneDegreesFileGetExistingTempFile(
		fileName,
		paths,
		1,
		-1,
		foundFileName)) <<
		"The existing temporary file was deleted even though it was in use.";

	fclose(handle);
	removeFile(foundFileName);
	removeDir(tempPath);
}

#endif

/**
 * Check that an existing temporary file can be found by the get existing temp
 * file method when only the first n bytes are compared, and that the path
 * returned is correct.
 */
TEST_F(File, TempExists_ComparePortion) {
	const char *paths[] = { tempPath };
	long bytesToCompare = 4;
	char alteredData[sizeof(someData)];
	strcpy(alteredData, someData);

	// Modify some of the data in altered data to make the temp file different
	// for the purposes of this test.
	alteredData[bytesToCompare]++;

	char tempFileName[FIFTYONE_DEGREES_FILE_MAX_PATH];
	char foundFileName[FIFTYONE_DEGREES_FILE_MAX_PATH];
	tempFileName[0] = '\0';
	// Make the temp directory to use.
	createDir(tempPath);
	EXPECT_EQ(FIFTYONE_DEGREES_STATUS_SUCCESS,
		fiftyoneDegreesFileCreateTempFile(fileName, paths, 1, tempFileName)) <<
		"A temporary file was not created.";
	EXPECT_EQ(FIFTYONE_DEGREES_STATUS_SUCCESS,
		fiftyoneDegreesFileWrite(
			tempFileName,
			(void*)alteredData,
			strlen(alteredData))) <<
		"Data was not written to the file.";

	EXPECT_FALSE(fiftyoneDegreesFileGetExistingTempFile(
		fileName,
		paths,
		1,
		bytesToCompare + 1,
		foundFileName)) <<
		"An incorrect temporary file was returned.";

	EXPECT_TRUE(fiftyoneDegreesFileGetExistingTempFile(
		fileName,
		paths,
		1,
		bytesToCompare,
		foundFileName)) <<
		"The existing temporary file was not found.";
	EXPECT_STREQ(tempFileName, foundFileName) <<
		"The matching file name was not written to the memory passed in.";

	removeFile(tempFileName);
	removeDir(tempPath);
}

/**
 * Check that the get existing temp file method returns false if a matching
 * file is not found.
 */
TEST_F(File, TempDoesNotExist) {
	const char *paths[] = { tempPath };
	char foundFileName[FIFTYONE_DEGREES_FILE_MAX_PATH];
	// Make the temp directory to use.
	createDir(tempPath);
	EXPECT_FALSE(fiftyoneDegreesFileGetExistingTempFile(
		fileName,
		paths,
		1,
		-1,
		foundFileName)) <<
		"An existing temporary file does not exist yet, so false should " <<
		"have been returned.";
	removeDir(tempPath);
}

/**
 * Check that the get existing temp file method returns false if a file matches
 * but is the master file passed in.
 */
TEST_F(File, TempDoesNotMatchSelf) {
	const char *paths[] = { tempPath };
	char tempFileName[FIFTYONE_DEGREES_FILE_MAX_PATH];
	char foundFileName[FIFTYONE_DEGREES_FILE_MAX_PATH];
	tempFileName[0] = '\0';
	// Make the temp directory to use.
	createDir(tempPath);
	EXPECT_EQ(FIFTYONE_DEGREES_STATUS_SUCCESS,
		fiftyoneDegreesFileCreateTempFile(fileName, paths, 1, tempFileName)) <<
		"A temporary file was not created.";

	EXPECT_FALSE(fiftyoneDegreesFileGetExistingTempFile(
		tempFileName,
		paths,
		1,
		-1,
		foundFileName)) <<
		"The same file was reported as a match for itself.";

	removeFile(tempFileName);
	removeDir(tempPath);
}

/**
 * Check that an existing file is found by the getPath method, and that the
 * path it was found at is written correctly.
 */
TEST_F(File, GetPath) {
	char relativePath[FIFTYONE_DEGREES_FILE_MAX_PATH];
	char absolutePath[FIFTYONE_DEGREES_FILE_MAX_PATH];
	char foundPath[FIFTYONE_DEGREES_FILE_MAX_PATH];
	sprintf(relativePath, "%s/%s", tempPath, fileName);
	createDir(tempPath);
	createFile(relativePath);

	EXPECT_EQ(
		FIFTYONE_DEGREES_STATUS_SUCCESS,
		fiftyoneDegreesFileGetPath(
			tempPath,
			fileName,
			foundPath,
			sizeof(foundPath))) <<
		"The file was not found.";
	
	GET_CURRENT_DIR(absolutePath, sizeof(absolutePath));
	sprintf(absolutePath + strlen(absolutePath), "/%s", relativePath);
	
	EXPECT_STREQ(absolutePath, foundPath) <<
		"The found file did not match the actual path.";
	
	removeFile(relativePath);
	removeDir(tempPath);
}

/**
 * Check that a missing file is reported correctly with the file not found
 * status.
 */
TEST_F(File, GetPath_NoFile) {
	char relativePath[FIFTYONE_DEGREES_FILE_MAX_PATH];
	char absolutePath[FIFTYONE_DEGREES_FILE_MAX_PATH];
	createDir(tempPath);
	sprintf(relativePath, "%s/%s", tempPath, fileName);

	EXPECT_EQ(
		FIFTYONE_DEGREES_STATUS_FILE_NOT_FOUND,
		fiftyoneDegreesFileGetPath(tempPath, fileName, NULL, 0)) <<
		"File not found was not reported.";

	GET_CURRENT_DIR(absolutePath, sizeof(absolutePath));
	sprintf(absolutePath + strlen(absolutePath), "/%s", relativePath);

	removeDir(tempPath);
}

/**
 * Check that a missing directory is reported correctly with the file not found
 * status.
 */
TEST_F(File, GetPath_NoDirectory) {
	char relativePath[FIFTYONE_DEGREES_FILE_MAX_PATH];
	char absolutePath[FIFTYONE_DEGREES_FILE_MAX_PATH];
	sprintf(relativePath, "%s/%s", tempPath, fileName);

	EXPECT_EQ(
		FIFTYONE_DEGREES_STATUS_FILE_NOT_FOUND,
		fiftyoneDegreesFileGetPath(tempPath, fileName, NULL, 0)) <<
		"File not found was not reported.";

	GET_CURRENT_DIR(absolutePath, sizeof(absolutePath));
	sprintf(absolutePath + strlen(absolutePath), "/%s", relativePath);
}

/**
 * Check that an insufficient memory status is returned when the memory to
 * write the found path to is not enough to store the string.
 */
TEST_F(File, GetPath_InsufficientMemory) {
	char relativePath[FIFTYONE_DEGREES_FILE_MAX_PATH];
	char foundPath[1];
	sprintf(relativePath, "%s/%s", tempPath, fileName);
	createDir(tempPath);
	createFile(relativePath);

	EXPECT_EQ(
		FIFTYONE_DEGREES_STATUS_INSUFFICIENT_MEMORY,
		fiftyoneDegreesFileGetPath(
			tempPath,
			fileName,
			foundPath,
			sizeof(foundPath))) <<
		"Insufficient memory to write the path was not reported.";

	removeFile(relativePath);
	removeDir(tempPath);
}

/**
 * Check that an exception is not thrown when a data directory parameter is
 * null.
 */
TEST_F(File, GetPath_NullPath) {
	char foundPath[FIFTYONE_DEGREES_FILE_MAX_PATH];

	EXPECT_EQ(
		FIFTYONE_DEGREES_STATUS_FILE_NOT_FOUND,
		fiftyoneDegreesFileGetPath(
			NULL,
			fileName,
			foundPath,
			sizeof(foundPath))) <<
		"A file should not have been found for a null path.";
}

/**
 * Check that an exception is not thrown when the file name parameter is null.
 */
TEST_F(File, GetPath_NullFile) {
	char foundPath[FIFTYONE_DEGREES_FILE_MAX_PATH];

	EXPECT_EQ(
		FIFTYONE_DEGREES_STATUS_FILE_NOT_FOUND,
		fiftyoneDegreesFileGetPath(
			tempPath,
			NULL,
			foundPath,
			sizeof(foundPath))) <<
		"A file should not have been found for a null file name.";
}

/**
 * Check that an insufficient memory status is reported if the parameters
 * passed result in a full path which cannot be stored in the method's array.
 */
TEST_F(File, GetPath_PathTooLong) {
	size_t i;
	char longName[FIFTYONE_DEGREES_FILE_MAX_PATH + 1];
	char foundPath[FIFTYONE_DEGREES_FILE_MAX_PATH];
	for (i = 0; i < sizeof(longName) / sizeof(char); i++) {
		longName[i] = 'A';
	}
	longName[sizeof(longName) / sizeof(char) - 1] = '\0';
	EXPECT_EQ(
		FIFTYONE_DEGREES_STATUS_INSUFFICIENT_MEMORY,
		fiftyoneDegreesFileGetPath(
			tempPath,
			longName,
			foundPath,
			sizeof(foundPath))) <<
		"Insufficient memory to store the path was not reported.";
}