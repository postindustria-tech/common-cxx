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

#include "../fiftyone.h"
#include "../string.h"
#include "Base.hpp"
#include "limits.h"

constexpr size_t bufferSize = 512;

class Strings : public Base {
public:
    virtual void SetUp();
    virtual void TearDown();
    StringBuilder *builder;
    
};

void Strings::SetUp() {
    builder = (StringBuilder *) fiftyoneDegreesMalloc(sizeof(StringBuilder));
    StringBuilder local_builder = {(char *)fiftyoneDegreesMalloc(bufferSize), bufferSize};
    memcpy((void *) builder, &local_builder, sizeof(StringBuilder));
}

void Strings::TearDown() {
    fiftyoneDegreesFree(builder->ptr);
    fiftyoneDegreesFree(builder);
}

TEST_F(Strings, StringBuilderAddChar) {
    fiftyoneDegreesStringBuilderInit(builder);
    fiftyoneDegreesStringBuilderAddChar(builder, 'a');
    ASSERT_EQ(builder->added, 1);
    ASSERT_EQ(builder->remaining, bufferSize - 1);

    fiftyoneDegreesStringBuilderAddChar(builder, 'b');
    ASSERT_EQ(builder->added, 2);
    ASSERT_EQ(builder->remaining, bufferSize - 2);
    ASSERT_FALSE(builder->full);

    fiftyoneDegreesStringBuilderComplete(builder);
    ASSERT_STREQ(builder->ptr, "ab");
    ASSERT_EQ(builder->added, 3);
    ASSERT_EQ(builder->remaining, bufferSize - 3);
    ASSERT_EQ(builder->length, bufferSize);
    ASSERT_FALSE(builder->full);

    //adding a char after completion does not influence the result for it is null-terminated
    //but adds to added field
    char *current = builder->current;
    fiftyoneDegreesStringBuilderAddChar(builder, 'c');
    fiftyoneDegreesStringBuilderAddChar(builder, 'd');
    ASSERT_STREQ(builder->ptr, "ab");
    ASSERT_EQ(builder->added, 5);
    ASSERT_EQ(builder->remaining, bufferSize - 5);
    StringBuilderComplete(builder);
    ASSERT_EQ(builder->added, 6);
    ASSERT_EQ(builder->remaining, bufferSize - 6);
    ASSERT_STREQ(builder->ptr, "ab");
    ASSERT_STREQ(current, "cd");
    ASSERT_EQ(builder->length, bufferSize);
    
    ASSERT_FALSE(builder->full);
    
}

TEST_F(Strings, StringBuilderAddCharPastTheEnd) {
    fiftyoneDegreesStringBuilderInit(builder);
    for (size_t i=0; i < bufferSize + 3; ++i) {
        fiftyoneDegreesStringBuilderAddChar(builder, 'a');
    }
    
    ASSERT_EQ(builder->added, bufferSize + 3);
    ASSERT_EQ(builder->length, bufferSize);
    ASSERT_EQ(builder->remaining, 1);
    fiftyoneDegreesStringBuilderComplete(builder);
    ASSERT_EQ(strlen(builder->ptr), bufferSize - 1);
    ASSERT_TRUE(builder->full);
}

TEST_F(Strings, StringBuilderAddChars) {
    fiftyoneDegreesStringBuilderInit(builder);
    char *s = (char *) "abcdef";
    size_t len = strlen(s);
    fiftyoneDegreesStringBuilderAddChars(builder, s, len);
    
    ASSERT_EQ(builder->added, len);
    ASSERT_EQ(builder->length, bufferSize);
    ASSERT_EQ(builder->remaining, bufferSize - len);
    fiftyoneDegreesStringBuilderComplete(builder);
    ASSERT_STREQ(builder->ptr, s);
    ASSERT_EQ(strlen(builder->ptr), len);
    ASSERT_FALSE(builder->full);
}

TEST_F(Strings, StringBuilderAddCharsPastEnd) {
    fiftyoneDegreesStringBuilderInit(builder);
    char *s = (char *) "abcdef";
    size_t len = strlen(s);
    for (size_t i = 0; i < bufferSize / len + 1; i++) {
        fiftyoneDegreesStringBuilderAddChars(builder, s, len);
    }
    // 512 = 85*6 + 2 => 512 % 6 = 2
    // 512 + (6 - 2) = 516
    ASSERT_EQ(builder->added, bufferSize + (len - bufferSize % len));
    ASSERT_EQ(builder->length, bufferSize);
    
    // 512 % 6 = 2
    ASSERT_EQ(builder->remaining, bufferSize % len);
    
    // is marked as full when could not add stuff... which might be slightly incorrect,
    // since it still can fit several characters (precisely 2)
    ASSERT_TRUE(builder->full);
    
    fiftyoneDegreesStringBuilderComplete(builder);
    ASSERT_EQ(strlen(builder->ptr), bufferSize - bufferSize % len);
}

TEST_F(Strings, StringBuilderAddInteger) {
    fiftyoneDegreesStringBuilderInit(builder);
    int x = 42;
    size_t len = 2;
    
    fiftyoneDegreesStringBuilderAddInteger(builder, x);
    ASSERT_EQ(builder->added, len);
    ASSERT_EQ(builder->length, bufferSize);
    ASSERT_EQ(builder->remaining, bufferSize - len);
    
    fiftyoneDegreesStringBuilderComplete(builder);
    ASSERT_STREQ(builder->ptr, "42");
    ASSERT_EQ(builder->added, len + 1);
    ASSERT_EQ(strlen(builder->ptr), len);
    ASSERT_FALSE(builder->full);
}

TEST_F(Strings, StringBuilderAddMaxInt) {
    fiftyoneDegreesStringBuilderInit(builder);
    int x = INT_MIN;
    char buf[bufferSize];
    snprintf(buf, bufferSize, "%d", x);
    size_t len = strlen(buf);
    
    fiftyoneDegreesStringBuilderAddInteger(builder, x);
    ASSERT_EQ(builder->added, len);
    ASSERT_EQ(builder->length, bufferSize);
    ASSERT_EQ(builder->remaining, bufferSize - len);
    
    fiftyoneDegreesStringBuilderComplete(builder);
    ASSERT_STREQ(builder->ptr, buf);
    ASSERT_EQ(builder->added, len + 1);
    ASSERT_EQ(strlen(builder->ptr), len);
    ASSERT_FALSE(builder->full);
}

TEST_F(Strings, StringBuilderNullBuffer) {
    StringBuilder *prev = builder;
    StringBuilder local = {nullptr, 0};
    builder = &local;
    fiftyoneDegreesStringBuilderInit(builder);
    EXPECT_EQ(builder->remaining, 0);
    fiftyoneDegreesStringBuilderAddChars(builder, (char *) "asdf", 4);
    fiftyoneDegreesStringBuilderComplete(builder);
    EXPECT_EQ(builder->full, true);
    builder = prev;
}

TEST_F(Strings, StringBuilderOneChar) {
    StringBuilder *prev = builder;
    StringBuilder local = {(char *) fiftyoneDegreesMalloc(1), 1};
    builder = &local;
    
    fiftyoneDegreesStringBuilderInit(builder);
    EXPECT_EQ(builder->remaining, 1);
    EXPECT_EQ(builder->full, false);
    fiftyoneDegreesStringBuilderAddChars(builder, (char *) "a", 1);
    EXPECT_EQ(builder->full, true);
    fiftyoneDegreesStringBuilderComplete(builder);
    EXPECT_STREQ(builder->ptr, "");
    EXPECT_EQ(builder->full, true);
    
    fiftyoneDegreesFree(local.ptr);
    builder = prev;
}


//Some tests below were generated using OpenAI ChatGPT
TEST_F(Strings, SubString) {
    // Basic Match Test
    {
        const char *haystack = "hello world";
        EXPECT_EQ(haystack + 6, fiftyoneDegreesStringSubString(haystack, "WORLD"));
    }

    // No Match Test
    {
        const char *haystack = "hello world";
        EXPECT_EQ(nullptr, fiftyoneDegreesStringSubString(haystack, "planet"));
    }

    // Exact Match Test
    {
        const char *haystack = "OpenAI";
        EXPECT_EQ(haystack + 0, fiftyoneDegreesStringSubString(haystack, "OpenAI"));
    }

    // Different Cases Test
    {
        const char *haystack = "TeStInG";
        EXPECT_EQ(haystack + 0, fiftyoneDegreesStringSubString(haystack, "testing"));
    }

    // Empty Substring Test
    {
        const char *haystack = "something";
        EXPECT_EQ(nullptr, fiftyoneDegreesStringSubString(haystack, ""));
    }

    // Substring at Start Test
    {
        const char *haystack = "Python Programming";
        EXPECT_EQ(haystack + 0, fiftyoneDegreesStringSubString(haystack, "PYTHON"));
    }

    // Substring at End Test
    {
        const char *haystack = "CaseInsensitive";
        EXPECT_EQ(haystack + 9, fiftyoneDegreesStringSubString(haystack, "SITIVE"));
    }

    // Single Character Match Test
    {
        const char *haystack = "A";
        EXPECT_EQ(haystack + 0, fiftyoneDegreesStringSubString(haystack, "a"));
    }

    // No Match With Similar Characters Test
    {
        const char *haystack = "abcdefgh";
        EXPECT_EQ(nullptr, fiftyoneDegreesStringSubString(haystack, "ABCXYZ"));
    }

    // Substring Longer Than String Test
    {
        const char *haystack = "short";
        EXPECT_EQ(nullptr, fiftyoneDegreesStringSubString(haystack, "much longer"));
    }

    // Unicode and Special Characters Test - currently not supported :(
//    {
//        const char *haystack = "café";
//        EXPECT_EQ(haystack + 0, fiftyoneDegreesStringSubString(haystack, "CAFÉ"));
//    }

    // Only Spaces Test
    {
        const char *haystack = "   ";
        EXPECT_EQ(haystack + 0, fiftyoneDegreesStringSubString(haystack, " "));
    }

    // Substring with Mixed Alphanumeric Characters
    {
        const char *haystack = "A1B2C3";
        EXPECT_EQ(haystack + 0, fiftyoneDegreesStringSubString(haystack, "a1b2c3"));
    }

    // String Containing Symbols Test
    {
        const char *haystack = "Hello, World!";
        EXPECT_EQ(haystack + 7, fiftyoneDegreesStringSubString(haystack, "WORLD!"));
    }

    // Substring Not Present in String
    {
        const char *haystack = "OpenAI GPT";
        EXPECT_EQ(nullptr, fiftyoneDegreesStringSubString(haystack, "ChatGPT"));
    }

}

TEST_F(Strings, StringCompare) {
    // Identical Strings Test
    {
        const char *str1 = "OpenAI";
        const char *str2 = "OpenAI";
        EXPECT_EQ(0, fiftyoneDegreesStringCompare(str1, str2));
    }

    // Different Case Identical Strings Test
    {
        const char *str1 = "OpenAI";
        const char *str2 = "openai";
        EXPECT_EQ(0, fiftyoneDegreesStringCompare(str1, str2));
    }

    // Completely Different Strings Test
    {
        const char *str1 = "Hello";
        const char *str2 = "World";
        EXPECT_NE(0, fiftyoneDegreesStringCompare(str1, str2));
    }

    // Prefix Match Test
    {
        const char *str1 = "HelloWorld";
        const char *str2 = "hello";
        EXPECT_NE(0, fiftyoneDegreesStringCompare(str1, str2));
    }

    // String with Special Characters Test, no unicode support for now :(
//    {
//        const char *str1 = "café";
//        const char *str2 = "CAFÉ";
//        EXPECT_EQ(0, fiftyoneDegreesStringCompare(str1, str2));
//    }

    // Different Lengths Same Prefix Test
    {
        const char *str1 = "Hello";
        const char *str2 = "hello world";
        EXPECT_NE(0, fiftyoneDegreesStringCompare(str1, str2));
    }

    // Empty Strings Test
    {
        const char *str1 = "";
        const char *str2 = "";
        EXPECT_EQ(0, fiftyoneDegreesStringCompare(str1, str2));
    }

    // One Empty String Test
    {
        const char *str1 = "NonEmpty";
        const char *str2 = "";
        EXPECT_NE(0, fiftyoneDegreesStringCompare(str1, str2));
    }

    // Numbers in Strings Test
    {
        const char *str1 = "123abc";
        const char *str2 = "123ABC";
        EXPECT_EQ(0, fiftyoneDegreesStringCompare(str1, str2));
    }

    // Special Symbols and Punctuation Test
    {
        const char *str1 = "!@#abc";
        const char *str2 = "!@#ABC";
        EXPECT_EQ(0, fiftyoneDegreesStringCompare(str1, str2));
    }

    // Case Difference Test
    {
        const char *str1 = "aBcDeF";
        const char *str2 = "AbCdEf";
        EXPECT_EQ(0, fiftyoneDegreesStringCompare(str1, str2));
    }

//    // Non-ASCII Characters Test - no unicode support for now :(
//    {
//        const char *str1 = "über";
//        const char *str2 = "ÜBER";
//        EXPECT_EQ(0, fiftyoneDegreesStringCompare(str1, str2));
//    }

    // Substring Comparison Test
    {
        const char *str1 = "Hello";
        const char *str2 = "Hel";
        EXPECT_NE(0, fiftyoneDegreesStringCompare(str1, str2));
    }

    // Longer vs Shorter String Test
    {
        const char *str1 = "abcd";
        const char *str2 = "abcdef";
        EXPECT_NE(0, fiftyoneDegreesStringCompare(str1, str2));
    }

    // Leading Space Difference Test
    {
        const char *str1 = " Hello";
        const char *str2 = "hello";
        EXPECT_NE(0, fiftyoneDegreesStringCompare(str1, str2));
    }
}

TEST_F(Strings, StringCompareLength) {
    // Identical Strings, Full Length Test
    {
        const char *str1 = "OpenAI";
        const char *str2 = "OpenAI";
        size_t n = 6;
        EXPECT_EQ(0, fiftyoneDegreesStringCompareLength(str1, str2, n));
    }

    // Identical Strings, Partial Length Test
    {
        const char *str1 = "OpenAI";
        const char *str2 = "OpenAI";
        size_t n = 3;
        EXPECT_EQ(0, fiftyoneDegreesStringCompareLength(str1, str2, n));
    }

    // Different Case, Full Length Test
    {
        const char *str1 = "OpenAI";
        const char *str2 = "openai";
        size_t n = 6;
        EXPECT_EQ(0, fiftyoneDegreesStringCompareLength(str1, str2, n));
    }

    // Different Case, Partial Length Test
    {
        const char *str1 = "OpenAI";
        const char *str2 = "openai";
        size_t n = 4;
        EXPECT_EQ(0, fiftyoneDegreesStringCompareLength(str1, str2, n));
    }

    // Completely Different Strings Test
    {
        const char *str1 = "Hello";
        const char *str2 = "World";
        size_t n = 3;
        EXPECT_NE(0, fiftyoneDegreesStringCompareLength(str1, str2, n));
    }

    // Prefix Match, Exceeding Length Test
    {
        const char *str1 = "HelloWorld";
        const char *str2 = "hello";
        size_t n = 8;
        EXPECT_NE(0, fiftyoneDegreesStringCompareLength(str1, str2, n));
    }

    // Prefix Match, Exact Length Test
    {
        const char *str1 = "HelloWorld";
        const char *str2 = "hello";
        size_t n = 5;
        EXPECT_EQ(0, fiftyoneDegreesStringCompareLength(str1, str2, n));
    }

    // String with Special Characters Test
    {
        const char *str1 = "café";
        const char *str2 = "CAFÉ";
        size_t n = 4;
        EXPECT_EQ(0, fiftyoneDegreesStringCompareLength(str1, str2, n));
    }

    // Different Lengths, Limit Exceeds First String Test
    {
        const char *str1 = "Hello";
        const char *str2 = "hello world";
        size_t n = 10;
        EXPECT_NE(0, fiftyoneDegreesStringCompareLength(str1, str2, n));
    }

    // Empty Strings Test
    {
        const char *str1 = "";
        const char *str2 = "";
        size_t n = 0;
        EXPECT_EQ(0, fiftyoneDegreesStringCompareLength(str1, str2, n));
    }

    // One Empty String Test
    {
        const char *str1 = "NonEmpty";
        const char *str2 = "";
        size_t n = 3;
        EXPECT_NE(0, fiftyoneDegreesStringCompareLength(str1, str2, n));
    }

    // Numbers in Strings Test
    {
        const char *str1 = "123abc";
        const char *str2 = "123ABC";
        size_t n = 6;
        EXPECT_EQ(0, fiftyoneDegreesStringCompareLength(str1, str2, n));
    }

    // Special Symbols and Punctuation Test
    {
        const char *str1 = "!@#abc";
        const char *str2 = "!@#ABC";
        size_t n = 6;
        EXPECT_EQ(0, fiftyoneDegreesStringCompareLength(str1, str2, n));
    }

    // Case Difference, Short Length Test
    {
        const char *str1 = "aBcDeF";
        const char *str2 = "AbCdEf";
        size_t n = 3;
        EXPECT_EQ(0, fiftyoneDegreesStringCompareLength(str1, str2, n));
    }

//    // Non-ASCII Characters Test, unicode unsupported for now
//    {
//        const char *str1 = "über";
//        const char *str2 = "ÜBER";
//        size_t n = 4;
//        EXPECT_EQ(0, fiftyoneDegreesStringCompareLength(str1, str2, n));
//    }

    // Substring Comparison Test
    {
        const char *str1 = "Hello";
        const char *str2 = "Hel";
        size_t n = 3;
        EXPECT_EQ(0, fiftyoneDegreesStringCompareLength(str1, str2, n));
    }

    // Longer vs Shorter String Test, Limit within First String
    {
        const char *str1 = "abcdef";
        const char *str2 = "abcd";
        size_t n = 4;
        EXPECT_EQ(0, fiftyoneDegreesStringCompareLength(str1, str2, n));
    }

    // Leading Space Difference Test
    {
        const char *str1 = " Hello";
        const char *str2 = "hello";
        size_t n = 6;
        EXPECT_NE(0, fiftyoneDegreesStringCompareLength(str1, str2, n));
    }
}
