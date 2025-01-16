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
#include "../ip.h"

static bool CheckResult(byte* result, byte* expected, uint16_t size) {
	bool match = true;
	for (uint16_t i = 0; i < size; i++) {
		match = match && *result == *expected;
		result++;
		expected++;
	}
	return match;
}

static fiftyoneDegreesIpAddressEvidence* parseIpAddressString(const char * const ipString) {
	const char * const end = ipString ? ipString + strlen(ipString) : nullptr;
	return fiftyoneDegreesIpAddressParse(malloc, ipString, end);
}

 // ------------------------------------------------------------------------------
 // IPv4 tests
 // ------------------------------------------------------------------------------
TEST(ParseIp, ParseIp_Ipv4_Low)
{
	const char* ip = "0.0.0.0";
	fiftyoneDegreesIpAddressEvidence* const result = parseIpAddressString(ip);
	byte expected[] = { 0, 0, 0, 0 };
	EXPECT_TRUE(CheckResult(result->address, expected, sizeof(expected))) <<
		L"Expected result to be '0.0.0.0'";
	free(result);
}
TEST(ParseIp, ParseIp_Ipv4_High)
{
	fiftyoneDegreesIpAddressEvidence* const result = parseIpAddressString("255.255.255.255");
	byte expected[] = { 255, 255, 255, 255 };
	EXPECT_TRUE(CheckResult(result->address, expected, sizeof(expected)))
		<< L"Expected result to be '255.255.255.255'";
	free(result);
}
TEST(ParseIp, ParseIp_Ipv4_PortNumber)
{
	fiftyoneDegreesIpAddressEvidence* const result = parseIpAddressString("1.2.3.4:80");
	byte expected[] = { 1, 2, 3, 4 };
	EXPECT_TRUE(CheckResult(result->address, expected, sizeof(expected)))
		<< L"Expected result to be '1.2.3.4'";
	free(result);
}
TEST(ParseIp, ParseIp_Ipv4_CIDRFormat)
{
	fiftyoneDegreesIpAddressEvidence* const result = parseIpAddressString("1.2.3.4/32");
	byte expected[] = { 1, 2, 3, 4 };
	EXPECT_TRUE(CheckResult(result->address, expected, sizeof(expected)))
		<< L"Expected result to be '1.2.3.4'";
	free(result);
}
// ------------------------------------------------------------------------------
// IPv6 tests
// ------------------------------------------------------------------------------
TEST(ParseIp, ParseIp_Ipv6_Low)
{
	fiftyoneDegreesIpAddressEvidence* const result = parseIpAddressString("0:0:0:0:0:0:0:0");
	byte expected[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	EXPECT_TRUE(CheckResult(result->address, expected, sizeof(expected)))
		<< L"Expected result to be '0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0'";
	free(result);
}
TEST(ParseIp, ParseIp_Ipv6_Low_Abbreviated)
{
	fiftyoneDegreesIpAddressEvidence* const result = parseIpAddressString("::");
	byte expected[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	EXPECT_TRUE(CheckResult(result->address, expected, sizeof(expected)))
		<< L"Expected result to be '0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0'";
	free(result);
}
TEST(ParseIp, ParseIp_Ipv6_High)
{
	fiftyoneDegreesIpAddressEvidence* const result = parseIpAddressString("FFFF:FFFF:FFFF:FFFF:FFFF:FFFF:FFFF:FFFF");
	byte expected[] = { 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 };
	EXPECT_TRUE(CheckResult(result->address, expected, sizeof(expected)))
		<< L"Expected result to be '255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255'";
	free(result);
}
TEST(ParseIp, ParseIp_Ipv6_AbbreviatedStart)
{
	const char *ipv6AbbreviatedStart = "::FFFF:FFFF:FFFF:FFFF";
	byte expected[FIFTYONE_DEGREES_IPV6_LENGTH] = 
		{ 0, 0, 0, 0, 0, 0, 0, 0, 255, 255, 255, 255, 255, 255, 255, 255 };

	fiftyoneDegreesIpAddressEvidence* result = 
		fiftyoneDegreesIpAddressParse(
			malloc,
			ipv6AbbreviatedStart,
			ipv6AbbreviatedStart + strlen(ipv6AbbreviatedStart));

	EXPECT_TRUE(result != nullptr) << "Abbreviated start IPv6 address "
		"should be successfully parsed, where the address is " <<
		ipv6AbbreviatedStart << ".";

	EXPECT_TRUE(result->type == FIFTYONE_DEGREES_IP_EVIDENCE_TYPE_IPV6) <<
		"IP address version was not identified correctly where where the " <<
		"IP address is " << ipv6AbbreviatedStart << ".";

	EXPECT_TRUE(
		CheckResult(result->address, expected, FIFTYONE_DEGREES_IPV6_LENGTH)) << 
		"The value of the abbreivated start IPv6 address is not correctly parsed.";
    free(result);
}
TEST(ParseIp, ParseIp_Ipv6_AbbreviatedMiddle)
{
	fiftyoneDegreesIpAddressEvidence* const result = parseIpAddressString("FFFF:FFFF::FFFF:FFFF");
	byte expected[] = { 255, 255, 255, 255, 0, 0, 0, 0, 0, 0, 0, 0, 255, 255, 255, 255 };
	EXPECT_TRUE(CheckResult(result->address, expected, sizeof(expected)))
		<< L"Expected result to be '255, 255, 255, 255, 0, 0, 0, 0, 0, 0, 0, 0, 255, 255, 255, 255'";
}
TEST(ParseIp, ParseIp_Ipv6_AbbreviatedEnd)
{
	fiftyoneDegreesIpAddressEvidence* const result = parseIpAddressString("FFFF:FFFF:FFFF:FFFF::");
	byte expected[] = { 255, 255, 255, 255, 255, 255, 255, 255, 0, 0, 0, 0, 0, 0, 0, 0 };
	EXPECT_TRUE(CheckResult(result->address, expected, sizeof(expected)))
		<< L"Expected result to be '255, 255, 255, 255, 255, 255, 255, 255, 0, 0, 0, 0, 0, 0, 0, 0'";
}
TEST(ParseIp, ParseIp_Ipv6_PortNumber)
{
	fiftyoneDegreesIpAddressEvidence* const result = parseIpAddressString("[2001::1]:80");
	byte expected[] = { 32, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 };
	EXPECT_TRUE(CheckResult(result->address, expected, sizeof(expected)))
		<< L"Expected result to be '32, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1'";
}
TEST(ParseIp, ParseIp_Ipv6_CIDRFormat)
{
	fiftyoneDegreesIpAddressEvidence* const result = parseIpAddressString("2001::1/128");
	byte expected[] = { 32, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 };
	EXPECT_TRUE(CheckResult(result->address, expected, sizeof(expected)))
		<< L"Expected result to be '32, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1'";
}

// ------------------------------------------------------------------------------
// Invalid data
// ------------------------------------------------------------------------------
TEST(ParseIp, ParseIp_Invalid_ipv4OutOfRange)
{
	const char *ipv4OutOfRange = "256.256.256.256";
	byte expected[FIFTYONE_DEGREES_IPV4_LENGTH] = 
		{255, 255, 255, 255};

	fiftyoneDegreesIpAddressEvidence* result = 
		fiftyoneDegreesIpAddressParse(
			malloc,
			ipv4OutOfRange,
			ipv4OutOfRange + strlen(ipv4OutOfRange));

	EXPECT_TRUE(result != nullptr) << "Out of range IPv4 address "
		"should be successfull parsed, where the address is " <<
		ipv4OutOfRange << ".";

	EXPECT_TRUE(result->type == FIFTYONE_DEGREES_IP_EVIDENCE_TYPE_IPV4) <<
		"IP address version was not identified correctly where where the " <<
		"IP address is " << ipv4OutOfRange << ".";

	EXPECT_TRUE(
		CheckResult(result->address, expected, FIFTYONE_DEGREES_IPV4_LENGTH)) <<
		"The value of the out of range IPv4 address is not correctly restricted "
		"at 255.";
    free(result);
}
TEST(ParseIp, ParseIp_Invalid_ipv4TooMany)
{
	fiftyoneDegreesIpAddressEvidence* const result = parseIpAddressString("1.2.3.4.5");
	EXPECT_FALSE(result);
	free(result);
}
TEST(ParseIp, ParseIp_Invalid_ipv4TooFew)
{
	fiftyoneDegreesIpAddressEvidence* const result = parseIpAddressString("1.2.3");
	EXPECT_FALSE(result);
	free(result);
}
TEST(ParseIp, ParseIp_Invalid_letters)
{
	fiftyoneDegreesIpAddressEvidence* const result = parseIpAddressString("a.b.c.d");
	EXPECT_FALSE(result);
	free(result);
}
TEST(ParseIp, ParseIp_Invalid_emptyString)
{
	fiftyoneDegreesIpAddressEvidence* const result = parseIpAddressString("");
	EXPECT_FALSE(result);
	free(result);
}
TEST(ParseIp, ParseIp_Invalid_null)
{
	fiftyoneDegreesIpAddressEvidence* const result = parseIpAddressString(nullptr);
	EXPECT_FALSE(result);
	free(result);
}
TEST(ParseIp, ParseIp_Invalid_Number)
{
	fiftyoneDegreesIpAddressEvidence* const result = parseIpAddressString("1234");
	EXPECT_FALSE(result);
	free(result);
}
TEST(ParseIp, ParseIp_Invalid_ipv6OutOfRange)
{
	fiftyoneDegreesIpAddressEvidence* const result = parseIpAddressString("10000::1");
	EXPECT_FALSE(result);
	free(result);
}
TEST(ParseIp, ParseIp_Invalid_ipv6InvalidLetter)
{
	fiftyoneDegreesIpAddressEvidence* const result = parseIpAddressString("GFFF::1");
	EXPECT_FALSE(result);
	free(result);
}
TEST(ParseIp, ParseIp_Invalid_ipv6TooMany)
{
	fiftyoneDegreesIpAddressEvidence* const result = parseIpAddressString("1:1:1:1:1:1:1:1:1");
	EXPECT_FALSE(result);
	free(result);
}
TEST(ParseIp, ParseIp_Invalid_ipv6TooFew)
{
	fiftyoneDegreesIpAddressEvidence* const result = parseIpAddressString("1:1:1:1:1:1:1");
	EXPECT_FALSE(result);
	free(result);
}
// ------------------------------------------------------------------------------
// Comparison
// ------------------------------------------------------------------------------
TEST(CompareIp, CompareIp_Ipv4_Bigger) {
	unsigned char ipAddress1[FIFTYONE_DEGREES_IPV4_LENGTH] = {9, 8, 7, 6};
	unsigned char ipAddress2[FIFTYONE_DEGREES_IPV4_LENGTH] = {9, 8, 6, 6};

	EXPECT_TRUE(
		fiftyoneDegreesIpAddressesCompare(
			ipAddress1, 
			ipAddress2, 
			FIFTYONE_DEGREES_IP_EVIDENCE_TYPE_IPV4) > 0) << "Result should be positive "
		"where IP address 1 is bigger than IP address 2\n";
}

TEST(CompareIp, CompareIp_Ipv4_Smaller) {
	unsigned char ipAddress1[FIFTYONE_DEGREES_IPV4_LENGTH] = {3, 4, 5, 6};
	unsigned char ipAddress2[FIFTYONE_DEGREES_IPV4_LENGTH] = {3, 4, 6, 6};

	EXPECT_TRUE(
		fiftyoneDegreesIpAddressesCompare(
			ipAddress1, 
			ipAddress2, 
			FIFTYONE_DEGREES_IP_EVIDENCE_TYPE_IPV4) < 0) << "Result should be negative "
		"where IP address 1 is bigger than IP address 2\n";
}

TEST(CompareIp, CompareIp_Ipv4_Equal) {
	unsigned char ipAddress1[FIFTYONE_DEGREES_IPV4_LENGTH] = {3, 4, 5, 6};
	unsigned char ipAddress2[FIFTYONE_DEGREES_IPV4_LENGTH] = {3, 4, 5, 6};

	EXPECT_TRUE(
		fiftyoneDegreesIpAddressesCompare(
			ipAddress1, 
			ipAddress2, 
			FIFTYONE_DEGREES_IP_EVIDENCE_TYPE_IPV4) == 0) << "Result should be 0 "
		"where IP address 1 is bigger than IP address 2\n";
}

TEST(CompareIp, CompareIp_Ipv6_Bigger) {
	unsigned char ipAddress1[FIFTYONE_DEGREES_IPV6_LENGTH] = 
		{16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1};
	unsigned char ipAddress2[FIFTYONE_DEGREES_IPV6_LENGTH] = 
		{16, 15, 14, 13, 12, 11, 9, 9, 8, 7, 6, 5, 4, 2, 2, 1};

	EXPECT_TRUE(
		fiftyoneDegreesIpAddressesCompare(
			ipAddress1, 
			ipAddress2, 
			FIFTYONE_DEGREES_IP_EVIDENCE_TYPE_IPV6) > 0) << "Result should be positive "
		"where IP address 1 is bigger than IP address 2\n";
}

TEST(CompareIp, CompareIp_Ipv6_Smaller) {
	unsigned char ipAddress1[FIFTYONE_DEGREES_IPV6_LENGTH] = 
		{16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1};
	unsigned char ipAddress2[FIFTYONE_DEGREES_IPV6_LENGTH] = 
		{16, 15, 14, 13, 12, 12, 9, 9, 8, 7, 6, 5, 4, 4, 2, 1};

	EXPECT_TRUE(
		fiftyoneDegreesIpAddressesCompare(
			ipAddress1, 
			ipAddress2, 
			FIFTYONE_DEGREES_IP_EVIDENCE_TYPE_IPV6) < 0) << "Result should be negative "
		"where IP address 1 is bigger than IP address 2\n";
}

TEST(CompareIp, CompareIp_Ipv6_Equal) {
	unsigned char ipAddress1[FIFTYONE_DEGREES_IPV6_LENGTH] = 
		{16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1};
	unsigned char ipAddress2[FIFTYONE_DEGREES_IPV6_LENGTH] = 
		{16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1};

	EXPECT_TRUE(
		fiftyoneDegreesIpAddressesCompare(
			ipAddress1, 
			ipAddress2, 
			FIFTYONE_DEGREES_IP_EVIDENCE_TYPE_IPV6) == 0) << "Result should be 0 "
		"where IP address 1 is bigger than IP address 2\n";
}

TEST(CompareIp, CompareIp_Ip_Invalid) {
	unsigned char ipAddress1[FIFTYONE_DEGREES_IPV6_LENGTH] = 
		{16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1};
	unsigned char ipAddress2[FIFTYONE_DEGREES_IPV6_LENGTH] = 
		{16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1};

	EXPECT_TRUE(
		fiftyoneDegreesIpAddressesCompare(
			ipAddress1, 
			ipAddress2, 
			FIFTYONE_DEGREES_IP_EVIDENCE_TYPE_INVALID) == 0) << "Result should be 0 "
		"where type is invalid\n";
}