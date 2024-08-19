#include "Exceptions.hpp"
#include "Transform.hpp"

using namespace FiftyoneDegrees::Common;

Transform::Transform(size_t capacity) : buffer(capacity) {}

Transform::Headers Transform::apiInvoker(CTransformAPI func,
                                         const std::string &json) {
  Transform::Headers res;
  EXCEPTION_CREATE;

  fiftyoneDegreesKeyValuePairArray *headers = NULL;
  FIFTYONE_DEGREES_ARRAY_CREATE(fiftyoneDegreesKeyValuePair, headers, 8);

  while (1) {
    func(json.c_str(), buffer.data(), buffer.size(), headers, exception);

    switch (exception->status) {
      case FIFTYONE_DEGREES_STATUS_CORRUPT_DATA: {
        fiftyoneDegreesFree(headers);
        EXCEPTION_THROW;
      } break;

      case FIFTYONE_DEGREES_STATUS_INSUFFICIENT_MEMORY: {
        headers->count = 0;
        exception->status = FIFTYONE_DEGREES_STATUS_SUCCESS;
        buffer.resize(buffer.size() * 2);
        continue;
      } break;

      case FIFTYONE_DEGREES_STATUS_SUCCESS: {
        for (uint32_t i = 0; i < headers->count; ++i) {
          fiftyoneDegreesKeyValuePair &pair = headers->items[i];

          res.emplace(std::string{pair.key, pair.keyLength},
                      std::string{pair.value, pair.valueLength});
        }

        fiftyoneDegreesFree(headers);

        return res;
      } break;

      default:
        break;
    }

    break;
  }

  return res;
}
