/* *********************************************************************
 * This Original Work is copyright of 51 Degrees Mobile Experts Limited.
 * Copyright 2019 51 Degrees Mobile Experts Limited, 5 Charlotte Close,
 * Caversham, Reading, Berkshire, United Kingdom RG4 7BY.
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

#include "CollectionConfig.hpp"

using namespace FiftyoneDegrees::Common;

#ifdef _MSC_VER
// A default constructor is needed for the SWIG interface. This is only
// used for forward declarations `CollectionConfig config` where a default
// constructor will be called, but it will actually be initialised later.
#pragma warning (disable:26495)  
#endif
CollectionConfig::CollectionConfig() { }
#ifdef _MSC_VER
#pragma warning (default:26495)  
#endif

CollectionConfig::CollectionConfig(
	fiftyoneDegreesCollectionConfig *config) {
	this->config = config;
}

void CollectionConfig::setCapacity(uint32_t capacity) {
	config->capacity = capacity; 
}

void CollectionConfig::setConcurrency(uint16_t concurrency) {
	config->concurrency = concurrency;
}

void CollectionConfig::setLoaded(uint32_t loaded) {
	config->loaded = loaded; 
}

uint32_t CollectionConfig::getCapacity() {
	return config->capacity; 
}

uint16_t CollectionConfig::getConcurrency() { 
	return config->concurrency; 
}

uint32_t CollectionConfig::getLoaded() { 
	return config->loaded;
}

fiftyoneDegreesCollectionConfig* CollectionConfig::getConfig() {
	return config;
}