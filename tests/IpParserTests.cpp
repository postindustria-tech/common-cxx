/* *********************************************************************
 * This Original Work is copyright of 51 Degrees Mobile Experts Limited.
 * Copyright 2022 51 Degrees Mobile Experts Limited, Davidson House,
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
#include "../ip.h"


bool CheckResult(byte* result, byte* expected, uint16_t size) {
	bool match = true;
	for (uint16_t i = 0; i < size; i++) {
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
TEST(ParseIp, ParseIp_Ipv6_AbbreviatedStart)
{
	const char *ipv6AbbreviatedStart = "::FFFF:FFFF:FFFF:FFFF";
	byte expected[FIFTYONE_DEGREES_IPV6_LENGTH] = 
		{ 0, 0, 0, 0, 0, 0, 0, 0, 255, 255, 255, 255, 255, 255, 255, 255 };

	fiftyoneDegreesEvidenceIpAddress* result = 
		fiftyoneDegreesIpParseAddress(
			malloc,
			ipv6AbbreviatedStart,
			ipv6AbbreviatedStart + strlen(ipv6AbbreviatedStart));

	EXPECT_TRUE(result != NULL) << "Abbreviated start IPv6 address "
		"should be successfull parsed, where the address is " <<
		ipv6AbbreviatedStart << ".";

	EXPECT_TRUE(result->type == FIFTYONE_DEGREES_EVIDENCE_IP_TYPE_IPV6) <<
		"IP address version was not identified correctly where where the " <<
		"IP address is " << ipv6AbbreviatedStart << ".";

	EXPECT_TRUE(
		CheckResult(result->address, expected, FIFTYONE_DEGREES_IPV6_LENGTH)) << 
		"The value of the abbreivated start IPv6 address is not correctly parsed.";
}
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
TEST(ParseIp, ParseIp_Invalid_ipv4OutOfRange)
{
	const char *ipv4OutOfRange = "256.256.256.256";
	byte expected[FIFTYONE_DEGREES_IPV4_LENGTH] = 
		{255, 255, 255, 255};

	fiftyoneDegreesEvidenceIpAddress* result = 
		fiftyoneDegreesIpParseAddress(
			malloc,
			ipv4OutOfRange,
			ipv4OutOfRange + strlen(ipv4OutOfRange));

	EXPECT_TRUE(result != NULL) << "Out of range IPv4 address "
		"should be successfull parsed, where the address is " <<
		ipv4OutOfRange << ".";

	EXPECT_TRUE(result->type == FIFTYONE_DEGREES_EVIDENCE_IP_TYPE_IPV4) <<
		"IP address version was not identified correctly where where the " <<
		"IP address is " << ipv4OutOfRange << ".";

	EXPECT_TRUE(
		CheckResult(result->address, expected, FIFTYONE_DEGREES_IPV4_LENGTH)) <<
		"The value of the out of range IPv4 address is not correctly restricted "
		"at 255.";
}
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
//// ------------------------------------------------------------------------------
//// Comparison
//// ------------------------------------------------------------------------------
TEST(CompareIp, CompareIp_Ipv4_Bigger) {
	unsigned char ipAddress1[FIFTYONE_DEGREES_IPV4_LENGTH] = {9, 8, 7, 6};
	unsigned char ipAddress2[FIFTYONE_DEGREES_IPV4_LENGTH] = {9, 8, 6, 6};

	EXPECT_TRUE(
		fiftyoneDegreesCompareIpAddresses(
			ipAddress1, 
			ipAddress2, 
			FIFTYONE_DEGREES_EVIDENCE_IP_TYPE_IPV4) > 0) << "Result should be positive "
		"where IP address 1 is bigger than IP address 2\n";
}

TEST(CompareIp, CompareIp_Ipv4_Smaller) {
	unsigned char ipAddress1[FIFTYONE_DEGREES_IPV4_LENGTH] = {3, 4, 5, 6};
	unsigned char ipAddress2[FIFTYONE_DEGREES_IPV4_LENGTH] = {3, 4, 6, 6};

	EXPECT_TRUE(
		fiftyoneDegreesCompareIpAddresses(
			ipAddress1, 
			ipAddress2, 
			FIFTYONE_DEGREES_EVIDENCE_IP_TYPE_IPV4) < 0) << "Result should be negative "
		"where IP address 1 is bigger than IP address 2\n";
}

TEST(CompareIp, CompareIp_Ipv4_Equal) {
	unsigned char ipAddress1[FIFTYONE_DEGREES_IPV4_LENGTH] = {3, 4, 5, 6};
	unsigned char ipAddress2[FIFTYONE_DEGREES_IPV4_LENGTH] = {3, 4, 5, 6};

	EXPECT_TRUE(
		fiftyoneDegreesCompareIpAddresses(
			ipAddress1, 
			ipAddress2, 
			FIFTYONE_DEGREES_EVIDENCE_IP_TYPE_IPV4) == 0) << "Result should be 0 "
		"where IP address 1 is bigger than IP address 2\n";
}

TEST(CompareIp, CompareIp_Ipv6_Bigger) {
	unsigned char ipAddress1[FIFTYONE_DEGREES_IPV6_LENGTH] = 
		{16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1};
	unsigned char ipAddress2[FIFTYONE_DEGREES_IPV6_LENGTH] = 
		{16, 15, 14, 13, 12, 11, 9, 9, 8, 7, 6, 5, 4, 2, 2, 1};

	EXPECT_TRUE(
		fiftyoneDegreesCompareIpAddresses(
			ipAddress1, 
			ipAddress2, 
			FIFTYONE_DEGREES_EVIDENCE_IP_TYPE_IPV6) > 0) << "Result should be positive "
		"where IP address 1 is bigger than IP address 2\n";
}

TEST(CompareIp, CompareIp_Ipv6_Smaller) {
	unsigned char ipAddress1[FIFTYONE_DEGREES_IPV6_LENGTH] = 
		{16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1};
	unsigned char ipAddress2[FIFTYONE_DEGREES_IPV6_LENGTH] = 
		{16, 15, 14, 13, 12, 12, 9, 9, 8, 7, 6, 5, 4, 4, 2, 1};

	EXPECT_TRUE(
		fiftyoneDegreesCompareIpAddresses(
			ipAddress1, 
			ipAddress2, 
			FIFTYONE_DEGREES_EVIDENCE_IP_TYPE_IPV6) < 0) << "Result should be negative "
		"where IP address 1 is bigger than IP address 2\n";
}

TEST(CompareIp, CompareIp_Ipv6_Equal) {
	unsigned char ipAddress1[FIFTYONE_DEGREES_IPV6_LENGTH] = 
		{16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1};
	unsigned char ipAddress2[FIFTYONE_DEGREES_IPV6_LENGTH] = 
		{16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1};

	EXPECT_TRUE(
		fiftyoneDegreesCompareIpAddresses(
			ipAddress1, 
			ipAddress2, 
			FIFTYONE_DEGREES_EVIDENCE_IP_TYPE_IPV6) == 0) << "Result should be 0 "
		"where IP address 1 is bigger than IP address 2\n";
}

TEST(CompareIp, CompareIp_Ip_Invalid) {
	unsigned char ipAddress1[FIFTYONE_DEGREES_IPV6_LENGTH] = 
		{16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1};
	unsigned char ipAddress2[FIFTYONE_DEGREES_IPV6_LENGTH] = 
		{16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1};

	EXPECT_TRUE(
		fiftyoneDegreesCompareIpAddresses(
			ipAddress1, 
			ipAddress2, 
			FIFTYONE_DEGREES_EVIDENCE_IP_TYPE_INVALID) == 0) << "Result should be 0 "
		"where type is invalid\n";
}