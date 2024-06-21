//
//  TransformEvidenceTests.cpp
//  51DegreesCommon
//
//  Created by Eugene Dorfman on 6/17/24.
//

#include "EvidenceTests.hpp"
#include "../memory.h"
#include "../transform.h"

class TransformEvidence: public Evidence {
public:
    static bool found;
    static const char *expectedFieldName;
    static const char *expectedFieldValue;
    static bool onMatchIterateString(void *state, fiftyoneDegreesEvidenceKeyValuePair *pair)
    {
        if (strcmp((const char*)pair->field, expectedFieldName) == 0) {
            EXPECT_TRUE(strcmp((const char*)pair->originalValue, expectedFieldValue) == 0) <<
                L"Expected original value to be '" << expectedFieldValue << "' not '" <<
            (const char*)pair->originalValue << "'";
            found = true;
            return false;
        }
        return true;
    }
    
    void checkFieldValue(const char *field, const char *value);
    void checkFieldAbsent(const char *field);
};

bool TransformEvidence::found = false;
const char *TransformEvidence::expectedFieldName = NULL;
const char *TransformEvidence::expectedFieldValue = NULL;

void TransformEvidence::checkFieldValue(const char *field, const char *value) {
    found = false;
    expectedFieldName = field;
    expectedFieldValue = value;
    fiftyoneDegreesEvidenceIterate(
        evidence,
        FIFTYONE_DEGREES_EVIDENCE_HTTP_HEADER_STRING,
        NULL,
        onMatchIterateString);
    ASSERT_TRUE(found) << "Field " << field << " was not found should be " << value;
}

void TransformEvidence::checkFieldAbsent(const char *field) {
    found = false;
    expectedFieldName = field;
    expectedFieldValue = "";
    fiftyoneDegreesEvidenceIterate(
        evidence,
        FIFTYONE_DEGREES_EVIDENCE_HTTP_HEADER_STRING,
        NULL,
        onMatchIterateString);
    ASSERT_FALSE(found) << "Field " << field << " should be absent";
}

TEST_F(TransformEvidence, GHEVHappyPath) {
    CreateEvidence(8); // some extra room for conversion
    const char *ghev = "{\"architecture\":\"x86\",\"brands\":[{\"brand\":\"Not/A)Brand\",\"version\":\"8\"},{\"brand\":\"Chromium\",\"version\":\"126\"},{\"brand\":\"Google Chrome\",\"version\":\"126\"}],\"fullVersionList\":[{\"brand\":\"Not/A)Brand\",\"version\":\"8.0.0.0\"},{\"brand\":\"Chromium\",\"version\":\"126.0.6478.61\"},{\"brand\":\"Google Chrome\",\"version\":\"126.0.6478.61\"}],\"mobile\":false,\"model\":\"\",\"platform\":\"macOS\",\"platformVersion\":\"14.5.0\"}";
    
    size_t bufferLength = strlen(ghev);
    char *buffer = (char *) fiftyoneDegreesMalloc(bufferLength);
    
    fiftyoneDegreesHeadersFromGHEV(ghev, evidence, buffer, bufferLength);
    
    // we expect to see these headers detected:
    // low entropy: sec-ch-ua, sec-ch-ua-mobile, sec-ch-ua-platform
    // high entropy: sec-ch-ua-platform-version, sec-ch-ua-model, sec-ch-ua-arch, sec-ch-ua-full-version-list
    
    ASSERT_EQ(evidence->count, 7);

    checkFieldValue("sec-ch-ua", "\"Not/A)Brand\";v=\"8\", \"Chromium\";v=\"126\", \"Google Chrome\";v=\"126\"");
    checkFieldValue("sec-ch-ua-mobile", "?0");
    checkFieldValue("sec-ch-ua-platform", "\"macOS\"");
    checkFieldValue("sec-ch-ua-platform-version", "\"14.5.0\"");
    checkFieldAbsent("sec-ch-ua-model");
    checkFieldValue("sec-ch-ua-arch", "x86");
    checkFieldValue("sec-ch-ua-full-version-list", "\"Not/A)Brand\";v=\"8.0.0.0\", \"Chromium\";v=\"126.0.6478.61\", \"Google Chrome\";v=\"126.0.6478.61\"");
}

TEST_F(TransformEvidence, GHEVPartial) {
    CreateEvidence(8); // some extra room for conversion
    const char *ghev = "{\"brands\":[{\"brand\":\"Not/A)Brand\",\"version\":\"8\"},{\"brand\":\"Chromium\",\"version\":\"126\"},{\"brand\":\"Google Chrome\",\"version\":\"126\"}],\"fullVersionList\":[{\"brand\":\"Not/A)Brand\",\"version\":\"8.0.0.0\"},{\"brand\":\"Chromium\",\"version\":\"126.0.6478.61\"},{\"brand\":\"Google Chrome\",\"version\":\"126.0.6478.61\"}],\"mobile\":false,\"model\":\"\",\"platform\":null}";
    
    size_t bufferLength = strlen(ghev);
    char *buffer = (char *) fiftyoneDegreesMalloc(bufferLength);
    
    fiftyoneDegreesStatusCode status = fiftyoneDegreesHeadersFromGHEV(ghev, evidence, buffer, bufferLength);
    ASSERT_EQ(status, FIFTYONE_DEGREES_STATUS_SUCCESS);
    
    // we expect to see these headers detected:
    // low entropy: sec-ch-ua, sec-ch-ua-mobile
    // high entropy: sec-ch-ua-model, sec-ch-ua-full-version-list
    // we check that either empty value (model), null-value (platform)
    // or entire absence of key (platformVersion) result in no header in the output
    // the policy is - we don't output empty data - no value == no evidence field
    
    ASSERT_EQ(evidence->count, 3);

    checkFieldValue("sec-ch-ua", "\"Not/A)Brand\";v=\"8\", \"Chromium\";v=\"126\", \"Google Chrome\";v=\"126\"");
    checkFieldValue("sec-ch-ua-mobile", "?0");
    checkFieldAbsent("sec-ch-ua-platform");
    checkFieldAbsent("sec-ch-ua-model");
    checkFieldAbsent("sec-ch-ua-arch");
    checkFieldValue("sec-ch-ua-full-version-list", "\"Not/A)Brand\";v=\"8.0.0.0\", \"Chromium\";v=\"126.0.6478.61\", \"Google Chrome\";v=\"126.0.6478.61\"");
}

TEST_F(TransformEvidence, GHEVCorruptInput) {
    CreateEvidence(8); // some extra room for conversion
    const char *ghev = "{\"brands\":[{\"brand\":\"Not/A)Brand\",\"version\":\"8\"},{\"brand\":\"Chromium\",\"version\":\"126\"},{\"brand\":\"Google Chrome\",\"version\":\"126\"}],\"fullVersionList\":[{\"brand\":\"Not/A)Brand\",\"version\":\"8.0.0.0\"},{\"brand\":\"Chromium\",\"version\":\"126.0.6478.61\"},{\"brand\":\"Google Chrome\",\"version\":\"126.0.6478.61\"}],\"mobile\":false,\"model\":\"\",\"platform\"";
    
    size_t bufferLength = strlen(ghev);
    char *buffer = (char *) fiftyoneDegreesMalloc(bufferLength);
    
    fiftyoneDegreesStatusCode status = fiftyoneDegreesHeadersFromGHEV(ghev, evidence, buffer, bufferLength);
    ASSERT_EQ(status, FIFTYONE_DEGREES_STATUS_CORRUPT_DATA);
}

TEST_F(TransformEvidence, GHEVBufferTooSmall) {
    CreateEvidence(8); // some extra room for conversion
    const char *ghev = "{\"brands\":[{\"brand\":\"Not/A)Brand\",\"version\":\"8\"},{\"brand\":\"Chromium\",\"version\":\"126\"},{\"brand\":\"Google Chrome\",\"version\":\"126\"}],\"fullVersionList\":[{\"brand\":\"Not/A)Brand\",\"version\":\"8.0.0.0\"},{\"brand\":\"Chromium\",\"version\":\"126.0.6478.61\"},{\"brand\":\"Google Chrome\",\"version\":\"126.0.6478.61\"}],\"mobile\":false,\"model\":\"\",\"platform\":\"macOS\"}";
    
    size_t bufferLength = 20;
    char *buffer = (char *) fiftyoneDegreesMalloc(bufferLength);
    
    fiftyoneDegreesStatusCode status = fiftyoneDegreesHeadersFromGHEV(ghev, evidence, buffer, bufferLength);
    ASSERT_EQ(status, FIFTYONE_DEGREES_STATUS_INSUFFICIENT_MEMORY);
}

TEST_F(TransformEvidence, GHEVEvidenceLowCapacity) {
    CreateEvidence(3); // some extra room for conversion
    const char *ghev = "{\"brands\":[{\"brand\":\"Not/A)Brand\",\"version\":\"8\"},{\"brand\":\"Chromium\",\"version\":\"126\"},{\"brand\":\"Google Chrome\",\"version\":\"126\"}],\"fullVersionList\":[{\"brand\":\"Not/A)Brand\",\"version\":\"8.0.0.0\"},{\"brand\":\"Chromium\",\"version\":\"126.0.6478.61\"},{\"brand\":\"Google Chrome\",\"version\":\"126.0.6478.61\"}],\"mobile\":false,\"model\":\"\",\"platform\":\"macOS\"}";
    
    size_t bufferLength = 20;
    char *buffer = (char *) fiftyoneDegreesMalloc(bufferLength);
    
    fiftyoneDegreesStatusCode status = fiftyoneDegreesHeadersFromGHEV(ghev, evidence, buffer, bufferLength);
    ASSERT_EQ(status, FIFTYONE_DEGREES_STATUS_INSUFFICIENT_MEMORY);
}

TEST_F(TransformEvidence, SUAHappyPath) {
    CreateEvidence(8); // some extra room for conversion
    const char *sua = "{\"source\": 2,\"browsers\": [{\"brand\": \"Not A;Brand\",\"version\": [\"99\",\"0\",\"0\",\"0\"]},{\"brand\": \"Chromium\",\"version\": [\"99\",\"0\",\"4844\",\"88\"]},{\"brand\": \"Google Chrome\",\"version\": [\"99\",\"0\",\"4844\",\"88\"]}],\"platform\": {\"brand\": \"Android\",\"version\": [\"12\"]},\"mobile\": 1,\"architecture\": \"arm\",\"bitness\": \"64\",\"model\": \"Pixel 6\"}";
    
    size_t bufferLength = strlen(sua);
    char *buffer = (char *) fiftyoneDegreesMalloc(bufferLength);
    
    fiftyoneDegreesHeadersFromSUA(sua, evidence, buffer, bufferLength);
    
    // we expect to see these headers detected:
    // low entropy: sec-ch-ua, sec-ch-ua-mobile, sec-ch-ua-platform
    // high entropy: sec-ch-ua-platform-version, sec-ch-ua-model, sec-ch-ua-arch, sec-ch-ua-full-version-list
    
    ASSERT_EQ(evidence->count, 7);

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

TEST_F(TransformEvidence, SUAPartial1) {
    CreateEvidence(8); // some extra room for conversion
    const char *sua = "{\"source\": 2,\"browsers\": [{\"brand\": \"Not A;Brand\",\"version\": [\"99\",\"0\",\"0\",\"0\"]},{\"brand\": \"Chromium\",\"version\": [\"99\",\"0\",\"4844\",\"88\"]},{\"brand\": \"Google Chrome\",\"version\": [\"99\",\"0\",\"4844\",\"88\"]}],\"platform\": {\"brand\": \"Android\"]},\"mobile\": 1,\"model\": \"\"}";
    
    size_t bufferLength = strlen(sua);
    char *buffer = (char *) fiftyoneDegreesMalloc(bufferLength);
    
    fiftyoneDegreesHeadersFromSUA(sua, evidence, buffer, bufferLength);
    
    // we expect to see these headers detected:
    // low entropy: sec-ch-ua, sec-ch-ua-mobile, sec-ch-ua-platform
    // high entropy: sec-ch-ua-platform-version, sec-ch-ua-model, sec-ch-ua-arch, sec-ch-ua-full-version-list
    
    ASSERT_EQ(evidence->count, 4);

    checkFieldValue("sec-ch-ua", "\"Not A;Brand\";v=\"99.0.0.0\", \"Chromium\";v=\"99.0.4844.88\", \"Google Chrome\";v=\"99.0.4844.88\"");
    checkFieldValue("sec-ch-ua-full-version-list", "\"Not A;Brand\";v=\"99.0.0.0\", \"Chromium\";v=\"99.0.4844.88\", \"Google Chrome\";v=\"99.0.4844.88\"");
    checkFieldValue("sec-ch-ua-platform", "\"Android\"");
    checkFieldAbsent("sec-ch-ua-platform-version");
    checkFieldValue("sec-ch-ua-mobile", "?1");
    checkFieldAbsent("sec-ch-ua-arch");
    checkFieldAbsent("sec-ch-ua-bitness");
    checkFieldAbsent("sec-ch-ua-model");
}

TEST_F(TransformEvidence, SUAPartial2) {
    CreateEvidence(8); // some extra room for conversion
    const char *sua = "{\"source\": 2,\"platform\": {\"brand\": \"Android\",\"version\": [\"12\"]},\"mobile\": 1,\"architecture\": \"arm\",\"bitness\": \"64\",\"model\": null}";
    
    size_t bufferLength = strlen(sua);
    char *buffer = (char *) fiftyoneDegreesMalloc(bufferLength);
    
    fiftyoneDegreesHeadersFromSUA(sua, evidence, buffer, bufferLength);
    
    // we expect to see these headers detected:
    // low entropy: sec-ch-ua, sec-ch-ua-mobile, sec-ch-ua-platform
    // high entropy: sec-ch-ua-platform-version, sec-ch-ua-model, sec-ch-ua-arch, sec-ch-ua-full-version-list
    
    ASSERT_EQ(evidence->count, 2);

    checkFieldAbsent("sec-ch-ua");
    checkFieldAbsent("sec-ch-ua-full-version-list");
    checkFieldValue("sec-ch-ua-platform", "\"Android\"");
    checkFieldAbsent("sec-ch-ua-platform-version");
    checkFieldValue("sec-ch-ua-mobile", "?1");
    checkFieldAbsent("sec-ch-ua-arch");
    checkFieldAbsent("sec-ch-ua-bitness");
    checkFieldAbsent("sec-ch-ua-model");
}

TEST_F(TransformEvidence, SUACorrupt1) {
    CreateEvidence(8); // some extra room for conversion
    const char *sua = "{\"source\": 2,,\"platform\": {\"brand\": \"Android\",\"version\": [\"12\"]},\"mobile\": 1,\"architecture\": \"arm\",\"bitness\": \"64\",\"model\": null}";
    
    size_t bufferLength = strlen(sua);
    char *buffer = (char *) fiftyoneDegreesMalloc(bufferLength);
    
    fiftyoneDegreesStatusCode status = fiftyoneDegreesHeadersFromSUA(sua, evidence, buffer, bufferLength);
    ASSERT_EQ(status, FIFTYONE_DEGREES_STATUS_CORRUPT_DATA);
}

TEST_F(TransformEvidence, SUACorrupt2) {
    CreateEvidence(8); // some extra room for conversion
    const char *sua = "{\"source\": 2,\"platform\": {\"brand\": \"Android\",\"version\": [12\"]},\"mobile\": 1,\"architecture\": \"arm\",\"bitness\": \"64\",\"model\": null}";
    
    size_t bufferLength = strlen(sua);
    char *buffer = (char *) fiftyoneDegreesMalloc(bufferLength);
    
    fiftyoneDegreesStatusCode status = fiftyoneDegreesHeadersFromSUA(sua, evidence, buffer, bufferLength);
    ASSERT_EQ(status, FIFTYONE_DEGREES_STATUS_CORRUPT_DATA);
}

TEST_F(TransformEvidence, SUACorrupt3) {
    CreateEvidence(8); // some extra room for conversion
    const char *sua = "{\"source\": 2,\"platform\": {\"brand\": \"Android\",\"version\": \"12\"]},\"mobile\": 1,\"architecture\": \"arm\",\"bitness\": \"64\",\"model\": null}";
    
    size_t bufferLength = strlen(sua);
    char *buffer = (char *) fiftyoneDegreesMalloc(bufferLength);
    
    fiftyoneDegreesStatusCode status = fiftyoneDegreesHeadersFromSUA(sua, evidence, buffer, bufferLength);
    ASSERT_EQ(status, FIFTYONE_DEGREES_STATUS_CORRUPT_DATA);
}

TEST_F(TransformEvidence, SUABufferTooSmall) {
    CreateEvidence(8); // some extra room for conversion
    const char *sua = "{\"source\": 2,\"browsers\": [{\"brand\": \"Not A;Brand\",\"version\": [\"99\",\"0\",\"0\",\"0\"]},{\"brand\": \"Chromium\",\"version\": [\"99\",\"0\",\"4844\",\"88\"]},{\"brand\": \"Google Chrome\",\"version\": [\"99\",\"0\",\"4844\",\"88\"]}],\"platform\": {\"brand\": \"Android\",\"version\": [\"12\"]},\"mobile\": 1,\"architecture\": \"arm\",\"bitness\": \"64\",\"model\": \"Pixel 6\"}";

    size_t bufferLength = 15;
    char *buffer = (char *) fiftyoneDegreesMalloc(bufferLength);
    
    fiftyoneDegreesStatusCode status = fiftyoneDegreesHeadersFromSUA(sua, evidence, buffer, bufferLength);
    ASSERT_EQ(status, FIFTYONE_DEGREES_STATUS_INSUFFICIENT_MEMORY);
}

TEST_F(TransformEvidence, SUAEvidenceLowCapacity) {
    CreateEvidence(2); // some extra room for conversion
    const char *sua = "{\"source\": 2,\"browsers\": [{\"brand\": \"Not A;Brand\",\"version\": [\"99\",\"0\",\"0\",\"0\"]},{\"brand\": \"Chromium\",\"version\": [\"99\",\"0\",\"4844\",\"88\"]},{\"brand\": \"Google Chrome\",\"version\": [\"99\",\"0\",\"4844\",\"88\"]}],\"platform\": {\"brand\": \"Android\",\"version\": [\"12\"]},\"mobile\": 1,\"architecture\": \"arm\",\"bitness\": \"64\",\"model\": \"Pixel 6\"}";

    size_t bufferLength = 15;
    char *buffer = (char *) fiftyoneDegreesMalloc(bufferLength);
    
    fiftyoneDegreesStatusCode status = fiftyoneDegreesHeadersFromSUA(sua, evidence, buffer, bufferLength);
    ASSERT_EQ(status, FIFTYONE_DEGREES_STATUS_INSUFFICIENT_MEMORY);  // 
}
