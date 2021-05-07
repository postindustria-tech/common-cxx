#include "Base.hpp"
#include "../headers.h"
#include "../evidence.h"
#include "StringCollection.hpp"

typedef struct test_key_value_pair_t {
	char key[50];
	char value[50];
} testKeyValuePair;

typedef struct test_expected_result_t {
	char result[50];
	fiftyoneDegreesEvidencePrefix prefix;
} testExpectedResult;

class PseudoHeaderTests : public Base {
public:
	void SetUp();
	void TearDown();
	void createHeaders(
		const char** headersList,
		int headersCount,
		bool expectUpperPrefixedHeaders);
	void addEvidence(
		testKeyValuePair* evidenceList,
		int size,
		fiftyoneDegreesEvidencePrefix prefix);
	void checkResults(const testExpectedResult *expectedEvidence, int size);
	void removePseudoEvidence(size_t bufferSize);
	int getNextPseudoIndex(const char* headers[], int size, int index);
protected:
	StringCollection* strings = NULL;
	fiftyoneDegreesHeaders* acceptedHeaders = NULL;
	fiftyoneDegreesEvidenceKeyValuePairArray* evidence = NULL;
};