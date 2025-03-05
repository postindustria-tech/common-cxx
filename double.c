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

#include <math.h>
#include "fiftyone.h"

#define IEEE_DOUBLE_MANTISSA_OFFSET (0)
#define IEEE_DOUBLE_EXPONENT_OFFSET (FIFTYONE_DEGREES_DOUBLE_MANT_SIZE)
#define IEEE_DOUBLE_SIGN_OFFSET (FIFTYONE_DEGREES_DOUBLE_MANT_SIZE + FIFTYONE_DEGREES_DOUBLE_EXP_SIZE)

#define IEEE_DOUBLE_MANTISSA_MASK (((1ULL << FIFTYONE_DEGREES_DOUBLE_MANT_SIZE) - 1) << IEEE_DOUBLE_MANTISSA_OFFSET)
#define IEEE_DOUBLE_EXPONENT_MASK (((1ULL << FIFTYONE_DEGREES_DOUBLE_EXP_SIZE) - 1) << IEEE_DOUBLE_EXPONENT_OFFSET)
#define IEEE_DOUBLE_SIGN_MASK (((1ULL << FIFTYONE_DEGREES_DOUBLE_SIGN_SIZE) - 1) << IEEE_DOUBLE_SIGN_OFFSET)

DoubleParts fiftyoneDegreesDoubleDecompose(fiftyoneDegreesDoubleInternal const d) {
	const uint64_t * const rawBytes = (const uint64_t *)&d;
	return (DoubleParts){
		((*rawBytes) & IEEE_DOUBLE_MANTISSA_MASK) >> IEEE_DOUBLE_MANTISSA_OFFSET,
		((*rawBytes) & IEEE_DOUBLE_EXPONENT_MASK) >> IEEE_DOUBLE_EXPONENT_OFFSET,
		((*rawBytes) & IEEE_DOUBLE_SIGN_MASK) >> IEEE_DOUBLE_SIGN_OFFSET,
	};
}

fiftyoneDegreesDoubleInternal fiftyoneDegreesDoubleRecompose(DoubleParts const parts) {
	const uint64_t rawBytes = (0ULL
		| ((parts.mantissa << IEEE_DOUBLE_MANTISSA_OFFSET) & IEEE_DOUBLE_MANTISSA_MASK)
		| ((((uint64_t)parts.exponent) << IEEE_DOUBLE_EXPONENT_OFFSET) & IEEE_DOUBLE_EXPONENT_MASK)
		| ((((uint64_t)parts.sign) << IEEE_DOUBLE_SIGN_OFFSET) & IEEE_DOUBLE_SIGN_MASK)
		);
	return *(fiftyoneDegreesDoubleInternal *)&rawBytes;
}

double fiftyoneDegreesDoubleToNative(fiftyoneDegreesDoubleInternal const d) {
	DoubleParts const parts = fiftyoneDegreesDoubleDecompose(d);
	double nativeValue;

	if (parts.exponent == 0) {
		if (parts.mantissa == 0) {
			// When all bits in exponent and mantissa are 0s. The double value is 0.
			nativeValue = 0;
		}
		else {
			/*
			 * When all bits in exponent are 0s but not in the mantissa. This is a
			 * denormalised double (or subnormal case).
			 * The exponent will be treated as '00000001' instead and the mantissa
			 * won't use the hidden bit.
			 * i.e. double = (-1)^sign * 2^(1 - bias) * (0 + mantissa)
			 */
			nativeValue = (double)pow(-1, (double)parts.sign);
			nativeValue *= pow(FIFTYONE_DEGREES_DOUBLE_RADIX, 1 - FIFTYONE_DEGREES_DOUBLE_BIAS);
			/*
			 * The actual mantissa value is calculated by take its unsigned integer
			 * and divided by 2 to the power of number of bits in the mantissa.
			 * This is how the fractional point is shifted in binary number.
			 */
			nativeValue *= ((double)parts.mantissa) / pow(FIFTYONE_DEGREES_DOUBLE_RADIX, FIFTYONE_DEGREES_DOUBLE_MANT_SIZE);
		}
	}
	else if (parts.exponent == FIFTYONE_DEGREES_DOUBLE_EXP_MAX) {
		if (parts.mantissa == FIFTYONE_DEGREES_DOUBLE_MANT_MAX) {
			/*
			 * If all bits in exponent and mantissa are 1s, it is an infinity value
			 */
			nativeValue = INFINITY;
		}
		else {
			/*
			 * If all bits in exponet are 1s but not the mantissa, it is not a number
			 */
			nativeValue = NAN;
		}
	}
	else {
		/*
		 * Normal case. Double value is caluclated by
		 * double = (-1)^sign * 2^(exponent - bias) * (1 + mantissa)
		 */
		nativeValue = (double)pow(-1, (double)parts.sign);
		nativeValue *= (parts.exponent == 0 ? 0 : pow(FIFTYONE_DEGREES_DOUBLE_RADIX, (double)(parts.exponent) - FIFTYONE_DEGREES_DOUBLE_BIAS));
		nativeValue *= (1 + ((double)parts.mantissa) / pow(FIFTYONE_DEGREES_DOUBLE_RADIX, FIFTYONE_DEGREES_DOUBLE_MANT_SIZE));
	}
	return nativeValue;
}

fiftyoneDegreesDoubleInternal fiftyoneDegreesNativeToDouble(double f) {
	DoubleParts parts = { 0, 0, 0 };
	double significand;
	int exponent;

	switch (fpclassify(f)) {
	case FP_NAN:
		// If NaN, set exponent to all 1s and mantissa to 1
		parts.exponent = FIFTYONE_DEGREES_DOUBLE_EXP_MAX;
		parts.mantissa = 1;
		break;
	case FP_INFINITE:
		// If Inf, set exponent and mantissa to all 1s
		parts.exponent = FIFTYONE_DEGREES_DOUBLE_EXP_MAX;
		parts.mantissa = FIFTYONE_DEGREES_DOUBLE_MANT_MAX;
		break;
	case FP_SUBNORMAL:
		// If subnormal, leave exponent to 0
		significand = (double)frexp(f, &exponent);
		parts.sign = (f >= 0) ? 0 : -1;
		/*
		 * Significand is fractional so time with 2 to power of number of mantissa bits
		 * to get its integer
		 */
		parts.mantissa = (uint64_t)(fabs(significand) * pow(FIFTYONE_DEGREES_DOUBLE_RADIX, FIFTYONE_DEGREES_DOUBLE_MANT_SIZE));
		break;
	case FP_NORMAL:
		significand = (double)frexp(f, &exponent);
		parts.sign = (f >= 0) ? 0 : -1;
		/*
		 * frexp returns value between (-1, 0.5],[0.5, 1) * exponent of 2
		 * to convert to the double calculation formular
		 * (-1)^sign + 2^(exponent - bias) * (1 + mantissa)
		 * the significand need to be > 1. Thus move 1 bit from the exponent
		 * to the mantissa. This 1 bit represents the hidden bit.
		 */
		parts.exponent = exponent - 1 + FIFTYONE_DEGREES_DOUBLE_BIAS;
		parts.mantissa = (uint64_t)((fabs(significand) * FIFTYONE_DEGREES_DOUBLE_RADIX - 1)
			* pow(FIFTYONE_DEGREES_DOUBLE_RADIX, FIFTYONE_DEGREES_DOUBLE_MANT_SIZE));
		break;
	case FP_ZERO:
	default:
		break;
	}
	return fiftyoneDegreesDoubleRecompose(parts);
}

int fiftyoneDegreesDoubleIsEqual(fiftyoneDegreesDoubleInternal f1, fiftyoneDegreesDoubleInternal f2) {
	int isEqual = 0;
	for (int i = 0; i < FIFTYONE_DEGREES_DOUBLE_SIZE; i++) {
		isEqual |= (f1.value[i] == f2.value[i] ? 0 : 1);
	}
	return isEqual;
}