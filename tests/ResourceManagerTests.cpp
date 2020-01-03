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
 
#include "pch.h"
#include "../resource.h"
#include "Base.hpp"

class ResourceManager : public Base
{
private:

	/**
	* Set the resource to true to indicate this method has been called by the
	* resource manager.
	*/
	static void freeResource(void *resourcePtr) {
		bool *resource = (bool*)resourcePtr;
		*resource = true;
	}

protected:

	/**
	 * Initialise the resource manager with a boolean resource, and set the
	 * resource to false to indicate the free method has not yet been called.
	 */
	void SetUp() {
		Base::SetUp();
		resource = false;
		fiftyoneDegreesResourceManagerInit(
			&manager,
			&resource,
			&resourceHandle,
			ResourceManager::freeResource);
	}

	/**
	 * Releases the resource manager structure if one was created. Then calls
	 * the base teardown method to check for memory leaks and compare expected
	 * and actual memory allocations.
	 */
	void TearDown() {
		disposeManager();
		ASSERT_TRUE(resource) <<
			"The resource was not closed.";
		Base::TearDown();
	}

	/**
	 * Dispose of the resource manager and check that the resource has been 
	 * freed.
	 */
	void disposeManager() {
		if (disposed == false) {
			disposed = true;
			fiftyoneDegreesResourceManagerFree(&manager);
		}
	}

	fiftyoneDegreesResourceManager manager;

	bool disposed = false;

	// The resource to use with the test.
	bool resource;

	// The handle assigned to the resource.
	fiftyoneDegreesResourceHandle *resourceHandle;
};

/**
 * Check that a resource manager contains the correct resource when it is
 * initialised.
 */
TEST_F(ResourceManager, Init) {
	ASSERT_EQ((void*)&resource, (void*)manager.active->resource) <<
		"The resource was not set correctly.";
}

/**
 * Check that the IncUse method returns a valid handle to the resource.
 */
TEST_F(ResourceManager, Handle_Inc) {
	fiftyoneDegreesResourceHandle *handle =
		fiftyoneDegreesResourceHandleIncUse(&manager);
	ASSERT_NE((void*)NULL, (void*)&handle) <<
		"A null handle was returned.";
	ASSERT_EQ((void*)&resource, (void*)handle->resource) <<
		"The handle does not contain the correct resource.";
	ASSERT_EQ(1, handle->inUse) <<
		"The in use counter was not incremented correctly.";
	ASSERT_EQ(&manager, handle->manager) <<
		"The handle is not linked to the manager.";
	fiftyoneDegreesResourceHandleDecUse(handle);
}

/**
 * Check that the DecUse method decrements the in use counter.
 */
TEST_F(ResourceManager, Handle_IncDec) {
	fiftyoneDegreesResourceHandle *handle =
		fiftyoneDegreesResourceHandleIncUse(&manager);
	ASSERT_EQ(1, handle->inUse) <<
		"The in use counter was not incremented correctly.";
	fiftyoneDegreesResourceHandleDecUse(handle);
	ASSERT_EQ(0, handle->inUse) <<
		"The in use counter was not decremented correctly.";
}

/**
 * Check that the resource replace method correctly replaces the resource, and
 * that all resources are freed.
 */
TEST_F(ResourceManager, ResourceReplace) {
	fiftyoneDegreesResourceHandle *handle = NULL;
	bool newResource = false;
	fiftyoneDegreesResourceReplace(
		&manager,
		(void*)&newResource,
		&handle);
	ASSERT_NE((void*)NULL, (void*)handle) <<
		"The resource was not replaced in the manager.";
	ASSERT_EQ(&newResource, handle->resource) <<
		"The new resource is not correct.";
	disposeManager();
	ASSERT_TRUE(newResource) <<
		"The new resource was not closed.";
}

/**
 * Check that the manager free method leaves the resource open until it is
 * released, and that the resource is freed in the end.
 */
TEST_F(ResourceManager, Free_HandleInUse) {
	fiftyoneDegreesResourceHandle *handle;
	handle = fiftyoneDegreesResourceHandleIncUse(&manager);
	disposeManager();
	ASSERT_EQ(&resource, handle->resource) <<
		"The new resource is not correct.";
	ASSERT_FALSE(resource) <<
		"The old resource was closed prematurely.";
	fiftyoneDegreesResourceHandleDecUse(handle);
	ASSERT_TRUE(resource) <<
		"The new resource was not closed.";
}

/**
 * Check that the resource replace method leaves the old resource open until it
 * is released.
 */
TEST_F(ResourceManager, ResourceReplace_HandleInUse) {
	fiftyoneDegreesResourceHandle *oldHandle, *newHandle;
	bool newResource = false;
	oldHandle = fiftyoneDegreesResourceHandleIncUse(&manager);
	fiftyoneDegreesResourceReplace(&manager, (void*)&newResource, &newHandle);
	ASSERT_NE((void*)NULL, (void*)newHandle) <<
		"The resource was not replaced in the manager.";
	ASSERT_EQ(&newResource, newHandle->resource) <<
		"The new resource is not correct.";
	ASSERT_FALSE(resource) <<
		"The old resource was closed prematurely.";
	fiftyoneDegreesResourceHandleDecUse(oldHandle);
	disposeManager();
	ASSERT_TRUE(newResource) <<
		"The new resource was not closed.";
}