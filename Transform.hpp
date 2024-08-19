#ifndef FIFTYONE_DEGREES_TRANSFORM_HPP
#define FIFTYONE_DEGREES_TRANSFORM_HPP

#include <map>
#include <string>
#include <vector>

#include "transform.h"

namespace FiftyoneDegrees {
namespace Common {

class Transform {
  using Headers = std::map<std::string, std::string>;

  using CTransformAPI =
      size_t (*)(const char* base64, char* buffer, size_t length,
                 fiftyoneDegreesKeyValuePairArray* const headers,
                 fiftyoneDegreesException* const exception);

  Headers apiInvoker(CTransformAPI func, const std::string& json);

 public:
  Transform(size_t capacity = 1024);

  Headers fromJsonGHEV(const std::string& json) {
    return apiInvoker(fiftyoneDegreesTransformGhevFromJson, json);
  }

  Headers fromBase64GHEV(const std::string& json) {
    return apiInvoker(fiftyoneDegreesTransformGhevFromBase64, json);
  }

  Headers fromSUA(const std::string& json) {
    return apiInvoker(fiftyoneDegreesTransformSua, json);
  }

 private:
  std::vector<char> buffer;
};
}  // namespace Common
}  // namespace FiftyoneDegrees

#endif  // FIFTYONE_DEGREES_TRANSFORM_HPP
