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

#include "wkbtot.hpp"

#include <memory>

#include "fiftyone.h"

namespace FiftyoneDegrees::Common {
    /**
     * The length for the buffer most WKT strings are expected to fit into.
     */
    static constexpr size_t reasonableWktStringLength = 128;

    WkbtotResult writeWkbStringToStringStream(
        const String * const wkbString,
        std::stringstream &stream,
        const uint8_t decimalPlaces,
        Exception * const exception) {

        WkbtotResult toWktResult = {
            0,
            false,
        };

        if (!wkbString || !exception) {
            EXCEPTION_SET(FIFTYONE_DEGREES_STATUS_NULL_POINTER);
            return toWktResult;
        }
        const auto * const wkbBytes = FIFTYONE_DEGREES_WKB(wkbString);
        if (!wkbBytes) {
            EXCEPTION_SET(FIFTYONE_DEGREES_STATUS_INVALID_INPUT);
            return toWktResult;
        }

        {
            char buffer[reasonableWktStringLength];
            toWktResult = ConvertWkbToWkt(
                wkbBytes,
                buffer,
                reasonableWktStringLength,
                decimalPlaces,
                exception
                );
            if (EXCEPTION_OKAY && !toWktResult.bufferTooSmall) {
                stream << buffer;
                return toWktResult;
            }
        }
        if (toWktResult.bufferTooSmall) {
            EXCEPTION_CLEAR;
            const size_t requiredSize = toWktResult.written + 1;
            const std::unique_ptr<char[]> buffer = std::make_unique<char[]>(requiredSize);
            toWktResult = fiftyoneDegreesConvertWkbToWkt(
                wkbBytes,
                buffer.get(),
                requiredSize,
                decimalPlaces,
                exception
                );
            if (EXCEPTION_OKAY && !toWktResult.bufferTooSmall) {
                stream << buffer.get();
                return toWktResult;
            }
        }
        return toWktResult;
    }
}
