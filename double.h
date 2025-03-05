/* *********************************************************************
 * This Original Work is copyright of 51 Degrees Mobile Experts Limited.
 * Copyright 2025 51 Degrees Mobile Experts Limited, Davidson House,
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

#ifndef FIFTYONE_DEGREES_DOUBLE_H_INCLUDED
#define FIFTYONE_DEGREES_DOUBLE_H_INCLUDED

/**
 * @ingroup FiftyOneDegreesCommon
 * @defgroup fiftyoneDegreesDouble Double
 *
 * IEEE Single Precision Doubleing Point standard implementation 
 * and methods to convert to native double type.
 *
 * ## Introduction
 * IEEE Single Precision Doubleing Point standard is supported in
 * majority of modern computers. However, it is not always guaranteed,
 * so a indepdent implementation is required so that on the machines where
 * this standard is not supported, the double type from the data file can
 * still be read correctly.
 *
 * ## Endianness
 * Endianess difference between machines will not be handled at the
 * moment, until it is supported by 51Degrees data file.
 *
 * ## Limitations
 * Positive sign will always be defaulted to during a conversion from native
 * double type when input double is NaN (Not a Number) or Inf (Infinity).
 * 
 * When converting from 51Degrees implementation to native double type, if it results
 * in a NaN or Inf, the value from compiler macros that represent
 * these number will be returned. i.e. NAN and INFINITY
 *
 * @{
 */

#include <stdint.h>
#include "data.h"
#include "common.h"

/**
 * IEEE single precision doubleing point bias value
 */
#define FIFTYONE_DEGREES_DOUBLE_BIAS 1023
/**
 * IEEE single precision doubleing point number of bytes
 */
#define FIFTYONE_DEGREES_DOUBLE_SIZE 8
/**
 * IEEE single precision doubleing point base value
 */
#define FIFTYONE_DEGREES_DOUBLE_RADIX 2
/**
 * IEEE single precision doubleing point number of bits for sgn
 */
#define FIFTYONE_DEGREES_DOUBLE_SIGN_SIZE 1
/**
 * IEEE single precision doubleing point number of bits for exponent
 */
#define FIFTYONE_DEGREES_DOUBLE_EXP_SIZE 11
/**
 * IEEE single precision doubleing point number of bits for mantissa
 */
#define FIFTYONE_DEGREES_DOUBLE_MANT_SIZE 52
/**
 * IEEE single precision doubleing point max positive value
 */
#define FIFTYONE_DEGREES_DOUBLE_MAX 1.79769313486E+308
/**
 * IEEE single precision doubleing point min positive value
 */
#define FIFTYONE_DEGREES_DOUBLE_MIN 2.225073859E-308
/**
 * IEEE single precision doubleing point min negative value
 */
#define FIFTYONE_DEGREES_DOUBLE_MIN_NEG (-1.79769313486E+308)
/**
 * IEEE single precision doubleing point max exponent value where all bits
 * are 1. This can only happen in NaN (Not a Number) and Inf (Infinity) cases.
 */
#define FIFTYONE_DEGREES_DOUBLE_EXP_MAX 2047
/**
 * IEEE single precision doubleing point max mantissa value where all bits
 * are 1.
 */
#define FIFTYONE_DEGREES_DOUBLE_MANT_MAX 4503599627370495LL

/**
 * Structure that represents 51Degrees implementation, which encapsulate
 * an array of 8 bytes. This array will be formated accordingly to
 * the IEEE standard
 */
typedef struct fiftyone_degrees_double_type {
	byte value[FIFTYONE_DEGREES_DOUBLE_SIZE];
} fiftyoneDegreesDoubleInternal;

/**
 * Structure that represents IEEE double decomposition.
 * Intermediate representation.
 */
typedef struct fiftyone_degrees_double_parts_type {
    uint64_t mantissa;
    uint32_t exponent;
    uint32_t sign;
} DoubleParts;

/**
 * Extracts bit fragments from internal value.
 * @param d IEEE value to decompose.
 * @return parts of double.
 */
EXTERNAL DoubleParts fiftyoneDegreesDoubleDecompose(fiftyoneDegreesDoubleInternal const d);

/**
 * Merges bit fragments into internal value.
 * @param parts parts of double.
 * @return Restored IEEE value.
 */
EXTERNAL fiftyoneDegreesDoubleInternal fiftyoneDegreesDoubleRecompose(DoubleParts const parts);

/**
 * Function that converts from a 51Degrees double implementation to a
 * native double value.
 * @param d input 51Degrees double value
 * @return a native double value
 */
EXTERNAL double fiftyoneDegreesDoubleToNative(fiftyoneDegreesDoubleInternal d);
/**
 * Function that converts from a native double value to 51Degrees double
 * value.
 * @param f input native double value
 * @return a 51Degrees double value
 */
EXTERNAL fiftyoneDegreesDoubleInternal fiftyoneDegreesNativeToDouble(double f);
/**
 * Function that compare if two 51Degrees double values are equal
 * @param f1 first input 51Degrees double value
 * @param f2 second input 51Degrees double value
 * @return 0 if the two are equal and 1 if they are not.
 */
EXTERNAL int fiftyoneDegreesDoubleIsEqual(fiftyoneDegreesDoubleInternal f1, fiftyoneDegreesDoubleInternal f2);

/**
 * For some compilers such as clang and Microsoft C or computers where
 * the IEEE single precision standard is supported, default the type
 * to the native double type.
 */
#if _MSC_VER || (FLT_RADIX == 2 && FLT_MANT_DIG == 24 && FLT_MAX_EXP == 128 && FLT_MIN_EXP == -125)
/**
 * Define 51Degrees double implementation as native double.
 */
typedef double fiftyoneDegreesDouble;

/**
 * Convert from 51Degrees double to native double
 * @param f 51Degrees double
 * @return native double value. In this case, it is a straight map
 * to the input value itself.
 */
#define FIFTYONE_DEGREES_DOUBLE_TO_NATIVE(f) f
/**
 * Convert from native double to 51Degrees double
 * @param f native double type
 * @return a 51Degrees double value. In this case, it is a straight
 * map to the input value itself.
 */
#define FIFTYONE_DEGREES_NATIVE_TO_DOUBLE(f) f
/**
 * Compare two 51Degrees double input values.
 * @param f1 a 51Degrees double input value.
 * @param f2 a 51Degrees double input value.
 * @return 0 if the two values are the same and 1 otherwise.
 */
#define FIFTYONE_DEGREES_DOUBLE_IS_EQUAL(f1, f2) (f1 == f2 ? 0 : 1)
#else
/**
 * Define 51Degrees double implementation as the internal type
 * IEEE standard is not supported in this case
 */
typedef fiftyoneDegreesDoubleInternal fiftyoneDegreesDouble;
/**
 * Function that converts from a 51Degrees double implementation to a
 * native double value.
 * @param f input 51Degrees double value
 * @return a native double value
 */
#define FIFTYONE_DEGREES_DOUBLE_TO_NATIVE(f) fiftyoneDegreesDoubleToNative(f)
/**
 * Function that converts from a native double value to 51Degrees double
 * value.
 * @param f input native double value
 * @return a 51Degrees double value
 */
#define FIFTYONE_DEGREES_NATIVE_TO_DOUBLE(f) fiftyoneDegreesNativeToDouble(f)
/**
 * Function that compare if two 51Degrees double values are equal
 * @param f1 first input 51Degrees double value
 * @param f2 second input 51Degrees double value
 * @return 0 if the two are equal and 1 if they are not.
 */
#define FIFTYONE_DEGREES_DOUBLE_IS_EQUAL(f1, f2) fiftyoneDegreesDoubleIsEqual(f1, f2)
#endif

/**
 * @}
 */

#endif
