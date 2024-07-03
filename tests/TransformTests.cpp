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

#include "EvidenceTests.hpp"
#include "../memory.h"
#include "../transform.h"

class Transform: public Base {
public:
    virtual void SetUp();
    
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
    for (int i = 0; i < results->count; ++i) {
        fiftyoneDegreesKeyValuePair *pair = &results->items[i];
        if (strcmp((const char*)pair->key, expectedFieldName) == 0) {
                EXPECT_TRUE(strcmp((const char*)pair->value, expectedFieldValue) == 0) <<
                    L"Expected value to be '" << expectedFieldValue << "' not '" <<
            (const char*)pair->value << "'";
            found = true;
            break;
        }
    }
    ASSERT_TRUE(found) << "Field " << field << " was not found should be " << value;
}

void Transform::checkFieldAbsent(const char *field) {
    found = false;
    expectedFieldName = field;
    expectedFieldValue = "";
    for (int i = 0; i < results->count; ++i) {
        fiftyoneDegreesKeyValuePair *pair = &results->items[i];
        if (strcmp((const char*)pair->key, expectedFieldName) == 0) {
            found = true;
            break;
        }
    }
    ASSERT_FALSE(found) << "Field " << field << " should be absent";
}

bool fillResultsCallback(fiftyoneDegreesKeyValuePair pair) {
    fiftyoneDegreesKeyValuePairArray *results = Transform::results;
    if (results->count < results->capacity) {
        results->items[results->count++] = pair;
        return true;
    }
    return false;
}

void Transform::SetUp() {
    FIFTYONE_DEGREES_ARRAY_CREATE(fiftyoneDegreesKeyValuePair, results, 8)
}

TEST_F(Transform, GHEVHappyPath) {
    const char *ghev = "{\"architecture\":\"x86\",\"brands\":[{\"brand\":\"Not/A)Brand\",\"version\":\"8\"},{\"brand\":\"Chromium\",\"version\":\"126\"},{\"brand\":\"Google Chrome\",\"version\":\"126\"}],\"fullVersionList\":[{\"brand\":\"Not/A)Brand\",\"version\":\"8.0.0.0\"},{\"brand\":\"Chromium\",\"version\":\"126.0.6478.61\"},{\"brand\":\"Google Chrome\",\"version\":\"126.0.6478.61\"}],\"mobile\":false,\"model\":\"\",\"platform\":\"macOS\",\"platformVersion\":\"14.5.0\"}";
    
    size_t bufferLength = strlen(ghev);
    char *buffer = (char *) fiftyoneDegreesMalloc(bufferLength);
    
    size_t count = fiftyoneDegreesTransformIterateGhevFromJson(ghev, buffer, bufferLength, &Transform::exception, fillResultsCallback);
    
    // we expect to see these headers detected:
    // low entropy: sec-ch-ua, sec-ch-ua-mobile, sec-ch-ua-platform
    // high entropy: sec-ch-ua-platform-version, sec-ch-ua-model, sec-ch-ua-arch, sec-ch-ua-full-version-list
    
    ASSERT_EQ(results->count, count);

    checkFieldValue("sec-ch-ua", "\"Not/A)Brand\";v=\"8\", \"Chromium\";v=\"126\", \"Google Chrome\";v=\"126\"");
    checkFieldValue("sec-ch-ua-mobile", "?0");
    checkFieldValue("sec-ch-ua-platform", "\"macOS\"");
    checkFieldValue("sec-ch-ua-platform-version", "\"14.5.0\"");
    checkFieldAbsent("sec-ch-ua-model");
    checkFieldValue("sec-ch-ua-arch", "x86");
    checkFieldValue("sec-ch-ua-full-version-list", "\"Not/A)Brand\";v=\"8.0.0.0\", \"Chromium\";v=\"126.0.6478.61\", \"Google Chrome\";v=\"126.0.6478.61\"");
}

TEST_F(Transform, GHEVPartial) {
    const char *ghev = "{\"brands\":[{\"brand\":\"Not/A)Brand\",\"version\":\"8\"},{\"brand\":\"Chromium\",\"version\":\"126\"},{\"brand\":\"Google Chrome\",\"version\":\"126\"}],\"fullVersionList\":[{\"brand\":\"Not/A)Brand\",\"version\":\"8.0.0.0\"},{\"brand\":\"Chromium\",\"version\":\"126.0.6478.61\"},{\"brand\":\"Google Chrome\",\"version\":\"126.0.6478.61\"}],\"mobile\":false,\"model\":\"\",\"platform\":null}";
    
    size_t bufferLength = strlen(ghev);
    char *buffer = (char *) fiftyoneDegreesMalloc(bufferLength);
    
    size_t count = fiftyoneDegreesTransformIterateGhevFromJson(ghev, buffer, bufferLength, &exception, fillResultsCallback);
    ASSERT_EQ(count, 3);
    ASSERT_EQ(exception.status, FIFTYONE_DEGREES_STATUS_SUCCESS);
    
    // we expect to see these headers detected:
    // low entropy: sec-ch-ua, sec-ch-ua-mobile
    // high entropy: sec-ch-ua-model, sec-ch-ua-full-version-list
    // we check that either empty value (model), null-value (platform)
    // or entire absence of key (platformVersion) result in no header in the output
    // the policy is - we don't output empty data - no value == no evidence field
    
    ASSERT_EQ(results->count, count);

    checkFieldValue("sec-ch-ua", "\"Not/A)Brand\";v=\"8\", \"Chromium\";v=\"126\", \"Google Chrome\";v=\"126\"");
    checkFieldValue("sec-ch-ua-mobile", "?0");
    checkFieldAbsent("sec-ch-ua-platform");
    checkFieldAbsent("sec-ch-ua-model");
    checkFieldAbsent("sec-ch-ua-arch");
    checkFieldValue("sec-ch-ua-full-version-list", "\"Not/A)Brand\";v=\"8.0.0.0\", \"Chromium\";v=\"126.0.6478.61\", \"Google Chrome\";v=\"126.0.6478.61\"");
}

TEST_F(Transform, GHEVCorruptInput) {
    const char *ghev = "{\"brands\":[{\"brand\":\"Not/A)Brand\",\"version\":\"8\"},{\"brand\":\"Chromium\",\"version\":\"126\"},{\"brand\":\"Google Chrome\",\"version\":\"126\"}],\"fullVersionList\":[{\"brand\":\"Not/A)Brand\",\"version\":\"8.0.0.0\"},{\"brand\":\"Chromium\",\"version\":\"126.0.6478.61\"},{\"brand\":\"Google Chrome\",\"version\":\"126.0.6478.61\"}],\"mobile\":false,\"model\":\"\",\"platform\"";
    
    size_t bufferLength = strlen(ghev);
    char *buffer = (char *) fiftyoneDegreesMalloc(bufferLength);
    
    size_t count = fiftyoneDegreesTransformIterateGhevFromJson(ghev, buffer, bufferLength, &exception, fillResultsCallback);
    ASSERT_EQ(exception.status, FIFTYONE_DEGREES_STATUS_CORRUPT_DATA);
}

TEST_F(Transform, GHEVBufferTooSmall) {
    const char *ghev = "{\"brands\":[{\"brand\":\"Not/A)Brand\",\"version\":\"8\"},{\"brand\":\"Chromium\",\"version\":\"126\"},{\"brand\":\"Google Chrome\",\"version\":\"126\"}],\"fullVersionList\":[{\"brand\":\"Not/A)Brand\",\"version\":\"8.0.0.0\"},{\"brand\":\"Chromium\",\"version\":\"126.0.6478.61\"},{\"brand\":\"Google Chrome\",\"version\":\"126.0.6478.61\"}],\"mobile\":false,\"model\":\"\",\"platform\":\"macOS\"}";
    
    size_t bufferLength = 20;
    char *buffer = (char *) fiftyoneDegreesMalloc(bufferLength);
    
    size_t count = fiftyoneDegreesTransformIterateGhevFromJson(ghev, buffer, bufferLength, &exception, fillResultsCallback);
    ASSERT_EQ(exception.status, FIFTYONE_DEGREES_STATUS_INSUFFICIENT_MEMORY);
}

TEST_F(Transform, GHEVEvidenceLowCapacity) {
    const char *ghev = "{\"brands\":[{\"brand\":\"Not/A)Brand\",\"version\":\"8\"},{\"brand\":\"Chromium\",\"version\":\"126\"},{\"brand\":\"Google Chrome\",\"version\":\"126\"}],\"fullVersionList\":[{\"brand\":\"Not/A)Brand\",\"version\":\"8.0.0.0\"},{\"brand\":\"Chromium\",\"version\":\"126.0.6478.61\"},{\"brand\":\"Google Chrome\",\"version\":\"126.0.6478.61\"}],\"mobile\":false,\"model\":\"\",\"platform\":\"macOS\"}";
    
    size_t bufferLength = 20;
    char *buffer = (char *) fiftyoneDegreesMalloc(bufferLength);
    
    size_t count = fiftyoneDegreesTransformIterateGhevFromJson(ghev, buffer, bufferLength, &exception, fillResultsCallback);
    ASSERT_EQ(exception.status, FIFTYONE_DEGREES_STATUS_INSUFFICIENT_MEMORY);
}

TEST_F(Transform, SUAHappyPath) {
    const char *sua = "{\"source\": 2,\"browsers\": [{\"brand\": \"Not A;Brand\",\"version\": [\"99\",\"0\",\"0\",\"0\"]},{\"brand\": \"Chromium\",\"version\": [\"99\",\"0\",\"4844\",\"88\"]},{\"brand\": \"Google Chrome\",\"version\": [\"99\",\"0\",\"4844\",\"88\"]}],\"platform\": {\"brand\": \"Android\",\"version\": [\"12\"]},\"mobile\": 1,\"architecture\": \"arm\",\"bitness\": \"64\",\"model\": \"Pixel 6\"}";
    
    size_t bufferLength = strlen(sua);
    char *buffer = (char *) fiftyoneDegreesMalloc(bufferLength);
    
    size_t count = fiftyoneDegreesTransformIterateSua(sua, buffer, bufferLength, &exception, fillResultsCallback);
    
    ASSERT_EQ(count, 7);
    // we expect to see these headers detected:
    // low entropy: sec-ch-ua, sec-ch-ua-mobile, sec-ch-ua-platform
    // high entropy: sec-ch-ua-platform-version, sec-ch-ua-model, sec-ch-ua-arch, sec-ch-ua-full-version-list
    
    ASSERT_EQ(results->count, count);

    // In device.sua representation there is no distinction between
    // sec-ch-ua and sec-ch-ua-full-version-list
    checkFieldValue("sec-ch-ua", "\"Not A;Brand\";v=\"99.0.0.0\", \"Chromium\";v=\"99.0.4844.88\", \"Google Chrome\";v=\"99.0.4844.88\"");
    checkFieldValue("sec-ch-ua-full-version-list", "\"Not A;Brand\";v=\"99.0.0.0\", \"Chromium\";v=\"99.0.4844.88\", \"Google Chrome\";v=\"99.0.4844.88\"");
    checkFieldValue("sec-ch-ua-platform", "\"Android\"");
    checkFieldValue("sec-ch-ua-platform-version", "\"12\"");
    checkFieldValue("sec-ch-ua-mobile", "?1");
    checkFieldValue("sec-ch-ua-arch", "\"arm\"");
    checkFieldAbsent("sec-ch-ua-bitness");  //we ignore bitness
    checkFieldValue("sec-ch-ua-model", "\"Pixel 6\"");
}

TEST_F(Transform, SUAPartial1) {
    const char *sua = "{\"source\": 2,\"browsers\": [{\"brand\": \"Not A;Brand\",\"version\": [\"99\",\"0\",\"0\",\"0\"]},{\"brand\": \"Chromium\",\"version\": [\"99\",\"0\",\"4844\",\"88\"]},{\"brand\": \"Google Chrome\",\"version\": [\"99\",\"0\",\"4844\",\"88\"]}],\"platform\": {\"brand\": \"Android\"]},\"mobile\": 1,\"model\": \"\"}";
    
    size_t bufferLength = strlen(sua);
    char *buffer = (char *) fiftyoneDegreesMalloc(bufferLength);
    
    size_t count = fiftyoneDegreesTransformIterateSua(sua, buffer, bufferLength, &exception, fillResultsCallback);
    ASSERT_EQ(count, 4);
    // we expect to see these headers detected:
    // low entropy: sec-ch-ua, sec-ch-ua-mobile, sec-ch-ua-platform
    // high entropy: sec-ch-ua-platform-version, sec-ch-ua-model, sec-ch-ua-arch, sec-ch-ua-full-version-list
    
    ASSERT_EQ(results->count, count);

    checkFieldValue("sec-ch-ua", "\"Not A;Brand\";v=\"99.0.0.0\", \"Chromium\";v=\"99.0.4844.88\", \"Google Chrome\";v=\"99.0.4844.88\"");
    checkFieldValue("sec-ch-ua-full-version-list", "\"Not A;Brand\";v=\"99.0.0.0\", \"Chromium\";v=\"99.0.4844.88\", \"Google Chrome\";v=\"99.0.4844.88\"");
    checkFieldValue("sec-ch-ua-platform", "\"Android\"");
    checkFieldAbsent("sec-ch-ua-platform-version");
    checkFieldValue("sec-ch-ua-mobile", "?1");
    checkFieldAbsent("sec-ch-ua-arch");
    checkFieldAbsent("sec-ch-ua-bitness");
    checkFieldAbsent("sec-ch-ua-model");
}

TEST_F(Transform, SUAPartial2) {
    const char *sua = "{\"source\": 2,\"platform\": {\"brand\": \"Android\",\"version\": [\"12\"]},\"mobile\": 1,\"architecture\": \"arm\",\"bitness\": \"64\",\"model\": null}";
    
    size_t bufferLength = strlen(sua);
    char *buffer = (char *) fiftyoneDegreesMalloc(bufferLength);
    
    size_t count = fiftyoneDegreesTransformIterateSua(sua, buffer, bufferLength, &exception, fillResultsCallback);
    ASSERT_EQ(count, 2);
    // we expect to see these headers detected:
    // low entropy: sec-ch-ua, sec-ch-ua-mobile, sec-ch-ua-platform
    // high entropy: sec-ch-ua-platform-version, sec-ch-ua-model, sec-ch-ua-arch, sec-ch-ua-full-version-list
    
    ASSERT_EQ(results->count, count);

    checkFieldAbsent("sec-ch-ua");
    checkFieldAbsent("sec-ch-ua-full-version-list");
    checkFieldValue("sec-ch-ua-platform", "\"Android\"");
    checkFieldAbsent("sec-ch-ua-platform-version");
    checkFieldValue("sec-ch-ua-mobile", "?1");
    checkFieldAbsent("sec-ch-ua-arch");
    checkFieldAbsent("sec-ch-ua-bitness");
    checkFieldAbsent("sec-ch-ua-model");
}

TEST_F(Transform, SUACorrupt1) {
    const char *sua = "{\"source\": 2,,\"platform\": {\"brand\": \"Android\",\"version\": [\"12\"]},\"mobile\": 1,\"architecture\": \"arm\",\"bitness\": \"64\",\"model\": null}";
    
    size_t bufferLength = strlen(sua);
    char *buffer = (char *) fiftyoneDegreesMalloc(bufferLength);
    
    size_t count = fiftyoneDegreesTransformIterateSua(sua, buffer, bufferLength, &exception, fillResultsCallback);
    ASSERT_EQ(exception.status, FIFTYONE_DEGREES_STATUS_CORRUPT_DATA);
}

TEST_F(Transform, SUACorrupt2) {
    const char *sua = "{\"source\": 2,\"platform\": {\"brand\": \"Android\",\"version\": [12\"]},\"mobile\": 1,\"architecture\": \"arm\",\"bitness\": \"64\",\"model\": null}";
    
    size_t bufferLength = strlen(sua);
    char *buffer = (char *) fiftyoneDegreesMalloc(bufferLength);
    
    size_t count = fiftyoneDegreesTransformIterateSua(sua, buffer, bufferLength, &exception, fillResultsCallback);
    ASSERT_EQ(exception.status, FIFTYONE_DEGREES_STATUS_CORRUPT_DATA);
}

TEST_F(Transform, SUACorrupt3) {
    const char *sua = "{\"source\": 2,\"platform\": {\"brand\": \"Android\",\"version\": \"12\"]},\"mobile\": 1,\"architecture\": \"arm\",\"bitness\": \"64\",\"model\": null}";
    
    size_t bufferLength = strlen(sua);
    char *buffer = (char *) fiftyoneDegreesMalloc(bufferLength);
    
    size_t count = fiftyoneDegreesTransformIterateSua(sua, buffer, bufferLength, &exception, fillResultsCallback);
    ASSERT_EQ(exception.status, FIFTYONE_DEGREES_STATUS_CORRUPT_DATA);
}

TEST_F(Transform, SUABufferTooSmall) {
    const char *sua = "{\"source\": 2,\"browsers\": [{\"brand\": \"Not A;Brand\",\"version\": [\"99\",\"0\",\"0\",\"0\"]},{\"brand\": \"Chromium\",\"version\": [\"99\",\"0\",\"4844\",\"88\"]},{\"brand\": \"Google Chrome\",\"version\": [\"99\",\"0\",\"4844\",\"88\"]}],\"platform\": {\"brand\": \"Android\",\"version\": [\"12\"]},\"mobile\": 1,\"architecture\": \"arm\",\"bitness\": \"64\",\"model\": \"Pixel 6\"}";

    size_t bufferLength = 15;
    char *buffer = (char *) fiftyoneDegreesMalloc(bufferLength);
    
    size_t count = fiftyoneDegreesTransformIterateSua(sua, buffer, bufferLength, &exception, fillResultsCallback);
    ASSERT_EQ(exception.status, FIFTYONE_DEGREES_STATUS_INSUFFICIENT_MEMORY);
}

TEST_F(Transform, SUAEvidenceLowCapacity) {
    const char *sua = "{\"source\": 2,\"browsers\": [{\"brand\": \"Not A;Brand\",\"version\": [\"99\",\"0\",\"0\",\"0\"]},{\"brand\": \"Chromium\",\"version\": [\"99\",\"0\",\"4844\",\"88\"]},{\"brand\": \"Google Chrome\",\"version\": [\"99\",\"0\",\"4844\",\"88\"]}],\"platform\": {\"brand\": \"Android\",\"version\": [\"12\"]},\"mobile\": 1,\"architecture\": \"arm\",\"bitness\": \"64\",\"model\": \"Pixel 6\"}";

    size_t bufferLength = 15;
    char *buffer = (char *) fiftyoneDegreesMalloc(bufferLength);
    
    size_t count = fiftyoneDegreesTransformIterateSua(sua, buffer, bufferLength, &exception, fillResultsCallback);
    ASSERT_EQ(exception.status, FIFTYONE_DEGREES_STATUS_INSUFFICIENT_MEMORY);
}
