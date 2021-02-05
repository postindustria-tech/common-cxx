#include "Base.hpp"
#include "../headers.h"
#include "../evidence.h"
#include "StringCollection.hpp"

typedef struct test_key_value_pair_t {
	char key[50];
	char value[50];
} testKeyValuePair;

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
	void checkResults(const char** expectedEvidence, int size);
	void removePseudoEvidence(size_t bufferSize);
protected:
	StringCollection* strings = NULL;
	fiftyoneDegreesHeaders* acceptedHeaders = NULL;
	fiftyoneDegreesEvidenceKeyValuePairArray* evidence = NULL;
};