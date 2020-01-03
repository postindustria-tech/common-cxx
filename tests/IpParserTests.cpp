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
#include "../ip.h"


bool CheckResult(byte* result, byte* expected) {
	bool match = true;
	for (int i = 0; i < (int)sizeof(expected); i++) {
		match = match && *result == *expected;
		result++;
		expected++;
	}
	return match;
}

// ------------------------------------------------------------------------------
// IPv4 tests
// ------------------------------------------------------------------------------
//TEST(ParseIp, ParseIp_Ipv4_Low)
//{
//	const char* ip = "0.0.0.0";
//	const char* end = (ip + 6);
//	fiftyoneDegreesEvidenceIpAddress* result = parseIpAddress(malloc, ip, end);
//	byte expected[] = { 0, 0, 0, 0 };
//	EXPECT_TRUE(CheckResult(result->address, expected)) <<
//		L"Expected result to be '0.0.0.0' not '" << result->address << "'";
//}
//TEST(ParseIp, ParseIp_Ipv4_High)
//{
//	byte* result = fiftyoneDegreesParseIp("255.255.255.255");
//	byte expected[] = { 255, 255, 255, 255 };
//	EXPECT_TRUE(CheckResult(result, expected),
//		L"Expected result to be '255.255.255.255'");
//}
//TEST(ParseIp, ParseIp_Ipv4_PortNumber)
//{
//	byte* result = fiftyoneDegreesParseIp("1.2.3.4:80");
//	byte expected[] = { 1, 2, 3, 4 };
//	EXPECT_TRUE(CheckResult(result, expected),
//		L"Expected result to be '1.2.3.4'");
//}
//TEST(ParseIp, ParseIp_Ipv4_CIDRFormat)
//{
//	byte* result = fiftyoneDegreesParseIp("1.2.3.4/32");
//	byte expected[] = { 1, 2, 3, 4 };
//	EXPECT_TRUE(CheckResult(result, expected),
//		L"Expected result to be '1.2.3.4'");
//}
//// ------------------------------------------------------------------------------
//// IPv6 tests
//// ------------------------------------------------------------------------------
//TEST(ParseIp, ParseIp_Ipv6_Low)
//{
//	byte* result = fiftyoneDegreesParseIp("0:0:0:0:0:0:0:0");
//	byte expected[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
//	EXPECT_TRUE(CheckResult(result, expected),
//		L"Expected result to be '0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0'");
//}
//TEST(ParseIp, ParseIp_Ipv6_Low_Abbreviated)
//{
//	byte* result = fiftyoneDegreesParseIp("::");
//	byte expected[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
//	EXPECT_TRUE(CheckResult(result, expected),
//		L"Expected result to be '0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0'");
//}
//TEST(ParseIp, ParseIp_Ipv6_High)
//{
//	byte* result = fiftyoneDegreesParseIp("FFFF:FFFF:FFFF:FFFF:FFFF:FFFF:FFFF:FFFF");
//	byte expected[] = { 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 };
//	EXPECT_TRUE(CheckResult(result, expected),
//		L"Expected result to be '255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255'");
//}
//TEST(ParseIp, ParseIp_Ipv6_AbbreviatedStart)
//{
//	byte* result = fiftyoneDegreesParseIp("::FFFF:FFFF:FFFF:FFFF");
//	byte expected[] = { 0, 0, 0, 0, 0, 0, 0, 0, 255, 255, 255, 255, 255, 255, 255, 255 };
//	EXPECT_TRUE(CheckResult(result, expected),
//		L"Expected result to be '0, 0, 0, 0, 0, 0, 0, 0, 255, 255, 255, 255, 255, 255, 255, 255'");
//}
//TEST(ParseIp, ParseIp_Ipv6_AbbreviatedMiddle)
//{
//	byte* result = fiftyoneDegreesParseIp("FFFF:FFFF::FFFF:FFFF");
//	byte expected[] = { 255, 255, 255, 255, 0, 0, 0, 0, 0, 0, 0, 0, 255, 255, 255, 255 };
//	EXPECT_TRUE(CheckResult(result, expected),
//		L"Expected result to be '255, 255, 255, 255, 0, 0, 0, 0, 0, 0, 0, 0, 255, 255, 255, 255'");
//}
//TEST(ParseIp, ParseIp_Ipv6_AbbreviatedEnd)
//{
//	byte* result = fiftyoneDegreesParseIp("FFFF:FFFF:FFFF:FFFF::");
//	byte expected[] = { 255, 255, 255, 255, 255, 255, 255, 255, 0, 0, 0, 0, 0, 0, 0, 0 };
//	EXPECT_TRUE(CheckResult(result, expected),
//		L"Expected result to be '255, 255, 255, 255, 255, 255, 255, 255, 0, 0, 0, 0, 0, 0, 0, 0'");
//}
//TEST(ParseIp, ParseIp_Ipv6_PortNumber)
//{
//	byte* result = fiftyoneDegreesParseIp("[2001::1]:80");
//	byte expected[] = { 32, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 };
//	EXPECT_TRUE(CheckResult(result, expected),
//		L"Expected result to be '32, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1'");
//}
//TEST(ParseIp, ParseIp_Ipv6_CIDRFormat)
//{
//	byte* result = fiftyoneDegreesParseIp("2001::1/128");
//	byte expected[] = { 32, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 };
//	EXPECT_TRUE(CheckResult(result, expected),
//		L"Expected result to be '32, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1'");
//}
//
//// ------------------------------------------------------------------------------
//// Invalid data
//// ------------------------------------------------------------------------------
//TEST(ParseIp, ParseIp_Invalid_ipv4OutOfRange)
//{
//	byte* result = fiftyoneDegreesParseIp("256.256.256.256");
//	Assert::IsNull(result);
//}
//TEST(ParseIp, ParseIp_Invalid_ipv4TooMany)
//{
//	byte* result = fiftyoneDegreesParseIp("1.2.3.4.5");
//	Assert::IsNull(result);
//}
//TEST(ParseIp, ParseIp_Invalid_ipv4TooFew)
//{
//	byte* result = fiftyoneDegreesParseIp("1.2.3");
//	Assert::IsNull(result);
//}
//TEST(ParseIp, ParseIp_Invalid_letters)
//{
//	byte* result = fiftyoneDegreesParseIp("a.b.c.d");
//	Assert::IsNull(result);
//}
//TEST(ParseIp, ParseIp_Invalid_emptyString)
//{
//	byte* result = fiftyoneDegreesParseIp("");
//	Assert::IsNull(result);
//}
//TEST(ParseIp, ParseIp_Invalid_null)
//{
//	byte* result = fiftyoneDegreesParseIp(NULL);
//	Assert::IsNull(result);
//}
//TEST(ParseIp, ParseIp_Invalid_Number)
//{
//	byte* result = fiftyoneDegreesParseIp("1234");
//	Assert::IsNull(result);
//}
//TEST(ParseIp, ParseIp_Invalid_ipv6OutOfRange)
//{
//	byte* result = fiftyoneDegreesParseIp("10000::1");
//	Assert::IsNull(result);
//}
//TEST(ParseIp, ParseIp_Invalid_ipv6InvalidLetter)
//{
//	byte* result = fiftyoneDegreesParseIp("GFFF::1");
//	Assert::IsNull(result);
//}
//TEST(ParseIp, ParseIp_Invalid_ipv6TooMany)
//{
//	byte* result = fiftyoneDegreesParseIp("1:1:1:1:1:1:1:1:1");
//	Assert::IsNull(result);
//}
//TEST(ParseIp, ParseIp_Invalid_ipv6TooFew)
//{
//	byte* result = fiftyoneDegreesParseIp("1:1:1:1:1:1:1");
//	Assert::IsNull(result);
//}