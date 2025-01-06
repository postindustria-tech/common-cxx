/* *********************************************************************
 * This Original Work is copyright of 51 Degrees Mobile Experts Limited.
 * Copyright 2025 51 Degrees Mobile Experts Limited, Davidson House,
 * Forbury Square, Reading, Berkshire, United Kingdom RG1 3EU.
 *
 * This Original Work is licensed under the European Union Public Licence (EUPL) 
 * v.1.2 and is subject to its terms as set out below.
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

#include <cstring>
#include <string>
#include <stdexcept>
#include "memory.h"
#include "IpAddress.hpp"

using namespace std;
using namespace FiftyoneDegrees::IpIntelligence;

IpAddress::IpAddress() {
    memset(this->ipAddress, 0, FIFTYONE_DEGREES_IPV6_LENGTH);
    this->type = FIFTYONE_DEGREES_EVIDENCE_IP_TYPE_INVALID;
}

IpAddress::IpAddress(const unsigned char ipAddress[],
    fiftyoneDegreesEvidenceIpType type) {
    init(ipAddress, type);
}

IpAddress::IpAddress(const char *ipAddressString) {
    fiftyoneDegreesEvidenceIpAddress *eIpAddress = 
		fiftyoneDegreesIpParseAddress(
			fiftyoneDegreesMalloc,
			ipAddressString,
			ipAddressString + strlen(ipAddressString));
    // Make sure the ip address has been parsed successfully
	if (eIpAddress == NULL) {
		throw bad_alloc();
	}
    // Initialize the IP address object
    init(eIpAddress->address, eIpAddress->type);

    // Free the previously allocated IP address
    fiftyoneDegreesFree(eIpAddress);
}

void IpAddress::init(const unsigned char *ipAddress,
    fiftyoneDegreesEvidenceIpType type) {
    switch (type) {
    case FIFTYONE_DEGREES_EVIDENCE_IP_TYPE_IPV4:
        memcpy(this->ipAddress, ipAddress, FIFTYONE_DEGREES_IPV4_LENGTH);
        break;
    case FIFTYONE_DEGREES_EVIDENCE_IP_TYPE_IPV6:
        memcpy(this->ipAddress, ipAddress, FIFTYONE_DEGREES_IPV6_LENGTH);
        break;
    default:
        memset(this->ipAddress, 0, FIFTYONE_DEGREES_IPV6_LENGTH);
        break;
    }
    this->type = type;
}

void IpAddress::getCopyOfIpAddress(unsigned char copy[], uint32_t size) {
	uint32_t copySize = type > FIFTYONE_DEGREES_IPV6_LENGTH ?
	    size : FIFTYONE_DEGREES_IPV6_LENGTH;
	memcpy(copy, this->ipAddress, copySize);
}
