#include "Transform.hpp"

#include "Exceptions.hpp"

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
