#include "Transform.hpp"

using namespace FiftyoneDegrees::Common;

// WARNING: tmp allocation
Transform::Transform() : capacity(4096), buffer(new char[capacity]) {
  // TODO: allocate memory in correct way
}

Transform::~Transform() {
  // TODO: deallocate memory
  // delete[] buffer;
}

size_t Transform::fromJsonGHEV(
    const char *json, fiftyoneDegreesException *const exception,
    fiftyoneDegreesKeyValuePairArray *const headers) {
  return fiftyoneDegreesTransformGhevFromJson(
      json, buffer + size, capacity - size, exception, headers);
}

size_t Transform::fromBase64GHEV(
    const char *base64, fiftyoneDegreesException *const exception,
    fiftyoneDegreesKeyValuePairArray *const headers) {
  return fiftyoneDegreesTransformGhevFromBase64(
      base64, buffer + size, capacity - size, exception, headers);
}

size_t Transform::fromSUA(const char *json,
                          fiftyoneDegreesException *const exception,
                          fiftyoneDegreesKeyValuePairArray *const headers) {
  return fiftyoneDegreesTransformSua(json, buffer + size, capacity - size,
                                     exception, headers);
}

// NOTE: this wrapper potentially should implement reallocation logic, isn't it?
