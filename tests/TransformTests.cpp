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

#include "../memory.h"
// #include "../transform.h"
#include "../Transform.hpp"
#include "Base.hpp"

class Transform : public Base {
 public:
  virtual void SetUp();
  virtual void TearDown();
  static bool found;
  static fiftyoneDegreesKeyValuePairArray *results;
  static fiftyoneDegreesException exception;
  static const char *expectedFieldName;
  static const char *expectedFieldValue;
  void checkFieldValue(const char *field, const char *value);
  void checkFieldAbsent(const char *field);
};

bool Transform::found = false;
const char *Transform::expectedFieldName = NULL;
const char *Transform::expectedFieldValue = NULL;
fiftyoneDegreesKeyValuePairArray *Transform::results = NULL;
fiftyoneDegreesException Transform::exception;

void Transform::checkFieldValue(const char *field, const char *value) {
  found = false;
  expectedFieldName = field;
  expectedFieldValue = value;
  size_t expectedFieldName_len = strlen(expectedFieldName);
  size_t expectedFieldValue_len = strlen(expectedFieldValue);

  for (size_t i = 0; i < results->count; ++i) {
    fiftyoneDegreesKeyValuePair *pair = &results->items[i];

    if (expectedFieldName_len == pair->keyLength) {
      found = true;

      for (size_t j = 0; j < pair->keyLength; ++j) {
        if (pair->key[j] != expectedFieldName[j]) {
          found = false;
          break;
        }
      }

      if (found) {
        EXPECT_TRUE(expectedFieldValue_len == pair->valueLength)
            << L"Expected value len to be '" << expectedFieldValue_len
            << "' not '" << pair->valueLength << "'";

        if (expectedFieldValue_len == pair->valueLength) {
          bool value_compare = true;

          for (size_t j = 0; j < pair->valueLength; ++j) {
            if (pair->value[j] != expectedFieldValue[j]) {
              value_compare = false;
              break;
            }
          }

          EXPECT_TRUE(value_compare)
              << L"Expected value to be '" << expectedFieldValue << "' not '"
              << (const char *)pair->value << "'";

          break;
        }
      }
    }
  }

  ASSERT_TRUE(found) << "Field " << field << " was not found should be "
                     << value;
}

void Transform::checkFieldAbsent(const char *field) {
  found = false;
  expectedFieldName = field;
  size_t expectedFieldName_len = strlen(expectedFieldName);

  for (size_t i = 0; i < results->count; ++i) {
    fiftyoneDegreesKeyValuePair *pair = &results->items[i];

    if (expectedFieldName_len == pair->keyLength) {
      found = true;

      for (size_t j = 0; j < pair->keyLength; ++j) {
        if (pair->key[j] != expectedFieldName[j]) {
          found = false;
          break;
        }
      }

      if (found) {
        break;
      }
    }
  }

  ASSERT_FALSE(found) << "Field " << field << " should be absent";
}

bool fillResultsCallback(void *ctx, fiftyoneDegreesKeyValuePair pair) {
  fiftyoneDegreesKeyValuePairArray *results =
      (fiftyoneDegreesKeyValuePairArray *)ctx;

  if (results->count < results->capacity) {
    results->items[results->count++] = pair;
    return true;
  }

  return false;
}

void Transform::SetUp() {
  Base::SetUp();
  FIFTYONE_DEGREES_ARRAY_CREATE(fiftyoneDegreesKeyValuePair, results, 8)
}

void Transform::TearDown() {
  fiftyoneDegreesFree(results);
  Base::TearDown();
}
// Tests
// ------------------------------------------------------------------------------------------

TEST_F(Transform, GHEVIterativeJSON) {
  const char *ghev =
      "{\"architecture\":\"x86\",\"brands\":[{\"brand\":\"Not/"
      "A)Brand\",\"version\":\"8\"},{\"brand\":\"Chromium\",\"version\":"
      "\"126\"},{\"brand\":\"Google "
      "Chrome\",\"version\":\"126\"}],\"fullVersionList\":[{\"brand\":\"Not/"
      "A)Brand\",\"version\":\"8.0.0.0\"},{\"brand\":\"Chromium\",\"version\":"
      "\"126.0.6478.61\"},{\"brand\":\"Google "
      "Chrome\",\"version\":\"126.0.6478.61\"}],\"mobile\":false,\"model\":"
      "\"\",\"platform\":\"macOS\",\"platformVersion\":\"14.5.0\"}";

  size_t bufferLength = strlen(ghev);
  char *buffer = (char *)fiftyoneDegreesMalloc(bufferLength);

  size_t count = fiftyoneDegreesTransformIterateGhevFromJson(
      ghev, buffer, bufferLength, &Transform::exception, fillResultsCallback,
      Transform::results);

  // ---

  ASSERT_EQ(exception.status, FIFTYONE_DEGREES_STATUS_SUCCESS);

  ASSERT_EQ(count, 7);
  ASSERT_EQ(results->count, count);

  checkFieldValue("sec-ch-ua-arch", "\"x86\"");
  checkFieldValue("sec-ch-ua",
                  "\"Not/A)Brand\";v=\"8\", \"Chromium\";v=\"126\", \"Google "
                  "Chrome\";v=\"126\"");
  checkFieldAbsent("sec-ch-ua-bitness");
  checkFieldValue(
      "sec-ch-ua-full-version-list",
      "\"Not/A)Brand\";v=\"8.0.0.0\", \"Chromium\";v=\"126.0.6478.61\", "
      "\"Google Chrome\";v=\"126.0.6478.61\"");
  checkFieldValue("sec-ch-ua-mobile", "?0");
  checkFieldValue("sec-ch-ua-model", "\"\"");
  checkFieldValue("sec-ch-ua-platform", "\"macOS\"");
  checkFieldValue("sec-ch-ua-platform-version", "\"14.5.0\"");
  fiftyoneDegreesFree(buffer);
}

TEST_F(Transform, IncompleteJSON) {
  size_t bufferLength = 4096;
  char *buffer = (char *)fiftyoneDegreesMalloc(bufferLength);

  std::vector<std::string> correct{
      "{ \"key_without_value\" }",

      "{ \"key_without_value\": ",

      "{\"architecture\":\"x86\","
      " \"incomplete_unknown_object\": { \"other_nested_object\": {    \n",

      "{ \"incomplete_string\": \"    \n",
      "{ \"complete_string\":   \" \" \n",

      "{\"incomplete_unknown_object\": { \"other_nested_object",

      "{\"incomplete_unknown_array\": [ \"other_nested_string",

      "{\"incomplete_unknown_array\": [",

      "{\"incomplete_unknown_array\": [[",

      "{\"incomplete_unknown_array\": [[],\"",
      "{\"incomplete_unknown_array\": [[],\"\"",
      "{\"complete_unknown_array\": [],",

      "{ \"incomplete_bool\": false",

      "{ \"\": \"empty_key\" }",

      "{\"bool\": true}",

      "{\"more\": true}",

      "{\"platformer\": 0}",

  };

  std::vector<std::string> corrupted{
      "{ \"model\": n",
      "{ \"model\": nu",
      "{ \"model\": \"he",
      "{ \"model\": \"he\\",
      "",
      "{ \"",
      "{ \"mobile\":",
      "{ \"mo",
      "{\"a",
      "{\"brands\":[{\"brand\": \"one\", \"version\":null}}",
  };

  for (const std::string &j : correct) {
    fiftyoneDegreesTransformIterateGhevFromJson(
        j.c_str(), buffer, bufferLength, &Transform::exception,
        fillResultsCallback, Transform::results);

    ASSERT_EQ(exception.status, FIFTYONE_DEGREES_STATUS_SUCCESS);
  }

  for (const std::string &j : corrupted) {
    fiftyoneDegreesTransformIterateGhevFromJson(
        j.c_str(), buffer, bufferLength, &Transform::exception,
        fillResultsCallback, Transform::results);

    ASSERT_EQ(exception.status, FIFTYONE_DEGREES_STATUS_CORRUPT_DATA);
  }
  fiftyoneDegreesFree(buffer);
}

TEST_F(Transform, IncompleteSUA) {
  size_t bufferLength = 4096;
  char *buffer = (char *)fiftyoneDegreesMalloc(bufferLength);

  std::vector<std::string> correct{
      "{ \"key_without_value\" }",

      "{ \"key_without_value\": ",

      "{ \"skip\": { \"key\": \"\\\"\\n\\\\\"value\\\"\" } }",

      "{\"architecture\":\"x86\\\"\","
      " \"incomplete_unknown_object\": { \"other_nested_object\": {    \n",

      "{ \"incomplete_string\": \"    \n",
      "{ \"complete_string\":   \" \" \n",

      "{\"incomplete_unknown_object\": { \"other_nested_object",

      "{\"incomplete_unknown_array\": [ \"other_nested_string",

      "{\"incomplete_unknown_array\": [",

      "{\"incomplete_unknown_array\": [[",

      "{\"incomplete_unknown_array\": [[],\"",
      "{\"incomplete_unknown_array\": [[],\"\"",
      "{\"complete_unknown_array\": [],",

      "{ \"incomplete_bool\": false",

      "{ \"\": \"empty_key\" }",

      "{\"bool\": true}",

      "{\"more\": true}",
      "{\"browsers\":[{\"brand\": null}]}",
  };

  std::vector<std::string> corrupted{
      "",
      "{ \"",
      "{\"a",
      "{ \"mo",
      "{ \"mobile\":",
      "{\"platformer\": 0}",
      "{\"browsers\":[{\"brand\": null}}}",
  };

  for (const std::string &j : correct) {
    fiftyoneDegreesTransformIterateSua(j.c_str(), buffer, bufferLength,
                                       &Transform::exception,
                                       fillResultsCallback, Transform::results);

    ASSERT_EQ(exception.status, FIFTYONE_DEGREES_STATUS_SUCCESS);
  }

  for (const std::string &j : corrupted) {
    fiftyoneDegreesTransformIterateSua(j.c_str(), buffer, bufferLength,
                                       &Transform::exception,
                                       fillResultsCallback, Transform::results);

    ASSERT_EQ(exception.status, FIFTYONE_DEGREES_STATUS_CORRUPT_DATA);
  }
  fiftyoneDegreesFree(buffer);
}

TEST_F(Transform, GHEVIncorrectBool) {
  const char *ghev =
      "{\"architecture\":\"x86\","
      "\"brands\":[{\"brand\": null, \"version\":\"8\"},"
      "{\"brand\": null\n},{\"brand\":\"Google Chrome\",\"version\":\"126\"}],"
      "\"fullVersionList\":[{\"brand\": null}],"
      "\"mobile\": 0,\"model\":"
      "\"\",\"platform\":\"macOS\",\"platformVersion\":\"14.5.0\"}";

  size_t bufferLength = strlen(ghev);
  char *buffer = (char *)fiftyoneDegreesMalloc(bufferLength);

  fiftyoneDegreesTransformIterateGhevFromJson(
      ghev, buffer, bufferLength, &Transform::exception, fillResultsCallback,
      Transform::results);

  // ---

  ASSERT_EQ(exception.status, FIFTYONE_DEGREES_STATUS_CORRUPT_DATA);
  fiftyoneDegreesFree(buffer);
}

TEST_F(Transform, SUAIncorrectBool) {
  const char *json = "{\"mobile\": false,\"model\":\"\"}";

  size_t bufferLength = 512;
  char *buffer = (char *)fiftyoneDegreesMalloc(bufferLength);

  fiftyoneDegreesTransformIterateSua(json, buffer, bufferLength,
                                     &Transform::exception, fillResultsCallback,
                                     Transform::results);

  // ---

  ASSERT_EQ(exception.status, FIFTYONE_DEGREES_STATUS_CORRUPT_DATA);
  fiftyoneDegreesFree(buffer);
}

TEST_F(Transform, GHEVIterativeNULLBrandJSON) {
  const char *ghev =
      "{\"architecture\":\"x86\","
      "\"brands\":[{\"brand\": null, \"version\":\"8\"},"
      "{\"brand\": null\n},{\"brand\":\"Google Chrome\",\"version\":\"126\"}],"
      "\"fullVersionList\":[{\"brand\": null}],"
      "\"mobile\":false,\"model\":"
      "\"\",\"platform\":\"macOS\",\"platformVersion\":\"14.5.0\"}";

  size_t bufferLength = strlen(ghev);
  char *buffer = (char *)fiftyoneDegreesMalloc(bufferLength);

  size_t count = fiftyoneDegreesTransformIterateGhevFromJson(
      ghev, buffer, bufferLength, &Transform::exception, fillResultsCallback,
      Transform::results);

  // ---

  ASSERT_EQ(exception.status, FIFTYONE_DEGREES_STATUS_SUCCESS);

  ASSERT_EQ(count, 6);
  ASSERT_EQ(results->count, count);

  checkFieldValue("sec-ch-ua-arch", "\"x86\"");
  checkFieldValue("sec-ch-ua", "\"Google Chrome\";v=\"126\"");
  checkFieldAbsent("sec-ch-ua-bitness");
  checkFieldAbsent("sec-ch-ua-full-version-list");
  checkFieldValue("sec-ch-ua-mobile", "?0");
  checkFieldValue("sec-ch-ua-model", "\"\"");
  checkFieldValue("sec-ch-ua-platform", "\"macOS\"");
  checkFieldValue("sec-ch-ua-platform-version", "\"14.5.0\"");
  fiftyoneDegreesFree(buffer);
}

TEST_F(Transform, GHEVIterativeNULLBrandVersionJSON) {
  const char *ghev =
      "{\"architecture\":\"x86\","
      "\"brands\":[{\"brand\": \"one\", \"version\":null},"
      "{\"brand\": null\n},{\"brand\":\"Google Chrome\",\"version\":\"126\"}],"
      "\"fullVersionList\":[{\"brand\": null}],"
      "\"mobile\":false,\"model\":"
      "\"\",\"platform\":\"macOS\",\"platformVersion\":\"14.5.0\"}";

  size_t bufferLength = strlen(ghev);
  char *buffer = (char *)fiftyoneDegreesMalloc(bufferLength);

  size_t count = fiftyoneDegreesTransformIterateGhevFromJson(
      ghev, buffer, bufferLength, &Transform::exception, fillResultsCallback,
      Transform::results);

  // ---

  ASSERT_EQ(exception.status, FIFTYONE_DEGREES_STATUS_SUCCESS);

  ASSERT_EQ(count, 6);
  ASSERT_EQ(results->count, count);

  checkFieldValue("sec-ch-ua-arch", "\"x86\"");
  checkFieldValue("sec-ch-ua", "\"one\";v=null, \"Google Chrome\";v=\"126\"");
  checkFieldAbsent("sec-ch-ua-bitness");
  checkFieldAbsent("sec-ch-ua-full-version-list");
  checkFieldValue("sec-ch-ua-mobile", "?0");
  checkFieldValue("sec-ch-ua-model", "\"\"");
  checkFieldValue("sec-ch-ua-platform", "\"macOS\"");
  checkFieldValue("sec-ch-ua-platform-version", "\"14.5.0\"");
  fiftyoneDegreesFree(buffer);
}

TEST_F(Transform, GHEVIterativeBase64) {
  const char *ghev =
      "eyJicmFuZHMiOlt7ImJyYW5kIjoiTm90L0EpQnJhbmQiLCJ2ZXJzaW9uIjoiOCJ9LHsiYnJh"
      "bmQiOiJDaHJvbWl1bSIsInZlcnNpb24iOiIxMjYifSx7ImJyYW5kIjoiR29vZ2xlIENocm9t"
      "ZSIsInZlcnNpb24iOiIxMjYifV0sImZ1bGxWZXJzaW9uTGlzdCI6W3siYnJhbmQiOiJOb3Qv"
      "QSlCcmFuZCIsInZlcnNpb24iOiI4LjAuMC4wIn0seyJicmFuZCI6IkNocm9taXVtIiwidmVy"
      "c2lvbiI6IjEyNi4wLjY0NzguMTI3In0seyJicmFuZCI6Ikdvb2dsZSBDaHJvbWUiLCJ2ZXJz"
      "aW9uIjoiMTI2LjAuNjQ3OC4xMjcifV0sIm1vYmlsZSI6ZmFsc2UsIm1vZGVsIjoiIiwicGxh"
      "dGZvcm0iOiJtYWNPUyIsInBsYXRmb3JtVmVyc2lvbiI6IjE0LjUuMCJ9";

  size_t bufferLength = 686;  // strlen(ghev);
  char *buffer = (char *)fiftyoneDegreesMalloc(bufferLength);

  size_t count = fiftyoneDegreesTransformIterateGhevFromBase64(
      ghev, buffer, bufferLength, &Transform::exception, fillResultsCallback,
      Transform::results);

  // ---

  ASSERT_EQ(exception.status, FIFTYONE_DEGREES_STATUS_SUCCESS);

  ASSERT_EQ(count, 6);
  ASSERT_EQ(results->count, count);

  checkFieldAbsent("sec-ch-ua-arch");
  checkFieldValue("sec-ch-ua",
                  "\"Not/A)Brand\";v=\"8\", \"Chromium\";v=\"126\", \"Google "
                  "Chrome\";v=\"126\"");
  checkFieldAbsent("sec-ch-ua-bitness");
  checkFieldValue(
      "sec-ch-ua-full-version-list",
      "\"Not/A)Brand\";v=\"8.0.0.0\", \"Chromium\";v=\"126.0.6478.127\", "
      "\"Google Chrome\";v=\"126.0.6478.127\"");
  checkFieldValue("sec-ch-ua-mobile", "?0");
  checkFieldValue("sec-ch-ua-model", "\"\"");
  checkFieldValue("sec-ch-ua-platform", "\"macOS\"");
  checkFieldValue("sec-ch-ua-platform-version", "\"14.5.0\"");
  fiftyoneDegreesFree(buffer);
}

TEST_F(Transform, GHEVBase64CorruptedLen) {
  const char *ghev =
      "eyJicmFuZHMiOlt7ImJyYW5kIjoiTm90L0EpQnJhbmQiLCJ2ZXJzaW9uIjoiOCJ9LHsiYnJh"
      "bmQiOiJDaHJvbWl1bSIsInZlcnNpb24iOiIxMjYifSx7ImJyYW5kIjoiR29vZ2xlIENocm9t"
      "ZSIsInZlcnNpb24iOiIxMjYifV0sImZ1bGxWZXJzaW9uTGlzdCI6W3siYnJhbmQiOiJOb3Qv"
      "QSlCcmFuZCIsInZlcnNpb24iOiI4Lj>"
      "AuMC4wIn0seyJicmFuZCI6IkNocm9taXVtIiwidmVy"
      "c2lvbiI6IjEyNi4wLjY0NzguMTI3In0seyJicmFuZCI6Ikdvb2dsZSBDaHJvbWUiLCJ2ZXJz"
      "aW9uIjoiMTI2LjAuNjQ3OC4xMjcifV0sIm1vYmlsZSI6ZmFsc2UsIm1vZGVsIjoiIiwicGxh"
      "dGZvcm0iOiJtYWNPUyIsInBsYXRmb3JtVmVyc2lvbiI6IjE0LjUuMCJ9";

  size_t bufferLength = 686;  // strlen(ghev);
  char *buffer = (char *)fiftyoneDegreesMalloc(bufferLength);

  fiftyoneDegreesTransformIterateGhevFromBase64(
      ghev, buffer, bufferLength, &Transform::exception, fillResultsCallback,
      Transform::results);

  ASSERT_EQ(exception.status, FIFTYONE_DEGREES_STATUS_CORRUPT_DATA);
  fiftyoneDegreesFree(buffer);
}

TEST_F(Transform, GHEVBase64CorruptedSymbol) {
  const char *ghev =
      "====cmFuZHMiOlt7ImJyYW5kIjoiTm90L0EpQnJhbmQiLCJ2ZXJzaW9uIjoiOCJ9LHsiYnJh"
      "bmQiOiJDaHJvbWl1bSIsInZlcnNpb24iOiIxMjYifSx7ImJyYW5kIjoiR29vZ2xlIENocm9t"
      "ZSIsInZlcnNpb24iOiIxMjYifV0sImZ1bGxWZXJzaW9uTGlzdCI6W3siYnJhbmQiOiJOb3Qv"
      "QSlCcmFuZCIsInZlcnNpb24iOiI4Lj>>>>"
      "AuMC4wIn0seyJicmFuZCI6IkNocm9taXVtIiwidmVy"
      "c2lvbiI6IjEyNi4wLjY0NzguMTI3In0seyJicmFuZCI6Ikdvb2dsZSBDaHJvbWUiLCJ2ZXJz"
      "aW9uIjoiMTI2LjAuNjQ3OC4xMjcifV0sIm1vYmlsZSI6ZmFsc2UsIm1vZGVsIjoiIiwicGxh"
      "dGZvcm0iOiJtYWNPUyIsInBsYXRmb3JtVmVyc2lvbiI6IjE0LjUuMCJ9";

  size_t bufferLength = 686;  // strlen(ghev);
  char *buffer = (char *)fiftyoneDegreesMalloc(bufferLength);

  fiftyoneDegreesTransformIterateGhevFromBase64(
      ghev, buffer, bufferLength, &Transform::exception, fillResultsCallback,
      Transform::results);

  ASSERT_EQ(exception.status, FIFTYONE_DEGREES_STATUS_CORRUPT_DATA);
  fiftyoneDegreesFree(buffer);
}

TEST_F(Transform, GHEVBase64CorruptedSymbol2) {
  const char *ghev =
      "&&&&cmFuZHMiOlt7ImJyYW5kIjoiTm90L0EpQnJhbmQiLCJ2ZXJzaW9uIjoiOCJ9LHsiYnJh"
      "bmQiOiJDaHJvbWl1bSIsInZlcnNpb24iOiIxMjYifSx7ImJyYW5kIjoiR29vZ2xlIENocm9t"
      "ZSIsInZlcnNpb24iOiIxMjYifV0sImZ1bGxWZXJzaW9uTGlzdCI6W3siYnJhbmQiOiJOb3Qv"
      "QSlCcmFuZCIsInZlcnNpb24iOiI4Lj>>>>"
      "AuMC4wIn0seyJicmFuZCI6IkNocm9taXVtIiwidmVy"
      "c2lvbiI6IjEyNi4wLjY0NzguMTI3In0seyJicmFuZCI6Ikdvb2dsZSBDaHJvbWUiLCJ2ZXJz"
      "aW9uIjoiMTI2LjAuNjQ3OC4xMjcifV0sIm1vYmlsZSI6ZmFsc2UsIm1vZGVsIjoiIiwicGxh"
      "dGZvcm0iOiJtYWNPUyIsInBsYXRmb3JtVmVyc2lvbiI6IjE0LjUuMCJ9";

  size_t bufferLength = 686;  // strlen(ghev);
  char *buffer = (char *)fiftyoneDegreesMalloc(bufferLength);

  fiftyoneDegreesTransformIterateGhevFromBase64(
      ghev, buffer, bufferLength, &Transform::exception, fillResultsCallback,
      Transform::results);

  ASSERT_EQ(exception.status, FIFTYONE_DEGREES_STATUS_CORRUPT_DATA);
  fiftyoneDegreesFree(buffer);
}

TEST_F(Transform, GHEVIterativeSua) {
  const char *sua =
      "{\n        \"source\": 2,\n        \"platform\": {\n                "
      "\"brand\": \"macOS\",\n                \"version\": [\"14\", \"5\", "
      "\"0\"]\n        },\n        \"browsers\": [{\n                "
      "\"brand\": \"Not/A)Brand\",\n                \"version\": [\"8\", "
      "\"0\", \"0\", \"0\"]\n        }, {\n                \"brand\": "
      "\"Chromium\",\n                \"version\": [\"126\", \"0\", \"6478\", "
      "\"127\"]\n        }, {\n                \"brand\": \"Google Chrome\",\n "
      "               \"version\": [\"126\", \"0\", \"6478\", \"127\"]\n       "
      " }],\n        \"mobile\": 0,\n        \"model\": \"\",\n        "
      "\"architecture\": \"x86\",\n        \"bitness\": \"64\"\n}";

  size_t bufferLength = strlen(sua);
  char *buffer = (char *)fiftyoneDegreesMalloc(bufferLength);

  size_t count = fiftyoneDegreesTransformIterateSua(
      sua, buffer, bufferLength, &Transform::exception, fillResultsCallback,
      Transform::results);

  // ---

  ASSERT_EQ(exception.status, FIFTYONE_DEGREES_STATUS_SUCCESS);

  ASSERT_EQ(count, 7);
  ASSERT_EQ(results->count, count);

  checkFieldValue("sec-ch-ua-arch", "\"x86\"");
  checkFieldAbsent("sec-ch-ua");
  checkFieldValue("sec-ch-ua-bitness", "\"64\"");
  checkFieldValue(
      "sec-ch-ua-full-version-list",
      "\"Not/A)Brand\";v=\"8.0.0.0\", \"Chromium\";v=\"126.0.6478.127\", "
      "\"Google Chrome\";v=\"126.0.6478.127\"");
  checkFieldValue("sec-ch-ua-mobile", "?0");
  checkFieldValue("sec-ch-ua-model", "\"\"");
  checkFieldValue("sec-ch-ua-platform", "\"macOS\"");
  checkFieldValue("sec-ch-ua-platform-version", "\"14.5.0\"");
  fiftyoneDegreesFree(buffer);
}

TEST_F(Transform, SuaWeirdPlatformVersion) {
  const char *sua =
      "{\n        \"source\": 2,\n        \"platform\": {\n                "
      "\"brand\": \"macOS\",\n                \"version\": [\"\\\"x\\\"\", "
      "\"\\\"y\\\"\", "
      "\"\\\"z\\\"\"]\n        },\n        \"browsers\": [{\n                "
      "\"brand\": \"Not/A)Brand\",\n                \"version\": [\"8\", "
      "\"0\", \"0\", \"0\"]\n        }, {\n                \"brand\": "
      "\"Chromium\",\n                \"version\": [\"126\", \"0\", \"6478\", "
      "\"127\"]\n        }, {\n                \"brand\": \"Google Chrome\",\n "
      "               \"version\": [\"126\", \"0\", \"6478\", \"127\"]\n       "
      " }],\n        \"mobile\": 0,\n        \"model\": \"\",\n        "
      "\"architecture\": \"x86\",\n        \"bitness\": \"64\"\n}";

  size_t bufferLength = strlen(sua);
  char *buffer = (char *)fiftyoneDegreesMalloc(bufferLength);

  size_t count = fiftyoneDegreesTransformIterateSua(
      sua, buffer, bufferLength, &Transform::exception, fillResultsCallback,
      Transform::results);

  // ---

  ASSERT_EQ(exception.status, FIFTYONE_DEGREES_STATUS_SUCCESS);

  ASSERT_EQ(count, 7);
  ASSERT_EQ(results->count, count);

  checkFieldValue("sec-ch-ua-arch", "\"x86\"");
  checkFieldAbsent("sec-ch-ua");
  checkFieldValue("sec-ch-ua-bitness", "\"64\"");
  checkFieldValue(
      "sec-ch-ua-full-version-list",
      "\"Not/A)Brand\";v=\"8.0.0.0\", \"Chromium\";v=\"126.0.6478.127\", "
      "\"Google Chrome\";v=\"126.0.6478.127\"");
  checkFieldValue("sec-ch-ua-mobile", "?0");
  checkFieldValue("sec-ch-ua-model", "\"\"");
  checkFieldValue("sec-ch-ua-platform", "\"macOS\"");
  checkFieldValue("sec-ch-ua-platform-version",
                  "\"\\\"x\\\".\\\"y\\\".\\\"z\\\"\"");
  fiftyoneDegreesFree(buffer);
}

TEST_F(Transform, SuaNullBrandPlatform) {
  const char *sua =
      "{\n        \"source\": 2,\n        \"platform\": {\n                "
      "\"brand\": null,\n                \"version\": [\"\\\"x\\\"\", "
      "\"\\\"y\\\"\", "
      "\"\\\"z\\\"\"]\n        },\n        \"browsers\": [{\n                "
      "\"brand\": \"Not/A)Brand\",\n                \"version\": [\"8\", "
      "\"0\", \"0\", \"0\"]\n        }, {\n                \"brand\": "
      "\"Chromium\",\n                \"version\": [\"126\", \"0\", \"6478\", "
      "\"127\"]\n        }, {\n                \"brand\": \"Google Chrome\",\n "
      "               \"version\": [\"126\", \"0\", \"6478\", \"127\"]\n       "
      " }],\n        \"mobile\": 0,\n        \"model\": \"\",\n        "
      "\"architecture\": \"x86\",\n        \"bitness\": \"64\"\n}";

  size_t bufferLength = strlen(sua);
  char *buffer = (char *)fiftyoneDegreesMalloc(bufferLength);

  size_t count = fiftyoneDegreesTransformIterateSua(
      sua, buffer, bufferLength, &Transform::exception, fillResultsCallback,
      Transform::results);

  // ---

  ASSERT_EQ(exception.status, FIFTYONE_DEGREES_STATUS_SUCCESS);

  ASSERT_EQ(count, 5);
  ASSERT_EQ(results->count, count);

  checkFieldValue("sec-ch-ua-arch", "\"x86\"");
  checkFieldAbsent("sec-ch-ua");
  checkFieldValue("sec-ch-ua-bitness", "\"64\"");
  checkFieldValue(
      "sec-ch-ua-full-version-list",
      "\"Not/A)Brand\";v=\"8.0.0.0\", \"Chromium\";v=\"126.0.6478.127\", "
      "\"Google Chrome\";v=\"126.0.6478.127\"");
  checkFieldValue("sec-ch-ua-mobile", "?0");
  checkFieldValue("sec-ch-ua-model", "\"\"");
  checkFieldAbsent("sec-ch-ua-platform");
  checkFieldAbsent("sec-ch-ua-platform-version");
  fiftyoneDegreesFree(buffer);
}

TEST_F(Transform, GHEVArrayJSON) {
  const char *ghev =
      "{\"architecture\":\"x86\",\"bitness\":\"64\",\"brands\":[{\"brand\":"
      "\"Not/"
      "A)Brand\",\"version\":\"8\"},{\"brand\":\"Chromium\",\"version\":"
      "\"126\"},{\"brand\":\"Google Chrome\",\"version\":\"126\"}],\"mobile\"  "
      " :   false,  \"model\"  :   \"MacBook\" ,  \"platform\" : "
      "\"macOS\",\"platformVersion\":\"14.5.0\",\"wow64\":false}";

  size_t bufferLength = strlen(ghev);
  char *buffer = (char *)fiftyoneDegreesMalloc(bufferLength);

  size_t count = fiftyoneDegreesTransformGhevFromJson(
      ghev, buffer, bufferLength, &Transform::exception, results);

  // ---

  ASSERT_EQ(exception.status, FIFTYONE_DEGREES_STATUS_SUCCESS);

  ASSERT_EQ(count, 7);
  ASSERT_EQ(results->count, count);

  checkFieldValue("sec-ch-ua-arch", "\"x86\"");
  checkFieldValue("sec-ch-ua",
                  "\"Not/A)Brand\";v=\"8\", \"Chromium\";v=\"126\", \"Google "
                  "Chrome\";v=\"126\"");
  checkFieldValue("sec-ch-ua-bitness", "\"64\"");
  checkFieldAbsent("sec-ch-ua-full-version-list");
  checkFieldValue("sec-ch-ua-mobile", "?0");
  checkFieldValue("sec-ch-ua-model", "\"MacBook\"");
  checkFieldValue("sec-ch-ua-platform", "\"macOS\"");
  checkFieldValue("sec-ch-ua-platform-version", "\"14.5.0\"");
  fiftyoneDegreesFree(buffer);
}

TEST_F(Transform, GHEVArrayInsufficientCapacity) {
  const char *ghev =
      "{\"architecture\":\"x86\",\"bitness\":\"64\",\"brands\":[{\"brand\":"
      "\"Not/"
      "A)Brand\",\"version\":\"8\"},{\"brand\":\"Chromium\",\"version\":"
      "\"126\"},{\"brand\":\"Google Chrome\",\"version\":\"126\"}],\"mobile\"  "
      " :   false,  \"model\"  :   \"MacBook\" ,  \"platform\" : "
      "\"macOS\",\"platformVersion\":\"14.5.0\",\"wow64\":false}";

  size_t bufferLength = strlen(ghev);
  char *buffer = (char *)fiftyoneDegreesMalloc(bufferLength);

  fiftyoneDegreesKeyValuePairArray *headers = NULL;
  FIFTYONE_DEGREES_ARRAY_CREATE(fiftyoneDegreesKeyValuePair, headers, 2);

  fiftyoneDegreesKeyValuePairArray *empty_headers = NULL;
  FIFTYONE_DEGREES_ARRAY_CREATE(fiftyoneDegreesKeyValuePair, empty_headers, 0);

  size_t count = fiftyoneDegreesTransformGhevFromJson(
      ghev, buffer, bufferLength, &Transform::exception, headers);

  ASSERT_EQ(exception.status, FIFTYONE_DEGREES_STATUS_SUCCESS);

  ASSERT_EQ(count, 2);
  ASSERT_EQ(headers->count, count);

  // ---

  count = fiftyoneDegreesTransformGhevFromJson(
      ghev, buffer, bufferLength, &Transform::exception, empty_headers);

  ASSERT_EQ(exception.status, FIFTYONE_DEGREES_STATUS_INSUFFICIENT_CAPACITY);
  ASSERT_EQ(count, 1);

  fiftyoneDegreesFree(buffer);
  fiftyoneDegreesFree(headers);
  fiftyoneDegreesFree(empty_headers);
}

TEST_F(Transform, GHEVBase64) {
  const char *ghev =
      "eyJiaXRuZXNzIjoiNjQiLCJicmFuZHMiOlt7ImJyYW5kIjoiTm90L0EpQnJhbmQiLCJ2ZXJz"
      "aW9uIjoiOCJ9LHsiYnJhbmQiOiJDaHJvbWl1bSIsInZlcnNpb24iOiIxMjYifSx7ImJyYW5k"
      "IjoiR29vZ2xlIENocm9tZSIsInZlcnNpb24iOiIxMjYifV0sImZ1bGxWZXJzaW9uTGlzdCI6"
      "W3siYnJhbmQiOiJOb3QvQSlCcmFuZCIsInZlcnNpb24iOiI4LjAuMC4wIn0seyJicmFuZCI6"
      "IkNocm9taXVtIiwidmVyc2lvbiI6IjEyNi4wLjY0NzguMTI3In0seyJicmFuZCI6Ikdvb2ds"
      "ZSBDaHJvbWUiLCJ2ZXJzaW9uIjoiMTI2LjAuNjQ3OC4xMjcifV0sIm1vYmlsZSI6dHJ1ZSwi"
      "bW9kZWwiOiIiLCJwbGF0Zm9ybSI6Im1hY09TIiwid293NjQiOmZhbHNlfQ==";

  size_t bufferLength = strlen(ghev) * 2;
  char *buffer = (char *)fiftyoneDegreesMalloc(bufferLength);

  size_t count = fiftyoneDegreesTransformGhevFromBase64(
      ghev, buffer, bufferLength, &Transform::exception, results);

  // ---

  ASSERT_EQ(exception.status, FIFTYONE_DEGREES_STATUS_SUCCESS);

  ASSERT_EQ(count, 6);
  ASSERT_EQ(results->count, count);

  checkFieldAbsent("sec-ch-ua-arch");
  checkFieldValue("sec-ch-ua",
                  "\"Not/A)Brand\";v=\"8\", \"Chromium\";v=\"126\", \"Google "
                  "Chrome\";v=\"126\"");
  checkFieldValue("sec-ch-ua-bitness", "\"64\"");
  checkFieldValue(
      "sec-ch-ua-full-version-list",
      "\"Not/A)Brand\";v=\"8.0.0.0\", \"Chromium\";v=\"126.0.6478.127\", "
      "\"Google Chrome\";v=\"126.0.6478.127\"");
  checkFieldValue("sec-ch-ua-mobile", "?1");
  checkFieldValue("sec-ch-ua-model", "\"\"");
  checkFieldValue("sec-ch-ua-platform", "\"macOS\"");
  checkFieldAbsent("sec-ch-ua-platform-version");
  fiftyoneDegreesFree(buffer);
}

TEST_F(Transform, GHEVBase64NotEnoughMemory) {
  const char *ghev =
      "eyJicmFuZHMiOlt7ImJyYW5kIjoiTm90L0EpQnJhbmQiLCJ2ZXJzaW9uIjoiOCJ9LHsiYnJh"
      "bmQiOiJDaHJvbWl1bSIsInZlcnNpb24iOiIxMjYifSx7ImJyYW5kIjoiR29vZ2xlIENocm9t"
      "ZSIsInZlcnNpb24iOiIxMjYifV0sImZ1bGxWZXJzaW9uTGlzdCI6W3siYnJhbmQiOiJOb3Qv"
      "QSlCcmFuZCIsInZlcnNpb24iOiI4LjAuMC4wIn0seyJicmFuZCI6IkNocm9taXVtIiwidmVy"
      "c2lvbiI6IjEyNi4wLjY0NzguMTI3In0seyJicmFuZCI6Ikdvb2dsZSBDaHJvbWUiLCJ2ZXJz"
      "aW9uIjoiMTI2LjAuNjQ3OC4xMjcifV0sIm1vYmlsZSI6ZmFsc2UsIm1vZGVsIjoiIiwicGxh"
      "dGZvcm0iOiJtYWNPUyIsInBsYXRmb3JtVmVyc2lvbiI6IjE0LjUuMCJ9";

  size_t bufferLength = strlen(ghev);
  char *buffer = (char *)fiftyoneDegreesMalloc(bufferLength);

  fiftyoneDegreesTransformIterateGhevFromBase64(
      ghev, buffer, bufferLength, &Transform::exception, fillResultsCallback,
      Transform::results);

  // ---

  ASSERT_EQ(exception.status, FIFTYONE_DEGREES_STATUS_INSUFFICIENT_MEMORY);
  fiftyoneDegreesFree(buffer);
}

TEST_F(Transform, GHEVArraySua) {
  const char *sua =
      "{\"source\":2,\"platform\":{\"brand\":\"macOS\",\"version\":[\"14\","
      "\"5\",\"0\"]},\"browsers\":[{\"brand\":\"Not/"
      "A)Brand\",\"version\":[\"8\",\"0\",\"0\",\"0\"]},{\"brand\":"
      "\"Chromium\",\"version\":[\"126\",\"0\",\"6478\",\"127\"]},{\"brand\":"
      "\"Google "
      "Chrome\",\"version\":[\"126\",\"0\",\"6478\",\"127\"]}],\"mobile\":1,"
      "\"model\":\"MacBook\",\"architecture\":\"x86\"}";

  size_t bufferLength = strlen(sua);
  char *buffer = (char *)fiftyoneDegreesMalloc(bufferLength);

  size_t count = fiftyoneDegreesTransformSua(sua, buffer, bufferLength,
                                             &Transform::exception, results);

  // ---

  ASSERT_EQ(exception.status, FIFTYONE_DEGREES_STATUS_SUCCESS);

  ASSERT_EQ(count, 6);
  ASSERT_EQ(results->count, count);

  checkFieldValue("sec-ch-ua-arch", "\"x86\"");
  checkFieldValue("sec-ch-ua-model", "\"MacBook\"");
  checkFieldValue("sec-ch-ua-mobile", "?1");
  checkFieldValue("sec-ch-ua-platform", "\"macOS\"");
  checkFieldValue("sec-ch-ua-platform-version", "\"14.5.0\"");
  checkFieldValue(
      "sec-ch-ua-full-version-list",
      "\"Not/"
      "A)Brand\";v=\"8.0.0.0\", \"Chromium\";v=\"126.0.6478.127\", \"Google "
      "Chrome\";v=\"126.0.6478.127\"");

  checkFieldAbsent("sec-ch-ua-bitness");
  checkFieldAbsent("sec-ch-ua");
  fiftyoneDegreesFree(buffer);
}

TEST_F(Transform, GHEVPartial) {
  const char *ghev =
      "{\"brands\":[{\"brand\":\"Not/"
      "A)Brand\",\"version\":\"8\"},{\"brand\":\"Chromium\",\"version\":"
      "\"126\"},{\"brand\":\"Google "
      "Chrome\",\"version\":\"126\"}],\"fullVersionList\":[{\"brand\":\"Not/"
      "A)Brand\",\"version\":\"8.0.0.0\"},{\"brand\":\"Chromium\",\"version\":"
      "\"126.0.6478.61\"},{\"brand\":\"Google "
      "Chrome\",\"version\":\"126.0.6478.61\"}],\"mobile\":false,\"model\":"
      "\"\",\"platform\":null}";

  size_t bufferLength = strlen(ghev);
  char *buffer = (char *)fiftyoneDegreesMalloc(bufferLength);

  size_t count = fiftyoneDegreesTransformIterateGhevFromJson(
      ghev, buffer, bufferLength, &exception, fillResultsCallback,
      Transform::results);
  ASSERT_EQ(count, 4);
  ASSERT_EQ(exception.status, FIFTYONE_DEGREES_STATUS_SUCCESS);

  // we expect to see these headers detected:
  // low entropy: sec-ch-ua, sec-ch-ua-mobile
  // high entropy: sec-ch-ua-model, sec-ch-ua-full-version-list
  // we check that either empty value (model), null-value (platform)
  // or entire absence of key (platformVersion) result in no header in the
  // output the policy is - we don't output empty data - no value == no evidence
  // field

  ASSERT_EQ(results->count, count);

  checkFieldValue("sec-ch-ua",
                  "\"Not/A)Brand\";v=\"8\", \"Chromium\";v=\"126\", \"Google "
                  "Chrome\";v=\"126\"");
  checkFieldValue(
      "sec-ch-ua-full-version-list",
      "\"Not/A)Brand\";v=\"8.0.0.0\", \"Chromium\";v=\"126.0.6478.61\", "
      "\"Google Chrome\";v=\"126.0.6478.61\"");
  checkFieldValue("sec-ch-ua-mobile", "?0");
  checkFieldValue("sec-ch-ua-model", "\"\"");

  checkFieldAbsent("sec-ch-ua-platform");
  checkFieldAbsent("sec-ch-ua-platform-version");
  checkFieldAbsent("sec-ch-ua-arch");
  checkFieldAbsent("sec-ch-ua-bitness");
  fiftyoneDegreesFree(buffer);
}

TEST_F(Transform, GHEVIgnoreUnused) {
  const char *ghev =
      "{       \"architecture\":\"x86\",\"bitness\":\"64\",\"brands\":[{    "
      "\"brand\" :   "
      "\"Not/"
      "A)Brand\",\"version\":\"8\"},{\"brand\":\"Chromium\",\"version\":"
      "\"126\"},{\"brand\":\"Google "
      "Chrome\",\"version\":\"126\"}],\"fullVersionList\":[{\"brand\":\"Not/"
      "A)Brand\",\"version\":\"8.0.0.0\"},{\"brand\":\"Chromium\"   ,    "
      "\"version\" : \"126.0.6478.127\"},{\"brand\":\"Google "
      "Chrome\",\"version\":\"126.0.6478.127\"}],\"mobile\":false,\"model\":"
      "\"\",\"platform\":\"macOS\",\"platformVersion\":\"14.5.0\",\"wow64\":"
      "false}";

  size_t bufferLength = strlen(ghev);
  char *buffer = (char *)fiftyoneDegreesMalloc(bufferLength);

  size_t count = fiftyoneDegreesTransformIterateGhevFromJson(
      ghev, buffer, bufferLength, &exception, fillResultsCallback,
      Transform::results);

  ASSERT_EQ(count, 8);
  ASSERT_EQ(exception.status, FIFTYONE_DEGREES_STATUS_SUCCESS);

  // we expect to see these headers detected:
  // low entropy: sec-ch-ua, sec-ch-ua-mobile
  // high entropy: sec-ch-ua-model, sec-ch-ua-full-version-list
  // we check that either empty value (model), null-value (platform)
  // or entire absence of key (platformVersion) result in no header in the
  // output the policy is - we don't output empty data - no value == no evidence
  // field

  checkFieldValue("sec-ch-ua",
                  "\"Not/A)Brand\";v=\"8\", \"Chromium\";v=\"126\", \"Google "
                  "Chrome\";v=\"126\"");

  checkFieldValue(
      "sec-ch-ua-full-version-list",
      "\"Not/A)Brand\";v=\"8.0.0.0\", \"Chromium\";v=\"126.0.6478.127\", "
      "\"Google Chrome\";v=\"126.0.6478.127\"");

  checkFieldValue("sec-ch-ua-mobile", "?0");
  checkFieldValue("sec-ch-ua-platform", "\"macOS\"");
  checkFieldValue("sec-ch-ua-platform-version", "\"14.5.0\"");
  checkFieldValue("sec-ch-ua-model", "\"\"");
  checkFieldValue("sec-ch-ua-arch", "\"x86\"");
  checkFieldValue("sec-ch-ua-bitness", "\"64\"");
  fiftyoneDegreesFree(buffer);
}

TEST_F(Transform, GHEVCorruptInput) {
  const char *ghev =
      "{\"brands\":[{\"brand\":\"Not/"
      "A)Brand\",\"version\":\"8\"},{\"brand\":\"Chromium\",\"version\":"
      "\"126\"},{\"brand\":\"Google "
      "Chrome\",\"version\":\"126\"}],\"fullVersionList\":[{\"brand\":\"Not/"
      "A)Brand\",\"version\":\"8.0.0.0\"},{\"brand\":\"Chromium\",\"version\":"
      "\"126.0.6478.61\"},{\"brand\":\"Google "
      "Chrome\",\"version\":\"126.0.6478.61\"}],\"mobile\":false,\"model\":"
      "\"\",\"platform\"";

  size_t bufferLength = strlen(ghev);
  char *buffer = (char *)fiftyoneDegreesMalloc(bufferLength);

  fiftyoneDegreesTransformIterateGhevFromJson(ghev, buffer, bufferLength,
                                              &exception, fillResultsCallback,
                                              Transform::results);
  ASSERT_EQ(exception.status, FIFTYONE_DEGREES_STATUS_CORRUPT_DATA);
  fiftyoneDegreesFree(buffer);
}

TEST_F(Transform, GHEVBufferTooSmall) {
  const char *ghev =
      "{\"brands\":[{\"brand\":\"Not/"
      "A)Brand\",\"version\":\"8\"},{\"brand\":\"Chromium\",\"version\":"
      "\"126\"},{\"brand\":\"Google "
      "Chrome\",\"version\":\"126\"}],\"fullVersionList\":[{\"brand\":\"Not/"
      "A)Brand\",\"version\":\"8.0.0.0\"},{\"brand\":\"Chromium\",\"version\":"
      "\"126.0.6478.61\"},{\"brand\":\"Google "
      "Chrome\",\"version\":\"126.0.6478.61\"}],\"mobile\":false,\"model\":"
      "\"\",\"platform\":\"macOS\"}";

  size_t bufferLength = 20;
  char *buffer = (char *)fiftyoneDegreesMalloc(bufferLength);

  fiftyoneDegreesTransformIterateGhevFromJson(ghev, buffer, bufferLength,
                                              &exception, fillResultsCallback,
                                              Transform::results);
  ASSERT_EQ(exception.status, FIFTYONE_DEGREES_STATUS_INSUFFICIENT_MEMORY);
  fiftyoneDegreesFree(buffer);
}

TEST_F(Transform, GHEVEvidenceLowCapacity) {
  const char *ghev =
      "{\"brands\":[{\"brand\":\"Not/"
      "A)Brand\",\"version\":\"8\"},{\"brand\":\"Chromium\",\"version\":"
      "\"126\"},{\"brand\":\"Google "
      "Chrome\",\"version\":\"126\"}],\"fullVersionList\":[{\"brand\":\"Not/"
      "A)Brand\",\"version\":\"8.0.0.0\"},{\"brand\":\"Chromium\",\"version\":"
      "\"126.0.6478.61\"},{\"brand\":\"Google "
      "Chrome\",\"version\":\"126.0.6478.61\"}],\"mobile\":false,\"model\":"
      "\"\",\"platform\":\"macOS\"}";

  size_t bufferLength = 20;
  char *buffer = (char *)fiftyoneDegreesMalloc(bufferLength);

  fiftyoneDegreesTransformIterateGhevFromJson(ghev, buffer, bufferLength,
                                              &exception, fillResultsCallback,
                                              Transform::results);
  ASSERT_EQ(exception.status, FIFTYONE_DEGREES_STATUS_INSUFFICIENT_MEMORY);
  fiftyoneDegreesFree(buffer);
}

TEST_F(Transform, SUAHappyPath) {
  const char *sua =
      "{\"source\": 2,\"browsers\": [{\"brand\": \"Not A;Brand\",\"version\":"
      "[\"99\",\"0\",\"0\",\"0\"]},{\"brand\": \"Chromium\",\"version\": "
      "[\"99\",\"0\",\"4844\",\"88\"]},{\"brand\": \"Google "
      "Chrome\",\"version\": [\"99\",\"0\",\"4844\",\"88\"]}],\"platform\": "
      "{\"brand\": \"Android\",\"version\": [\"12\"]},\"mobile\": "
      "1,\"architecture\": \"arm\",\"bitness\": \"64\",\"model\": \"Pixel6\"}";

  size_t bufferLength = strlen(sua);
  char *buffer = (char *)fiftyoneDegreesMalloc(bufferLength);

  size_t count = fiftyoneDegreesTransformIterateSua(
      sua, buffer, bufferLength, &exception, fillResultsCallback,
      Transform::results);

  ASSERT_EQ(exception.status, FIFTYONE_DEGREES_STATUS_SUCCESS);

  ASSERT_EQ(count, 7);
  // we expect to see these headers detected:
  // low entropy: sec-ch-ua, sec-ch-ua-mobile, sec-ch-ua-platform
  // high entropy: sec-ch-ua-platform-version, sec-ch-ua-model, sec-ch-ua-arch,
  // sec-ch-ua-full-version-list

  ASSERT_EQ(results->count, count);

  // In device.sua representation there is no distinction between
  // sec-ch-ua and sec-ch-ua-full-version-list
  // checkFieldValue(
  //     "sec-ch-ua",
  //     "\"Not A;Brand\";v=\"99.0.0.0\",\"Chromium\";v=\"99.0.4844.88\","
  //     "\"Google Chrome\";v=\"99.0.4844.88\"\n");

  checkFieldValue(
      "sec-ch-ua-full-version-list",
      "\"Not A;Brand\";v=\"99.0.0.0\", \"Chromium\";v=\"99.0.4844.88\", "
      "\"Google Chrome\";v=\"99.0.4844.88\"");

  checkFieldValue("sec-ch-ua-platform", "\"Android\"");
  checkFieldValue("sec-ch-ua-platform-version", "\"12\"");
  checkFieldValue("sec-ch-ua-mobile", "?1");
  checkFieldValue("sec-ch-ua-arch", "\"arm\"");
  checkFieldValue("sec-ch-ua-bitness", "\"64\"");
  checkFieldValue("sec-ch-ua-model", "\"Pixel6\"");
  fiftyoneDegreesFree(buffer);
}

TEST_F(Transform, SUAPlatformExt) {
  const char *sua =
      "{\"source\": 2,\"browsers\": [{\"brand\": \"Not A;Brand\",\"version\": "
      "[\"99\",\"0\",\"0\",\"0\"]},{\"brand\": \"Chromium\",\"version\": "
      "[\"99\",\"0\",\"4844\",\"88\"]},{\"brand\": \"Google "
      "Chrome\",\"version\": [\"99\",\"0\",\"4844\",\"88\"]}],\"platform\": "
      "{\"brand\": \"Android\", \"version\": [\"13\"], "
      "\"ext\": { \"some_random_key\" : [ \"some\", \"random\", \"\\n\", "
      "\" \\\" values \" ], "
      "\"another_random_key\": null}},"
      "\"mobile\": 1,\"model\": \"\"}";

  size_t bufferLength = strlen(sua);
  char *buffer = (char *)fiftyoneDegreesMalloc(bufferLength);

  size_t count = fiftyoneDegreesTransformIterateSua(
      sua, buffer, bufferLength, &exception, fillResultsCallback,
      Transform::results);
  ASSERT_EQ(count, 5);
  // we expect to see these headers detected:
  // low entropy: sec-ch-ua, sec-ch-ua-mobile, sec-ch-ua-platform
  // high entropy: sec-ch-ua-platform-version, sec-ch-ua-model, sec-ch-ua-arch,
  // sec-ch-ua-full-version-list

  ASSERT_EQ(results->count, count);
  ASSERT_EQ(exception.status, FIFTYONE_DEGREES_STATUS_SUCCESS);

  checkFieldAbsent("sec-ch-ua");
  checkFieldValue(
      "sec-ch-ua-full-version-list",
      "\"Not A;Brand\";v=\"99.0.0.0\", \"Chromium\";v=\"99.0.4844.88\", "
      "\"Google Chrome\";v=\"99.0.4844.88\"");
  checkFieldValue("sec-ch-ua-platform", "\"Android\"");
  checkFieldValue("sec-ch-ua-platform-version", "\"13\"");
  checkFieldValue("sec-ch-ua-mobile", "?1");
  checkFieldAbsent("sec-ch-ua-arch");
  checkFieldAbsent("sec-ch-ua-bitness");
  checkFieldValue("sec-ch-ua-model", "\"\"");
  fiftyoneDegreesFree(buffer);
}

TEST_F(Transform, SUAPartial1) {
  const char *sua =
      "{\"source\": 2,\"browsers\": [{\"brand\": \"Not A;Brand\",\"version\": "
      "[\"99\",\"0\",\"0\",\"0\"]},{\"brand\": \"Chromium\",\"version\": "
      "[\"99\",\"0\",\"4844\",\"88\"]},{\"brand\": \"Google "
      "Chrome\",\"version\": [\"99\",\"0\",\"4844\",\"88\"]}],\"platform\": "
      "{\"brand\": \"Android\"},\"mobile\": 1,\"model\": \"\"}";

  size_t bufferLength = strlen(sua);
  char *buffer = (char *)fiftyoneDegreesMalloc(bufferLength);

  size_t count = fiftyoneDegreesTransformIterateSua(
      sua, buffer, bufferLength, &exception, fillResultsCallback,
      Transform::results);
  ASSERT_EQ(count, 4);
  // we expect to see these headers detected:
  // low entropy: sec-ch-ua, sec-ch-ua-mobile, sec-ch-ua-platform
  // high entropy: sec-ch-ua-platform-version, sec-ch-ua-model, sec-ch-ua-arch,
  // sec-ch-ua-full-version-list

  ASSERT_EQ(results->count, count);
  ASSERT_EQ(exception.status, FIFTYONE_DEGREES_STATUS_SUCCESS);

  checkFieldAbsent("sec-ch-ua");
  checkFieldValue(
      "sec-ch-ua-full-version-list",
      "\"Not A;Brand\";v=\"99.0.0.0\", \"Chromium\";v=\"99.0.4844.88\", "
      "\"Google Chrome\";v=\"99.0.4844.88\"");
  checkFieldValue("sec-ch-ua-platform", "\"Android\"");
  checkFieldAbsent("sec-ch-ua-platform-version");
  checkFieldValue("sec-ch-ua-mobile", "?1");
  checkFieldAbsent("sec-ch-ua-arch");
  checkFieldAbsent("sec-ch-ua-bitness");
  checkFieldValue("sec-ch-ua-model", "\"\"");
  fiftyoneDegreesFree(buffer);
}

TEST_F(Transform, SUAPartial2) {
  const char *sua =
      "{\"source\": 2,\"platform\": {\"brand\": \"Android\",\"version\": "
      "[\"12\"]},\"mobile\": 1,\"architecture\": \"arm\",\"bitness\": "
      "\"64\",\"model\": null}";

  size_t bufferLength = 2 * strlen(sua);
  char *buffer = (char *)fiftyoneDegreesMalloc(bufferLength);

  size_t count = fiftyoneDegreesTransformIterateSua(
      sua, buffer, bufferLength, &exception, fillResultsCallback,
      Transform::results);
  ASSERT_EQ(count, 5);

  ASSERT_EQ(exception.status, FIFTYONE_DEGREES_STATUS_SUCCESS);
  // we expect to see these headers detected:
  // low entropy: sec-ch-ua, sec-ch-ua-mobile, sec-ch-ua-platform
  // high entropy: sec-ch-ua-platform-version, sec-ch-ua-model, sec-ch-ua-arch,
  // sec-ch-ua-full-version-list

  ASSERT_EQ(results->count, count);

  checkFieldAbsent("sec-ch-ua");
  checkFieldAbsent("sec-ch-ua-full-version-list");
  checkFieldValue("sec-ch-ua-platform", "\"Android\"");
  checkFieldValue("sec-ch-ua-platform-version", "\"12\"");
  checkFieldValue("sec-ch-ua-mobile", "?1");
  checkFieldValue("sec-ch-ua-arch", "\"arm\"");
  checkFieldValue("sec-ch-ua-bitness", "\"64\"");
  checkFieldAbsent("sec-ch-ua-model");
  fiftyoneDegreesFree(buffer);
}

TEST_F(Transform, SUATolerableCorrupt) {
  const char *sua =
      "{\"source\": 2,,\"platform\": {\"brand\": \"Android\",\"version\": "
      "[\"12\"]},\"mobile\": 1,\"architecture\": \"arm\",\"bitness\": "
      "\"64\",\"model\": null}";

  size_t bufferLength = 2 * strlen(sua);
  char *buffer = (char *)fiftyoneDegreesMalloc(bufferLength);

  size_t count = fiftyoneDegreesTransformIterateSua(
      sua, buffer, bufferLength, &exception, fillResultsCallback,
      Transform::results);
  ASSERT_EQ(exception.status, FIFTYONE_DEGREES_STATUS_SUCCESS);

  ASSERT_EQ(count, 5);
  ASSERT_EQ(results->count, count);

  checkFieldAbsent("sec-ch-ua");
  checkFieldAbsent("sec-ch-ua-full-version-list");

  checkFieldValue("sec-ch-ua-arch", "\"arm\"");
  checkFieldValue("sec-ch-ua-bitness", "\"64\"");
  checkFieldValue("sec-ch-ua-mobile", "?1");
  checkFieldAbsent("sec-ch-ua-model");
  checkFieldValue("sec-ch-ua-platform", "\"Android\"");
  checkFieldValue("sec-ch-ua-platform-version", "\"12\"");
  fiftyoneDegreesFree(buffer);
}

TEST_F(Transform, SUACorrupt2) {
  const char *sua =
      "{\"source\": 2,\"platform\": {\"brand\": \"Android\",\"version\": "
      "[12\"]},\"mobile\": 1,\"architecture\": \"arm\",\"bitness\": "
      "\"64\",\"model\": null}";

  size_t bufferLength = 2 * strlen(sua);
  char *buffer = (char *)fiftyoneDegreesMalloc(bufferLength);

  fiftyoneDegreesTransformIterateSua(sua, buffer, bufferLength, &exception,
                                     fillResultsCallback, Transform::results);
  ASSERT_EQ(exception.status, FIFTYONE_DEGREES_STATUS_CORRUPT_DATA);
  fiftyoneDegreesFree(buffer);
}

TEST_F(Transform, SUACorrupt3) {
  const char *sua =
      "{\"source\": 2,\"platform\": {\"brand\": \"Android\",\"version\": "
      "\"12\"]},\"mobile\": 1,\"architecture\": \"arm\",\"bitness\": "
      "\"64\",\"model\": null}";

  size_t bufferLength = 2 * strlen(sua);
  char *buffer = (char *)fiftyoneDegreesMalloc(bufferLength);

  fiftyoneDegreesTransformIterateSua(sua, buffer, bufferLength, &exception,
                                     fillResultsCallback, Transform::results);
  ASSERT_EQ(exception.status, FIFTYONE_DEGREES_STATUS_CORRUPT_DATA);
  fiftyoneDegreesFree(buffer);
}

TEST_F(Transform, SUABufferTooSmall) {
  const char *sua =
      "{\"source\": 2,\"browsers\": [{\"brand\": \"Not A;Brand\",\"version\": "
      "[\"99\",\"0\",\"0\",\"0\"]},{\"brand\": \"Chromium\",\"version\": "
      "[\"99\",\"0\",\"4844\",\"88\"]},{\"brand\": \"Google "
      "Chrome\",\"version\": [\"99\",\"0\",\"4844\",\"88\"]}],\"platform\": "
      "{\"brand\": \"Android\",\"version\": [\"12\"]},\"mobile\": "
      "1,\"architecture\": \"arm\",\"bitness\": \"64\",\"model\": \"Pixel 6\"}";

  size_t bufferLength = 15;
  char *buffer = (char *)fiftyoneDegreesMalloc(bufferLength);

  fiftyoneDegreesTransformIterateSua(sua, buffer, bufferLength, &exception,
                                     fillResultsCallback, Transform::results);
  ASSERT_EQ(exception.status, FIFTYONE_DEGREES_STATUS_INSUFFICIENT_MEMORY);
  fiftyoneDegreesFree(buffer);
}

TEST_F(Transform, SUAEvidenceLowCapacity) {
  const char *sua =
      "{\"source\": 2,\"browsers\": [{\"brand\": \"Not A;Brand\",\"version\": "
      "[\"99\",\"0\",\"0\",\"0\"]},{\"brand\": \"Chromium\",\"version\": "
      "[\"99\",\"0\",\"4844\",\"88\"]},{\"brand\": \"Google "
      "Chrome\",\"version\": [\"99\",\"0\",\"4844\",\"88\"]}],\"platform\": "
      "{\"brand\": \"Android\",\"version\": [\"12\"]},\"mobile\": "
      "1,\"architecture\": \"arm\",\"bitness\": \"64\",\"model\": \"Pixel 6\"}";

  size_t bufferLength = 15;
  char *buffer = (char *)fiftyoneDegreesMalloc(bufferLength);

  fiftyoneDegreesTransformIterateSua(sua, buffer, bufferLength, &exception,
                                     fillResultsCallback, Transform::results);
  ASSERT_EQ(exception.status, FIFTYONE_DEGREES_STATUS_INSUFFICIENT_MEMORY);
  fiftyoneDegreesFree(buffer);
}

TEST_F(Transform, CPPWrapperGHEV) {
  FiftyoneDegrees::Common::Transform t;

  auto h = t.fromJsonGHEV(
      "{\"brands\":[{\"brand\":\"Not/"
      "A)Brand\",\"version\":\"8\"},{\"brand\":\"Chromium\",\"version\":"
      "\"126\"},{\"brand\":\"Google "
      "Chrome\",\"version\":\"126\"}],\"fullVersionList\":[{\"brand\":\"Not/"
      "A)Brand\",\"version\":\"8.0.0.0\"},{\"brand\":\"Chromium\","
      "\"version\":"
      "\"126.0.6478.61\"},{\"brand\":\"Google "
      "Chrome\",\"version\":\"126.0.6478.61\"}],\"mobile\":false,\"model\":"
      "\"\",\"platform\":null}");

  ASSERT_EQ(h.size(), 4);

  EXPECT_TRUE(h.find("sec-ch-ua") != h.end());
  EXPECT_TRUE(h.find("sec-ch-ua-full-version-list") != h.end());
  EXPECT_TRUE(h.find("sec-ch-ua-mobile") != h.end());
  EXPECT_TRUE(h.find("sec-ch-ua-model") != h.end());

  ASSERT_EQ(h["sec-ch-ua"],
            "\"Not/A)Brand\";v=\"8\", \"Chromium\";v=\"126\", \"Google "
            "Chrome\";v=\"126\"");

  ASSERT_EQ(h["sec-ch-ua-full-version-list"],
            "\"Not/A)Brand\";v=\"8.0.0.0\", \"Chromium\";v=\"126.0.6478.61\", "
            "\"Google Chrome\";v=\"126.0.6478.61\"");

  ASSERT_EQ(h["sec-ch-ua-mobile"], "?0");
  ASSERT_EQ(h["sec-ch-ua-model"], "\"\"");

  EXPECT_FALSE(h.find("sec-ch-ua-platform") != h.end());
  EXPECT_FALSE(h.find("sec-ch-ua-platform-version") != h.end());
  EXPECT_FALSE(h.find("sec-ch-ua-arch") != h.end());
  EXPECT_FALSE(h.find("sec-ch-ua-bitness") != h.end());
}

TEST_F(Transform, CPPWrapperBase64) {
  FiftyoneDegrees::Common::Transform t;

  auto h = t.fromBase64GHEV(
      "eyJicmFuZHMiOlt7ImJyYW5kIjoiTm90L0EpQnJhbmQiLCJ2ZXJzaW9uIjoiOCJ9LHsiYnJh"
      "bmQiOiJDaHJvbWl1bSIsInZlcnNpb24iOiIxMjYifSx7ImJyYW5kIjoiR29vZ2xlIENocm9t"
      "ZSIsInZlcnNpb24iOiIxMjYifV0sImZ1bGxWZXJzaW9uTGlzdCI6W3siYnJhbmQiOiJOb3Qv"
      "QSlCcmFuZCIsInZlcnNpb24iOiI4LjAuMC4wIn0seyJicmFuZCI6IkNocm9taXVtIiwidmVy"
      "c2lvbiI6IjEyNi4wLjY0NzguMTI3In0seyJicmFuZCI6Ikdvb2dsZSBDaHJvbWUiLCJ2ZXJz"
      "aW9uIjoiMTI2LjAuNjQ3OC4xMjcifV0sIm1vYmlsZSI6ZmFsc2UsIm1vZGVsIjoiIiwicGxh"
      "dGZvcm0iOiJtYWNPUyIsInBsYXRmb3JtVmVyc2lvbiI6IjE0LjUuMCJ9");

  ASSERT_EQ(h.size(), 6);

  EXPECT_TRUE(h.find("sec-ch-ua") != h.end());
  EXPECT_TRUE(h.find("sec-ch-ua-full-version-list") != h.end());
  EXPECT_TRUE(h.find("sec-ch-ua-mobile") != h.end());
  EXPECT_TRUE(h.find("sec-ch-ua-model") != h.end());
  EXPECT_TRUE(h.find("sec-ch-ua-platform") != h.end());
  EXPECT_TRUE(h.find("sec-ch-ua-platform-version") != h.end());

  ASSERT_EQ(h["sec-ch-ua"],
            "\"Not/A)Brand\";v=\"8\", \"Chromium\";v=\"126\", \"Google "
                  "Chrome\";v=\"126\"");

  ASSERT_EQ(h["sec-ch-ua-full-version-list"],
            "\"Not/A)Brand\";v=\"8.0.0.0\", \"Chromium\";v=\"126.0.6478.127\", "
      "\"Google Chrome\";v=\"126.0.6478.127\"");

  ASSERT_EQ(h["sec-ch-ua-mobile"], "?0");
  ASSERT_EQ(h["sec-ch-ua-model"], "\"\"");
  ASSERT_EQ(h["sec-ch-ua-platform"], "\"macOS\"");
  ASSERT_EQ(h["sec-ch-ua-platform-version"], "\"14.5.0\"");


  EXPECT_FALSE(h.find("sec-ch-ua-arch") != h.end());
  EXPECT_FALSE(h.find("sec-ch-ua-bitness") != h.end());
}

TEST_F(Transform, CPPWrapperBase64InsufficientMemory) {
  FiftyoneDegrees::Common::Transform t(128);

  auto h = t.fromBase64GHEV(
      "eyJicmFuZHMiOlt7ImJyYW5kIjoiTm90L0EpQnJhbmQiLCJ2ZXJzaW9uIjoiOCJ9LHsiYnJh"
      "bmQiOiJDaHJvbWl1bSIsInZlcnNpb24iOiIxMjYifSx7ImJyYW5kIjoiR29vZ2xlIENocm9t"
      "ZSIsInZlcnNpb24iOiIxMjYifV0sImZ1bGxWZXJzaW9uTGlzdCI6W3siYnJhbmQiOiJOb3Qv"
      "QSlCcmFuZCIsInZlcnNpb24iOiI4LjAuMC4wIn0seyJicmFuZCI6IkNocm9taXVtIiwidmVy"
      "c2lvbiI6IjEyNi4wLjY0NzguMTI3In0seyJicmFuZCI6Ikdvb2dsZSBDaHJvbWUiLCJ2ZXJz"
      "aW9uIjoiMTI2LjAuNjQ3OC4xMjcifV0sIm1vYmlsZSI6ZmFsc2UsIm1vZGVsIjoiIiwicGxh"
      "dGZvcm0iOiJtYWNPUyIsInBsYXRmb3JtVmVyc2lvbiI6IjE0LjUuMCJ9");

  ASSERT_EQ(h.size(), 6);

  EXPECT_TRUE(h.find("sec-ch-ua") != h.end());
  EXPECT_TRUE(h.find("sec-ch-ua-full-version-list") != h.end());
  EXPECT_TRUE(h.find("sec-ch-ua-mobile") != h.end());
  EXPECT_TRUE(h.find("sec-ch-ua-model") != h.end());
  EXPECT_TRUE(h.find("sec-ch-ua-platform") != h.end());
  EXPECT_TRUE(h.find("sec-ch-ua-platform-version") != h.end());

  ASSERT_EQ(h["sec-ch-ua"],
            "\"Not/A)Brand\";v=\"8\", \"Chromium\";v=\"126\", \"Google "
                  "Chrome\";v=\"126\"");

  ASSERT_EQ(h["sec-ch-ua-full-version-list"],
            "\"Not/A)Brand\";v=\"8.0.0.0\", \"Chromium\";v=\"126.0.6478.127\", "
      "\"Google Chrome\";v=\"126.0.6478.127\"");

  ASSERT_EQ(h["sec-ch-ua-mobile"], "?0");
  ASSERT_EQ(h["sec-ch-ua-model"], "\"\"");
  ASSERT_EQ(h["sec-ch-ua-platform"], "\"macOS\"");
  ASSERT_EQ(h["sec-ch-ua-platform-version"], "\"14.5.0\"");


  EXPECT_FALSE(h.find("sec-ch-ua-arch") != h.end());
  EXPECT_FALSE(h.find("sec-ch-ua-bitness") != h.end());
}

TEST_F(Transform, CPPWrapperSUA) {
  FiftyoneDegrees::Common::Transform t;

  auto h = t.fromSUA(
      "{\"source\":2,\"platform\":{\"brand\":\"macOS\",\"version\":[\"14\","
      "\"5\",\"0\"]},\"browsers\":[{\"brand\":\"Not/"
      "A)Brand\",\"version\":[\"8\",\"0\",\"0\",\"0\"]},{\"brand\":"
      "\"Chromium\",\"version\":[\"126\",\"0\",\"6478\",\"127\"]},{\"brand\":"
      "\"Google "
      "Chrome\",\"version\":[\"126\",\"0\",\"6478\",\"127\"]}],\"mobile\":1,"
      "\"model\":\"MacBook\",\"architecture\":\"x86\"}");

  ASSERT_EQ(h.size(), 6);

  EXPECT_TRUE(h.find("sec-ch-ua-arch") != h.end());
  EXPECT_TRUE(h.find("sec-ch-ua-model") != h.end());
  EXPECT_TRUE(h.find("sec-ch-ua-mobile") != h.end());
  EXPECT_TRUE(h.find("sec-ch-ua-platform") != h.end());
  EXPECT_TRUE(h.find("sec-ch-ua-platform-version") != h.end());
  EXPECT_TRUE(h.find("sec-ch-ua-full-version-list") != h.end());


  ASSERT_EQ(h["sec-ch-ua-arch"], "\"x86\"");
  ASSERT_EQ(h["sec-ch-ua-model"], "\"MacBook\"");
  ASSERT_EQ(h["sec-ch-ua-mobile"], "?1");
  ASSERT_EQ(h["sec-ch-ua-platform"], "\"macOS\"");
  ASSERT_EQ(h["sec-ch-ua-platform-version"], "\"14.5.0\"");
  ASSERT_EQ(h["sec-ch-ua-full-version-list"],
            "\"Not/"
      "A)Brand\";v=\"8.0.0.0\", \"Chromium\";v=\"126.0.6478.127\", \"Google "
      "Chrome\";v=\"126.0.6478.127\"");

  EXPECT_FALSE(h.find("sec-ch-ua") != h.end());
  EXPECT_FALSE(h.find("sec-ch-ua-bitness") != h.end());
}
