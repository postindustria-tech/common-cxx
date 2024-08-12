#ifndef FIFTYONE_DEGREES_TRANSFORM_HPP
#define FIFTYONE_DEGREES_TRANSFORM_HPP

#include "transform.h"

namespace FiftyoneDegrees {
namespace Common {
class Transform {
  size_t capacity = 0;
  size_t size = 0;
  char *buffer = nullptr;

 public:
  Transform();  // TODO: define ctor signature

  virtual ~Transform();

  // NOTE: maybe use C++ signatures instead of C? I mean std::string,
  // std::vector, throw exception?
  size_t fromJsonGHEV(const char *json,
                      fiftyoneDegreesException *const exception,
                      fiftyoneDegreesKeyValuePairArray *const headers);

  size_t fromBase64GHEV(const char *base64,
                        fiftyoneDegreesException *const exception,
                        fiftyoneDegreesKeyValuePairArray *const headers);

  size_t fromSUA(const char *json, fiftyoneDegreesException *const exception,
                 fiftyoneDegreesKeyValuePairArray *const headers);
};
}  // namespace Common
}  // namespace FiftyoneDegrees

#endif  // FIFTYONE_DEGREES_TRANSFORM_HPP
