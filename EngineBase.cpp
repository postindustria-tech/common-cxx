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

#include "EngineBase.hpp"

#include "fiftyone.h"

using namespace FiftyoneDegrees::Common;

EngineBase::EngineBase(
	ConfigBase *config,
	RequiredPropertiesConfig *requiredProperties) {
	defaultDataKey = "device";
	updateUrl = string("");
	licenceKey = string("");
	manager = make_shared<fiftyoneDegreesResourceManager>();
	manager->active = nullptr;
	this->requiredProperties = requiredProperties;
	this->config = config;
	metaData = nullptr;
}

EngineBase::~EngineBase() {
	
	// Free the meta data if set.
	delete metaData;
	
	// Release the resource used by the engine.
	if (manager->active != nullptr) {
		ResourceManagerFree(manager.get());
	}
}

void EngineBase::addKey(string key) {
	if (find(keys.begin(), keys.end(), key) == keys.end()) {
		keys.push_back(key);
	}
}

void EngineBase::initOverrideKeys(
	fiftyoneDegreesOverridePropertyArray *overrideProperties) {
	uint32_t i;
	const char *tempKey;
	if (overrideProperties != nullptr) {
		for (i = 0; i < overrideProperties->count; i++) {
			string key = string("cookie.");
			if (overrideProperties->prefix == true) {
				key.append("51D_");
			}
			tempKey = STRING(
				overrideProperties->items[i].available->name.data.ptr);
			if (tempKey != nullptr) {
				key.append(tempKey);
				addKey(key);
			}
			else {
				throw invalid_argument("Override evidence key was null.");
			}
			key = string("query.");
			if (overrideProperties->prefix == true) {
				key.append("51D_");
			}
			tempKey = STRING(
				overrideProperties->items[i].available->name.data.ptr);
			if (tempKey != nullptr) {
				key.append(tempKey);
				addKey(key);
			}
			else {
				throw invalid_argument("Override evidence key was null.");
			}
		}
	}
}

void EngineBase::initHttpHeaderKeys(
	fiftyoneDegreesHeaders *uniqueHeaders) {
	uint32_t i, p;
	const char *prefixes[] = { "header.", "query." };
	for (i = 0; i < uniqueHeaders->count; i++) {
		for (p = 0; p < sizeof(prefixes) / sizeof(const char*); p++) {
			string key = string(prefixes[p]);
			if (config->getUseUpperPrefixHeaders()) {
				key.append("HTTP_");
			}
			key.append(&((fiftyoneDegreesString*)
				uniqueHeaders->items[i].name.data.ptr)->value);
			addKey(key);
		}
	}
}

void EngineBase::setLicenseKey(const string &licenseKey) {
	this->licenceKey.assign(licenseKey);
}

void EngineBase::setDataUpdateUrl(const string &newUpdateUrl) {
	this->updateUrl.assign(newUpdateUrl);
}

string EngineBase::getDataUpdateUrl() {
	stringstream stream;
	if (updateUrl.empty() == false) {
		stream << updateUrl;
	}
	else if (licenceKey.empty() == false) {
		stream << "https://distributor.51degrees.com/api/v2/download"
			<< "?LicenceKeys=" << licenceKey.c_str()
			<< "&Type=" << getType()
			<< "&Product=" << getProduct();
	}
	return stream.str();
}

MetaData* EngineBase::getMetaData() {
	return metaData;
}

bool EngineBase::getAutomaticUpdatesEnabled() {
	return getDataUpdateUrl().empty() == false;
}

vector<string>* EngineBase::getKeys() {
	return &keys;
}

bool EngineBase::getIsThreadSafe() {
	return ThreadingGetIsThreadSafe();
}

void EngineBase::appendString(
	stringstream &stream,
	fiftyoneDegreesCollection *strings,
	uint32_t offset) {
	EXCEPTION_CREATE;
	Item item;
	DataReset(&item.data);
	String *string = StringGet(strings, offset, &item, exception);
	if (string != NULL && EXCEPTION_OKAY) {
		stream << STRING(string);
		COLLECTION_RELEASE(strings, &item);
	}
	EXCEPTION_THROW;
}