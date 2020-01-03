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
#include "StringCollection.hpp"
extern "C" {
#include "../headers.h"
}

/**
 * Header test class used to test the functionality of header.c using a string
 * collection built from the StringCollection class.
 */
class HeadersTests : public Base {
protected:
	StringCollection *strings = nullptr;
	int count;
	fiftyoneDegreesHeaders *headers = nullptr;
	
	/**
	 * Calls the base setup method to enable memory leak checking and memory
	 * allocation checking.
	 */
	void SetUp() {
		Base::SetUp();
	}

	/**
	 * Frees the headers structure and string collection if either were
	 * created. Then calls the base teardown method to check for memory leaks
	 * and compare expected and actual memory allocations.
	 */
	void TearDown() {
		if (headers != nullptr) {
			fiftyoneDegreesHeadersFree(headers);
		}
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
	void CreateHeaders(
		const char** headersList,
		int headersCount,
		bool expectUpperPrefixedHeaders) {
		count = headersCount;
		strings = new StringCollection(headersList, count);
		headers = fiftyoneDegreesHeadersCreate(
			expectUpperPrefixedHeaders,
			strings->getState(),
			getHeaderUniqueId);
	}
};

// ----------------------------------------------------------------------
// Check that header collection creation works properly when passed a
// collection containing a single header
// ----------------------------------------------------------------------
const char* testHeaders_Single[] = {
	"Red"
};

TEST_F(HeadersTests, Single) {
	CreateHeaders(
		testHeaders_Single,
		sizeof(testHeaders_Single) / sizeof(const char*),
		false);
	ASSERT_EQ(1, headers->count);
	fiftyoneDegreesString* str = (fiftyoneDegreesString*)headers->items[0].name.data.ptr;
	EXPECT_STREQ("Red", FIFTYONE_DEGREES_STRING(str));
}

// ----------------------------------------------------------------------
// Check that header collection creation works properly when passed a
// collection containing multiple headers
// ----------------------------------------------------------------------
const char* testHeaders_Multiple[] = {
	"Red",
	"Green",
	"Blue",
	"Yellow",
};

TEST_F(HeadersTests, Multiple) {
	CreateHeaders(
		testHeaders_Multiple,
		sizeof(testHeaders_Multiple) / sizeof(const char*),
		false);

	ASSERT_EQ(4, headers->count);
	EXPECT_STREQ("Red", FIFTYONE_DEGREES_STRING(
		(fiftyoneDegreesString*)headers->items[0].name.data.ptr));
	EXPECT_STREQ("Green", FIFTYONE_DEGREES_STRING(
		(fiftyoneDegreesString*)headers->items[1].name.data.ptr));
	EXPECT_STREQ("Blue", FIFTYONE_DEGREES_STRING(
		(fiftyoneDegreesString*)headers->items[2].name.data.ptr));
	EXPECT_STREQ("Yellow", FIFTYONE_DEGREES_STRING(
		(fiftyoneDegreesString*)headers->items[3].name.data.ptr));
}

// ----------------------------------------------------------------------
// Check that header collection creation works properly when passed a
// collection containing a single duplicate
// ----------------------------------------------------------------------
const char* testHeaders_SingleDuplicate[] = {
	"Red",
	"Red",
};

TEST_F(HeadersTests, SingleDuplicate) {
	CreateHeaders(
		testHeaders_SingleDuplicate,
		sizeof(testHeaders_SingleDuplicate) / sizeof(const char*),
		false);

	ASSERT_EQ(1, headers->count);
	EXPECT_STREQ("Red", FIFTYONE_DEGREES_STRING(
		(fiftyoneDegreesString*)headers->items[0].name.data.ptr));
}


// ----------------------------------------------------------------------
// Check that header collection creation works properly when passed a
// collection containing multiple duplicates
// ----------------------------------------------------------------------
const char* testHeaders_MultipleDuplicate[] = {
	"Green",
	"Red",
	"Red",
	"Black",
	"Green"
};

TEST_F(HeadersTests, MultipleDuplicate) {
	CreateHeaders(
		testHeaders_MultipleDuplicate,
		sizeof(testHeaders_MultipleDuplicate) / sizeof(const char*),
		false);

	ASSERT_EQ(3, headers->count);
	EXPECT_STREQ("Green", FIFTYONE_DEGREES_STRING(
		(fiftyoneDegreesString*)headers->items[0].name.data.ptr));
	EXPECT_STREQ("Red", FIFTYONE_DEGREES_STRING(
		(fiftyoneDegreesString*)headers->items[1].name.data.ptr));
	EXPECT_STREQ("Black", FIFTYONE_DEGREES_STRING(
		(fiftyoneDegreesString*)headers->items[2].name.data.ptr));
}



// ----------------------------------------------------------------------
// Check that header collection creation works properly when one of
// the headers is an empty string
// ----------------------------------------------------------------------
const char* testHeaders_EmptyString[] = {
	"Green",
	"",
	"Black",
};

TEST_F(HeadersTests, EmptyString) {
	CreateHeaders(
		testHeaders_EmptyString,
		sizeof(testHeaders_EmptyString) / sizeof(const char*),
		false);

	ASSERT_EQ(2, headers->count);
	EXPECT_STREQ("Green", FIFTYONE_DEGREES_STRING(
		(fiftyoneDegreesString*)headers->items[0].name.data.ptr));
	EXPECT_STREQ("Black", FIFTYONE_DEGREES_STRING(
		(fiftyoneDegreesString*)headers->items[1].name.data.ptr));
}

// ----------------------------------------------------------------------
// Check that header collection creation works properly when one of
// the headers is NULL
// ----------------------------------------------------------------------
const char* testHeaders_NullString[] = {
	"Green",
	NULL,
	"Black",
};

TEST_F(HeadersTests, NullString) {
	CreateHeaders(
		testHeaders_NullString,
		sizeof(testHeaders_NullString) / sizeof(const char*),
		false);

	ASSERT_EQ(2, headers->count);
	EXPECT_STREQ("Green", FIFTYONE_DEGREES_STRING(
		(fiftyoneDegreesString*)headers->items[0].name.data.ptr));
	EXPECT_STREQ("Black", FIFTYONE_DEGREES_STRING(
		(fiftyoneDegreesString*)headers->items[1].name.data.ptr));
}

// ----------------------------------------------------------------------
// Check that header collection creation works properly when two of
// the headers are the same text but different case.
// ----------------------------------------------------------------------
const char* testHeaders_Case[] = {
	"Green",
	"green",
	"Black",
};

TEST_F(HeadersTests, CheckCase) {
	CreateHeaders(
		testHeaders_Case,
		sizeof(testHeaders_Case) / sizeof(const char*),
		false);
	ASSERT_EQ(2, headers->count);
	EXPECT_STREQ("Green", FIFTYONE_DEGREES_STRING(
		(fiftyoneDegreesString*)headers->items[0].name.data.ptr));
	EXPECT_STREQ("Black", FIFTYONE_DEGREES_STRING(
		(fiftyoneDegreesString*)headers->items[1].name.data.ptr));
}


// ----------------------------------------------------------------------
// Check that header collection creation works correctly when the
// 'expectUpperPrefixedHeaders' option is enabled.
// ----------------------------------------------------------------------
const char* testHeaders_HttpPrefix[] = {
	"Red",
	"Black",
};

TEST_F(HeadersTests, HttpPrefix) {
	CreateHeaders(
		testHeaders_HttpPrefix,
		sizeof(testHeaders_HttpPrefix) / sizeof(const char*),
		true);

	ASSERT_EQ(0,
		fiftyoneDegreesHeaderGetIndex(
			headers,
			"HTTP_Red",
			strlen("HTTP_Red")));
	ASSERT_EQ(1,
		fiftyoneDegreesHeaderGetIndex(
			headers,
			"HTTP_Black",
			strlen("HTTP_Black")));
}

// ----------------------------------------------------------------------
// Check that header collection creation works correctly when a 
// collection with no headers is passed
// ----------------------------------------------------------------------
const char** testHeaders_None = new const char*[0];

TEST_F(HeadersTests, None) {
	CreateHeaders(
		testHeaders_None,
		0,
		false);
	ASSERT_EQ(0, headers->count);
}