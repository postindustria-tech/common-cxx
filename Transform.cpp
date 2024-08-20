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

#include "Exceptions.hpp"
#include "Transform.hpp"

using namespace FiftyoneDegrees::Common;

Transform::Transform(size_t capacity) : buffer(capacity) {}

Transform::Headers Transform::apiInvoker(CTransformAPI func,
                                         const std::string& json) {
  Transform::Headers res;
  EXCEPTION_CREATE;

  fiftyoneDegreesKeyValuePairArray* headers = NULL;
  FIFTYONE_DEGREES_ARRAY_CREATE(fiftyoneDegreesKeyValuePair, headers, 8);

  func(json.c_str(), buffer.data(), buffer.size(), headers, exception);

  while (exception->status == FIFTYONE_DEGREES_STATUS_INSUFFICIENT_MEMORY) {
    headers->count = 0;
    exception->status = FIFTYONE_DEGREES_STATUS_SUCCESS;
    buffer.resize(buffer.size() * 2);

    func(json.c_str(), buffer.data(), buffer.size(), headers, exception);
  }

  if (exception->status == FIFTYONE_DEGREES_STATUS_CORRUPT_DATA) {
    fiftyoneDegreesFree(headers);
    EXCEPTION_THROW;
  }

  for (uint32_t i = 0; i < headers->count; ++i) {
    fiftyoneDegreesKeyValuePair& pair = headers->items[i];

    res.emplace(std::string{pair.key, pair.keyLength},
                std::string{pair.value, pair.valueLength});
  }

  fiftyoneDegreesFree(headers);

  return res;
}

Transform::Headers Transform::fromJsonGHEV(const std::string& json) {
  return apiInvoker(fiftyoneDegreesTransformGhevFromJson, json);
}

Transform::Headers Transform::fromBase64GHEV(const std::string& json) {
  return apiInvoker(fiftyoneDegreesTransformGhevFromBase64, json);
}

Transform::Headers Transform::fromSUA(const std::string& json) {
  return apiInvoker(fiftyoneDegreesTransformSua, json);
}
