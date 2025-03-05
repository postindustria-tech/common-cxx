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
#include <cmath>
#include "../double.h"

using namespace std;

static fiftyoneDegreesDoubleInternal toInternalDouble(const uint64_t n) {
	return *reinterpret_cast<const fiftyoneDegreesDoubleInternal *>(&n);
}

/*
 * fiftyoneDegreesDouble to Native conversion tests
 */
TEST(Double, Double_DoubleToNative_Postive)
{
	auto const dValue = toInternalDouble(4638364427826256413ULL);

	double expected = 123.123;
	EXPECT_DOUBLE_EQ(expected, fiftyoneDegreesDoubleToNative(dValue));
#if (DBL_RADIX == 2 && DBL_MANT_DIG == 53 && DBL_MAX_EXP == 1024 && DBL_MIN_EXP == -1021)
	/*
	 * On machine where the standard is supported. 51Degrees implementation should
	 * have the exact match as native double.
	 */
	EXPECT_EQ(0, memcmp(&expected, dValue.value, FIFTYONE_DEGREES_DOUBLE_SIZE));
#endif
}

TEST(Double, Double_DoubleToNative_Postive_Low)
{
	auto const dValue = toInternalDouble(4593527504729830064ULL);

	double expected = 0.123;
	EXPECT_DOUBLE_EQ(expected, fiftyoneDegreesDoubleToNative(dValue));
#if (DBL_RADIX == 2 && DBL_MANT_DIG == 53 && DBL_MAX_EXP == 1024 && DBL_MIN_EXP == -1021)
	/*
	 * On machine where the standard is supported. 51Degrees implementation should
	 * have the exact match as native double.
	 */
	EXPECT_EQ(0, memcmp(&expected, dValue.value, FIFTYONE_DEGREES_DOUBLE_SIZE));
#endif
}

TEST(Double, Double_DoubleToNative_Negative)
{
	auto const dValue = toInternalDouble(13861736464681032221ULL);

	double expected = -123.123;
	EXPECT_DOUBLE_EQ(expected, fiftyoneDegreesDoubleToNative(dValue));
#if (DBL_RADIX == 2 && DBL_MANT_DIG == 53 && DBL_MAX_EXP == 1024 && DBL_MIN_EXP == -1021)
	/*
	 * On machine where the standard is supported. 51Degrees implementation should
	 * have the exact match as native double.
	 */
	EXPECT_EQ(0, memcmp(&expected, dValue.value, FIFTYONE_DEGREES_DOUBLE_SIZE));
#endif
}

TEST(Double, Double_DoubleToNative_Negative_High)
{
	auto const dValue = toInternalDouble(13816899541584605872ULL);

	double expected = -0.123;
	EXPECT_DOUBLE_EQ(expected, fiftyoneDegreesDoubleToNative(dValue));
#if (DBL_RADIX == 2 && DBL_MANT_DIG == 53 && DBL_MAX_EXP == 1024 && DBL_MIN_EXP == -1021)
	/*
	 * On machine where the standard is supported. 51Degrees implementation should
	 * have the exact match as native double.
	 */
	EXPECT_EQ(0, memcmp(&expected, dValue.value, FIFTYONE_DEGREES_DOUBLE_SIZE));
#endif
}

TEST(Double, Double_DoubleToNative_Zero)
{
	auto const dValue = toInternalDouble(0ULL);

	double expected = 0;
	EXPECT_DOUBLE_EQ(expected, fiftyoneDegreesDoubleToNative(dValue));
#if (DBL_RADIX == 2 && DBL_MANT_DIG == 53 && DBL_MAX_EXP == 1024 && DBL_MIN_EXP == -1021)
	/*
	 * On machine where the standard is supported. 51Degrees implementation should
	 * have the exact match as native double.
	 */
	EXPECT_EQ(0, memcmp(&expected, dValue.value, FIFTYONE_DEGREES_DOUBLE_SIZE));
#endif
}

TEST(Double, Double_DoubleToNative_Max)
{
	auto const dValue = toInternalDouble(9218868437227393708ULL);

	const double expected = FIFTYONE_DEGREES_DOUBLE_MAX;
	EXPECT_DOUBLE_EQ(expected, fiftyoneDegreesDoubleToNative(dValue));
#if (DBL_RADIX == 2 && DBL_MANT_DIG == 53 && DBL_MAX_EXP == 1024 && DBL_MIN_EXP == -1021)
	/*
	 * On machine where the standard is supported. 51Degrees implementation should
	 * have the exact match as native double.
	 */
	EXPECT_EQ(0, memcmp(&expected, dValue.value, FIFTYONE_DEGREES_DOUBLE_SIZE));
#endif
}

TEST(Double, Double_DoubleToNative_Min)
{
	auto const dValue = toInternalDouble(4503599628367932ULL);

	double expected = FIFTYONE_DEGREES_DOUBLE_MIN;
	EXPECT_DOUBLE_EQ(expected, fiftyoneDegreesDoubleToNative(dValue));
#if (DBL_RADIX == 2 && DBL_MANT_DIG == 53 && DBL_MAX_EXP == 1024 && DBL_MIN_EXP == -1021)
	/*
	 * On machine where the standard is supported. 51Degrees implementation should
	 * have the exact match as native double.
	 */
	EXPECT_EQ(0, memcmp(&expected, dValue.value, FIFTYONE_DEGREES_DOUBLE_SIZE));
#endif
}

TEST(Double, Double_DoubleToNative_Min_Neg)
{
	auto const dValue = toInternalDouble(18442240474082169516ULL);

	double expected = FIFTYONE_DEGREES_DOUBLE_MIN_NEG;
	EXPECT_DOUBLE_EQ(expected, fiftyoneDegreesDoubleToNative(dValue));
#if (DBL_RADIX == 2 && DBL_MANT_DIG == 53 && DBL_MAX_EXP == 1024 && DBL_MIN_EXP == -1021)
	/*
	 * On machine where the standard is supported. 51Degrees implementation should
	 * have the exact match as native double.
	 */
	EXPECT_EQ(0, memcmp(&expected, dValue.value, FIFTYONE_DEGREES_DOUBLE_SIZE));
#endif
}

TEST(Double, Double_DoubleToNative_Nan)
{
	auto const dValue = toInternalDouble(9221120237041090560ULL);

	EXPECT_EQ(FP_NAN, fpclassify(fiftyoneDegreesDoubleToNative(dValue)));
}

TEST(Double, Double_DoubleToNative_Inf)
{
	auto const dValue = toInternalDouble(INT64_MAX);

	EXPECT_EQ(FP_INFINITE, fpclassify(fiftyoneDegreesDoubleToNative(dValue)));
}

TEST(Double, Double_DoubleToNative_Subnormal)
{
	auto const dValue = toInternalDouble(1ULL);

	EXPECT_EQ(FP_SUBNORMAL, fpclassify(fiftyoneDegreesDoubleToNative(dValue)));
}

/*
 * Native to fiftyoneDegreesDouble conversion tests
 */
TEST(Double, Double_NativeToDouble_Postive)
{
	double testDouble = 123.123;

	auto const expected = toInternalDouble(4638364427826256413ULL);
	
	fiftyoneDegreesDoubleInternal actual = fiftyoneDegreesNativeToDouble(testDouble);
	ASSERT_EQ(0, fiftyoneDegreesDoubleIsEqual(expected, actual));
#if (DBL_RADIX == 2 && DBL_MANT_DIG == 53 && DBL_MAX_EXP == 1024 && DBL_MIN_EXP == -1021)
	/*
	 * On machine where the standard is supported. 51Degrees implementation should
	 * have the exact match as native double.
	 */
	ASSERT_EQ(0, memcmp(&testDouble, actual.value, FIFTYONE_DEGREES_DOUBLE_SIZE));
#endif
}

TEST(Double, Double_NativeToDouble_Postive_Low)
{
	double testDouble = 0.123;

	auto const expected = toInternalDouble(4593527504729830064ULL);

	fiftyoneDegreesDoubleInternal actual = fiftyoneDegreesNativeToDouble(testDouble);
	ASSERT_EQ(0, fiftyoneDegreesDoubleIsEqual(expected, actual));
#if (DBL_RADIX == 2 && DBL_MANT_DIG == 53 && DBL_MAX_EXP == 1024 && DBL_MIN_EXP == -1021)
	/*
	 * On machine where the standard is supported. 51Degrees implementation should
	 * have the exact match as native double.
	 */
	ASSERT_EQ(0, memcmp(&testDouble, actual.value, FIFTYONE_DEGREES_DOUBLE_SIZE));
#endif
}

TEST(Double, Double_NativeToDouble_Negative)
{
	double testDouble = -123.123;

	auto const expected = toInternalDouble(13861736464681032221ULL);

	fiftyoneDegreesDoubleInternal actual = fiftyoneDegreesNativeToDouble(testDouble);
	ASSERT_EQ(0, fiftyoneDegreesDoubleIsEqual(expected, actual));
#if (DBL_RADIX == 2 && DBL_MANT_DIG == 53 && DBL_MAX_EXP == 1024 && DBL_MIN_EXP == -1021)
	/*
	 * On machine where the standard is supported. 51Degrees implementation should
	 * have the exact match as native double.
	 */
	ASSERT_EQ(0, memcmp(&testDouble, actual.value, FIFTYONE_DEGREES_DOUBLE_SIZE));
#endif
}

TEST(Double, Double_NativeToDouble_Negative_High)
{
	double testDouble = -0.123;

	auto const expected = toInternalDouble(13816899541584605872ULL);

	fiftyoneDegreesDoubleInternal actual = fiftyoneDegreesNativeToDouble(testDouble);
	ASSERT_EQ(0, fiftyoneDegreesDoubleIsEqual(expected, actual));
#if (DBL_RADIX == 2 && DBL_MANT_DIG == 53 && DBL_MAX_EXP == 1024 && DBL_MIN_EXP == -1021)
	/*
	 * On machine where the standard is supported. 51Degrees implementation should
	 * have the exact match as native double.
	 */
	ASSERT_EQ(0, memcmp(&testDouble, actual.value, FIFTYONE_DEGREES_DOUBLE_SIZE));
#endif
}

TEST(Double, Double_NativeToDouble_Zero)
{
	double testDouble = 0;

	auto const expected = toInternalDouble(0ULL);

	fiftyoneDegreesDoubleInternal actual = fiftyoneDegreesNativeToDouble(testDouble);
	ASSERT_EQ(0, fiftyoneDegreesDoubleIsEqual(expected, actual));
#if (DBL_RADIX == 2 && DBL_MANT_DIG == 53 && DBL_MAX_EXP == 1024 && DBL_MIN_EXP == -1021)
	/*
	 * On machine where the standard is supported. 51Degrees implementation should
	 * have the exact match as native double.
	 */
	ASSERT_EQ(0, memcmp(&testDouble, actual.value, FIFTYONE_DEGREES_DOUBLE_SIZE));
#endif
}

TEST(Double, Double_NativeToDouble_Max)
{
	double testDouble = FIFTYONE_DEGREES_DOUBLE_MAX;

	auto const expected = toInternalDouble(9218868437227393708ULL);

	fiftyoneDegreesDoubleInternal actual = fiftyoneDegreesNativeToDouble(testDouble);
	ASSERT_EQ(0, fiftyoneDegreesDoubleIsEqual(expected, actual));
#if (DBL_RADIX == 2 && DBL_MANT_DIG == 53 && DBL_MAX_EXP == 1024 && DBL_MIN_EXP == -1021)
	/*
	 * On machine where the standard is supported. 51Degrees implementation should
	 * have the exact match as native double.
	 */
	ASSERT_EQ(0, memcmp(&testDouble, actual.value, FIFTYONE_DEGREES_DOUBLE_SIZE));
#endif
}

TEST(Double, Double_NativeToDouble_Min)
{
	double testDouble = FIFTYONE_DEGREES_DOUBLE_MIN;

	auto const expected = toInternalDouble(4503599628367932ULL);

	fiftyoneDegreesDoubleInternal actual = fiftyoneDegreesNativeToDouble(testDouble);
	ASSERT_EQ(0, fiftyoneDegreesDoubleIsEqual(expected, actual));
#if (DBL_RADIX == 2 && DBL_MANT_DIG == 53 && DBL_MAX_EXP == 1024 && DBL_MIN_EXP == -1021)
	/*
	 * On machine where the standard is supported. 51Degrees implementation should
	 * have the exact match as native double.
	 */
	ASSERT_EQ(0, memcmp(&testDouble, actual.value, FIFTYONE_DEGREES_DOUBLE_SIZE));
#endif
}

TEST(Double, Double_NativeToDouble_Min_Neg)
{
	double testDouble = FIFTYONE_DEGREES_DOUBLE_MIN_NEG;

	auto const expected = toInternalDouble(18442240474082169516ULL);

	fiftyoneDegreesDoubleInternal actual = fiftyoneDegreesNativeToDouble(testDouble);
	ASSERT_EQ(0, fiftyoneDegreesDoubleIsEqual(expected, actual));
#if (DBL_RADIX == 2 && DBL_MANT_DIG == 53 && DBL_MAX_EXP == 1024 && DBL_MIN_EXP == -1021)
	/*
	 * On machine where the standard is supported. 51Degrees implementation should
	 * have the exact match as native double.
	 */
	ASSERT_EQ(0, memcmp(&testDouble, actual.value, FIFTYONE_DEGREES_DOUBLE_SIZE));
#endif
}

TEST(Double, Double_NativeToDouble_Nan)
{
	double testDouble = NAN;

	auto const expected = toInternalDouble( 9218868437227405313ULL);
	ASSERT_EQ(0, fiftyoneDegreesDoubleIsEqual(expected, fiftyoneDegreesNativeToDouble(testDouble)));
}

TEST(Double, Double_NativeToDouble_Inf)
{
	double testDouble = INFINITY;

	auto const expected = toInternalDouble(INT64_MAX);
	ASSERT_EQ(0, fiftyoneDegreesDoubleIsEqual(expected, fiftyoneDegreesNativeToDouble(testDouble)));
}

TEST(Double, Double_PartsCodec_Postive)
{
	auto const expected = toInternalDouble(4638364427826256413ULL);
	auto const parts = fiftyoneDegreesDoubleDecompose(expected);
	auto const actual = fiftyoneDegreesDoubleRecompose(parts);
	ASSERT_EQ(0, fiftyoneDegreesDoubleIsEqual(expected, actual));
}

TEST(Double, Double_PartsCodec_NaN)
{
	auto const expected = toInternalDouble(9221120237041090560ULL);
	auto const parts = fiftyoneDegreesDoubleDecompose(expected);
	auto const actual = fiftyoneDegreesDoubleRecompose(parts);
	ASSERT_EQ(0, fiftyoneDegreesDoubleIsEqual(expected, actual));
}


