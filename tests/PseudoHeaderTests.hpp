#include "Base.hpp"
#include "../headers.h"
#include "../evidence.h"

// Used to construct header strings
#pragma pack(push, 1)
typedef struct test_tring_t {
	uint16_t size;
	char value[50];
} testString;
#pragma pack(pop)

typedef struct test_key_value_pair_t {
	char key[50];
	char value[50];
} testKeyValuePair;

typedef struct test_headers_t {
	size_t size;
	testString* headers;
} testHeaders;

class PseudoHeaderTests : public Base {
public:
	PseudoHeaderTests();
	~PseudoHeaderTests();
	void addEvidence(
		testKeyValuePair* evidenceList,
		int size,
		fiftyoneDegreesEvidencePrefix prefix);
	void checkResults(const char** expectedEvidence, int size);
	void removePseudoEvidence(size_t bufferSize);
protected:
	fiftyoneDegreesHeaders* acceptedHeaders;
	fiftyoneDegreesHeaders* acceptedHeadersNoPseudoHeaders;
	fiftyoneDegreesEvidenceKeyValuePairArray* evidence;
};