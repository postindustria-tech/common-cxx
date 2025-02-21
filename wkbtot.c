/* *********************************************************************
 * This Original Work is copyright of 51 Degrees Mobile Experts Limited.
 * Copyright 2025 51 Degrees Mobile Experts Limited, Davidson House,
 * Forbury Square, Reading, Berkshire, United Kingdom RG1 3EU.
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

#include "wkbtot.h"
#include <math.h>
#include "fiftyone.h"

typedef struct {
    short dimensionsCount;
    const char *tag;
    size_t tagLength;
} CoordMode;


static const CoordMode CoordModes[] = {
    { 2, NULL, 0 },
    { 3, "Z", 1 },
    { 3, "M", 1 },
    { 4, "ZM", 2 },
};


typedef enum {
    FIFTYONE_DEGREES_WKBToT_ByteOrder_XDR = 0, // Big Endian
    FIFTYONE_DEGREES_WKBToT_ByteOrder_NDR = 1, // Little Endian
} ByteOrder;

#define ByteOrder_XDR FIFTYONE_DEGREES_WKBToT_ByteOrder_XDR
#define ByteOrder_NDR FIFTYONE_DEGREES_WKBToT_ByteOrder_NDR

typedef uint32_t (*IntReader)(const byte *wkbBytes);
typedef double (*DoubleReader)(const byte *wkbBytes);
typedef struct  {
    const char *name;
    IntReader readInt;
    DoubleReader readDouble;
} NumReader;

static uint32_t readIntMatchingByteOrder(const byte *wkbBytes) {
    return *(uint32_t *)wkbBytes;
}
static double readDoubleMatchingByteOrder(const byte *wkbBytes) {
    return *(double *)wkbBytes;
}

static uint32_t readIntMismatchingByteOrder(const byte *wkbBytes) {
    byte t[4];
    for (short i = 0; i < 4; i++) {
        t[i] = wkbBytes[3 - i];
    }
    return *(uint32_t *)t;
}
static double readDoubleMismatchingByteOrder(const byte *wkbBytes) {
    byte t[8];
    for (short i = 0; i < 8; i++) {
        t[i] = wkbBytes[7 - i];
    }
    return *(double *)t;
}

static const NumReader MATCHING_BYTE_ORDER_NUM_READER = {
    "Matching Byte Order NumReader",
    readIntMatchingByteOrder,
    readDoubleMatchingByteOrder,
};

static const NumReader MISMATCHING_BYTE_ORDER_NUM_READER = {
    "Mismatching Byte Order NumReader",
    readIntMismatchingByteOrder,
    readDoubleMismatchingByteOrder,
};

static ByteOrder getMachineByteOrder() {
    byte buffer[4];
    *(uint32_t *)buffer = 1;
    return buffer[0];
}


typedef struct {
    const byte *binaryBuffer;
    StringBuilder * const stringBuilder;

    CoordMode coordMode;
    ByteOrder wkbByteOrder;
    ByteOrder const machineByteOrder;
    const NumReader *numReader;

    uint8_t const decimalPlaces;
    Exception * const exception;
} ProcessingContext;


static uint32_t readInt(
    ProcessingContext * const context) {

    const uint32_t result = context->numReader->readInt(context->binaryBuffer);
    context->binaryBuffer += 4;
    return result;
}

static double readDouble(
    ProcessingContext * const context) {

    const double result = context->numReader->readDouble(context->binaryBuffer);
    context->binaryBuffer += 8;
    return result;
}

static void writeEmpty(
    ProcessingContext * const context) {

    static const char empty[] = "EMPTY";
    StringBuilderAddChars(context->stringBuilder, empty, sizeof(empty));
}

static void writeTaggedGeometryName(
    const ProcessingContext * const context,
    const char * const geometryName) {

    StringBuilderAddChars(
        context->stringBuilder,
        geometryName,
        strlen(geometryName));
    if (context->coordMode.tag) {
        StringBuilderAddChar(context->stringBuilder, ' ');
        StringBuilderAddChars(
            context->stringBuilder,
            context->coordMode.tag,
            context->coordMode.tagLength);
    }
}



typedef void (*LoopVisitor)(
    ProcessingContext * const context);

static void withParenthesesIterate(
    ProcessingContext * const context,
    const LoopVisitor visitor,
    const uint32_t count) {

    Exception * const exception = context->exception;

    StringBuilderAddChar(context->stringBuilder, '(');
    for (uint32_t i = 0; i < count; i++) {
        if (i) {
            StringBuilderAddChar(context->stringBuilder, ',');
        }
        visitor(context);
        if (EXCEPTION_FAILED) {
            return;
        }
    }
    StringBuilderAddChar(context->stringBuilder, ')');
}

static void handlePointSegment(
    ProcessingContext * const context) {

    for (short i = 0; i < context->coordMode.dimensionsCount; i++) {
        if (i) {
            StringBuilderAddChar(context->stringBuilder, ' ');
        }
        const double nextCoord = readDouble(context);
        StringBuilderAddDouble(context->stringBuilder, nextCoord, context->decimalPlaces);
    }
}

static void handleLoop(
    ProcessingContext * const context,
    const LoopVisitor visitor) {

    const uint32_t count = readInt(context);
    if (count) {
        withParenthesesIterate(context, visitor, count);
    } else {
        writeEmpty(context);
    }
}

static void handleLinearRing(
    ProcessingContext * const context) {

    handleLoop(
        context, handlePointSegment);
}


typedef struct GeometryParser_t {
    const char * const nameToPrint;
    const bool hasChildCount;
    const struct GeometryParser_t * const childGeometry;
    const LoopVisitor childParser;
} GeometryParser;

static void handleUnknownGeometry(
    ProcessingContext *context);



static const GeometryParser GEOMETRY_GEOMETRY = {
    // ABSTRACT -- ANY GEOMETRY BELOW QUALIFIES
    "GEOMETRY",
    false,
    NULL,
    writeEmpty,
};
static const GeometryParser GEOMETRY_POINT = {
    "POINT",
    false,
    NULL,
    handlePointSegment,
};
static const GeometryParser GEOMETRY_LINESTRING = {
    "LINESTRING",
    true,
    NULL,
    handlePointSegment,
};
static const GeometryParser GEOMETRY_POLYGON = {
    "POLYGON",
    true,
    NULL,
    handleLinearRing,
};
static const GeometryParser GEOMETRY_MULTIPOINT = {
    "MULTIPOINT",
    true,
    &GEOMETRY_POINT,
    NULL,
};
static const GeometryParser GEOMETRY_MULTILINESTRING = {
    "MULTILINESTRING",
    true,
    &GEOMETRY_LINESTRING,
    NULL,
};
static const GeometryParser GEOMETRY_MULTIPOLYGON = {
    "MULTIPOLYGON",
    true,
    &GEOMETRY_POLYGON,
    NULL,
};
static const GeometryParser GEOMETRY_GEOMETRYCOLLECTION = {
    "GEOMETRYCOLLECTION",
    true,
    NULL,
    handleUnknownGeometry,
};
static const GeometryParser GEOMETRY_CIRCULARSTRING = {
    // RESERVED IN STANDARD (OGC 06-103r4) FOR FUTURE USE
    "CIRCULARSTRING",
    false,
    NULL,
    NULL,
};
static const GeometryParser GEOMETRY_COMPOUNDCURVE = {
    // RESERVED IN STANDARD (OGC 06-103r4) FOR FUTURE USE
    "COMPOUNDCURVE",
    false,
    NULL,
    NULL,
};
static const GeometryParser GEOMETRY_CURVEPOLYGON = {
    // RESERVED IN STANDARD (OGC 06-103r4) FOR FUTURE USE
    "CURVEPOLYGON",
    false,
    NULL,
    NULL,
};
static const GeometryParser GEOMETRY_MULTICURVE = {
    // NON-INSTANTIABLE -- SEE `MultiLineString` SUBCLASS
    "MULTICURVE",
    false,
    NULL,
    NULL,
};
static const GeometryParser GEOMETRY_MULTISURFACE = {
    // NON-INSTANTIABLE -- SEE `MultiPolygon` SUBCLASS
    "MULTISURFACE",
    false,
    NULL,
    NULL,
};
static const GeometryParser GEOMETRY_CURVE = {
    // NON-INSTANTIABLE -- SEE `LineString` SUBCLASS.
    // ALSO `LinearRing` and `Line`
    "CURVE",
    false,
    NULL,
    NULL,
};
static const GeometryParser GEOMETRY_SURFACE = {
    // NON-INSTANTIABLE -- SEE `Polygon` AND `PolyhedralSurface` SUBCLASSES.
    "SURFACE",
    false,
    NULL,
    NULL,
};
static const GeometryParser GEOMETRY_POLYHEDRALSURFACE = {
    "POLYHEDRALSURFACE",
    true,
    &GEOMETRY_POLYGON,
    NULL,
};
static const GeometryParser GEOMETRY_TIN = {
    "TIN",
    true,
    &GEOMETRY_POLYGON,
    NULL,
};
static const GeometryParser GEOMETRY_TRIANGLE = {
    "TRIANGLE",
    true,
    NULL,
    handleLinearRing,
};

static const GeometryParser * const GEOMETRIES[] = {
    &GEOMETRY_GEOMETRY,
    &GEOMETRY_POINT,
    &GEOMETRY_LINESTRING,
    &GEOMETRY_POLYGON,
    &GEOMETRY_MULTIPOINT,
    &GEOMETRY_MULTILINESTRING,
    &GEOMETRY_MULTIPOLYGON,
    &GEOMETRY_GEOMETRYCOLLECTION,
    &GEOMETRY_CIRCULARSTRING,
    &GEOMETRY_COMPOUNDCURVE,
    &GEOMETRY_CURVEPOLYGON,
    &GEOMETRY_MULTICURVE,
    &GEOMETRY_MULTISURFACE,
    &GEOMETRY_CURVE,
    &GEOMETRY_SURFACE,
    &GEOMETRY_POLYHEDRALSURFACE,
    &GEOMETRY_TIN,
    &GEOMETRY_TRIANGLE,
};


static void updateWkbByteOrder(
    ProcessingContext * const context) {

    const ByteOrder newByteOrder = *context->binaryBuffer;
    context->binaryBuffer++;

    if (newByteOrder == context->wkbByteOrder) {
        return;
    }
    context->wkbByteOrder = newByteOrder;
    context->numReader = (
        (context->wkbByteOrder == context->machineByteOrder)
        ? &MATCHING_BYTE_ORDER_NUM_READER
        : &MISMATCHING_BYTE_ORDER_NUM_READER);
}

static void handleKnownGeometry(
    ProcessingContext *context);

static void handleGeometry(
    ProcessingContext * const context,
    const bool typeIsKnown) {

    updateWkbByteOrder(context);

    const uint32_t geometryTypeFull = readInt(context);
    const uint32_t coordType = geometryTypeFull / 1000;
    const uint32_t geometryCode = geometryTypeFull % 1000;

    context->coordMode = CoordModes[coordType];

    static size_t const GeometriesCount =
        sizeof(GEOMETRIES) / sizeof(GEOMETRIES[0]);
    if (geometryCode >= GeometriesCount) {
        Exception * const exception = context->exception;
        // TODO: New status code -- Unknown geometry
        EXCEPTION_SET(FIFTYONE_DEGREES_STATUS_INVALID_INPUT);
        return;
    }

    const GeometryParser * const parser =
        GEOMETRIES[geometryCode];
    if (!typeIsKnown && parser->nameToPrint) {
        writeTaggedGeometryName(context, parser->nameToPrint);
    }

    const LoopVisitor visitor = (parser->childGeometry
        ? handleKnownGeometry
        : parser->childParser);
    if (!visitor) {
        Exception * const exception = context->exception;
        // TODO: New status code -- Abstract/reserved geometry
        EXCEPTION_SET(FIFTYONE_DEGREES_STATUS_INVALID_INPUT);
        return;
    }

    if (parser->hasChildCount) {
        handleLoop(context, visitor);
    } else {
        withParenthesesIterate(context, visitor, 1);
    }
}

static void handleUnknownGeometry(
    ProcessingContext * const context) {

    handleGeometry(context, false);
}

static void handleKnownGeometry(
    ProcessingContext * const context) {

    handleGeometry(context, true);
}

static void handleWKBRoot(
    const byte *binaryBuffer,
    StringBuilder * const stringBuilder,
    uint8_t const decimalPlaces,
    Exception * const exception) {

    ProcessingContext context = {
        binaryBuffer,
        stringBuilder,

        CoordModes[0],
        ~*binaryBuffer,
        getMachineByteOrder(),
        NULL,

        decimalPlaces,
        exception,
    };

    handleUnknownGeometry(&context);
}


void fiftyoneDegreesWriteWkbAsWktToStringBuilder(
    unsigned const char * const wellKnownBinary,
    const uint8_t decimalPlaces,
    fiftyoneDegreesStringBuilder * const builder,
    fiftyoneDegreesException * const exception) {

    handleWKBRoot(wellKnownBinary, builder, decimalPlaces, exception);
}

fiftyoneDegreesWkbtotResult fiftyoneDegreesConvertWkbToWkt(
    const byte * const wellKnownBinary,
    char * const buffer, size_t const length,
    uint8_t const decimalPlaces,
    Exception * const exception) {

    StringBuilder stringBuilder = { buffer, length };
    StringBuilderInit(&stringBuilder);

    handleWKBRoot(wellKnownBinary, &stringBuilder, decimalPlaces, exception);

    StringBuilderComplete(&stringBuilder);

    const fiftyoneDegreesWkbtotResult result = {
        stringBuilder.added,
        stringBuilder.full,
    };
    return result;
}
