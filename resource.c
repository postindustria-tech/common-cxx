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

#include "resource.h"

#include "fiftyone.h"

/**
 * Returns the handle to the resource that is ready to be set for the manager,
 * or NULL if the handle was not successfully created.
 * @param manager of the resource
 * @param resource to be assigned to the manager
 * @parma resourceHandle reference to the handle within the resource
 */
static void setupResource(
	ResourceManager *manager, 
	void *resource, 
	ResourceHandle **resourceHandle,
	void(*freeResource)(void*)) {

	// Create a new active handle for the manager. Align this to double
	// arcitecture's bus size to enable double width atomic operations.
	ResourceHandle *handle = (ResourceHandle*)MallocAligned(
		sizeof(void*) * 2,
		sizeof(ResourceHandle));

	// Set the number of users of the resource to zero.
	handle->self = handle;
	handle->counter.padding = NULL;

	// Set a link between the new active resource and the manager. Used to
	// check if the resource can be freed when the last thread has finished
	// using it.
	handle->manager = manager;

	// Set the resource that the new handle manages to the one provided.
	handle->resource = resource;

	// Set the free resource method as this may not be available if the manager
	// is disposed of.
	handle->freeResource = freeResource;

	// Ensure the resource's handle is set before assigning the handle
	// as the active handle.
	*resourceHandle = handle;
}

static void freeHandle(ResourceHandle *handle) {
	handle->freeResource((void*)handle->resource);
	FreeAligned((void*)handle);
}

void fiftyoneDegreesResourceManagerInit(
	fiftyoneDegreesResourceManager *manager,
	void *resource,
	fiftyoneDegreesResourceHandle **resourceHandle,
	void(*freeResource)(void*)) {

	// Initialise the manager with the resource ensuring that the resources
	// handle is set before it's made the active resource.
	setupResource(manager, resource, resourceHandle, freeResource);
	manager->active = *resourceHandle;
}

void fiftyoneDegreesResourceManagerFree(
	fiftyoneDegreesResourceManager *manager) {
	assert(manager->active->counter.inUse >= 0);
	if (manager->active != NULL) {

		ResourceHandle* newHandlePointer;
		fiftyoneDegreesResourceReplace(
			manager,
			NULL,
			&newHandlePointer);
		FreeAligned(newHandlePointer);
	}
}

void fiftyoneDegreesResourceHandleDecUse(
	fiftyoneDegreesResourceHandle *handle) {
	// When modifying this method, it is important to note the reason for using
	// two separate compareand swaps. The first compare and swap ensures that
	// we are certain the handle is ready to be released i.e. the inUse counter
	// is zero, and the handle is no longer active in the manager. The second
	// compare and swap ensures that we are certain the handle can be freed by
	// THIS thread. See below for an example of when this can happen.

	assert(handle->counter.inUse > 0);
	ResourceHandle decremented;
#ifndef FIFTYONE_DEGREES_NO_THREADING
	ResourceHandle tempHandle;
	do {
		tempHandle = *handle;
		decremented = tempHandle;

		decremented.counter.inUse--;

	} while (INTERLOCK_EXCHANGE_PTR_DW(
		handle,
		decremented,
		tempHandle) == false);
#else
	handle->counter.inUse--;
	decremented = *handle;
#endif
	if (decremented.counter.inUse == 0 &&  // Am I the last user of the handle?
		decremented.manager->active != decremented.self) { // Is the handle still active?
#ifndef FIFTYONE_DEGREES_NO_THREADING
		// Atomically set the handle's self pointer to null to ensure only
		// one thread can get into the freeHandle method.
		// Consider the scenario where between the decrement this if statement:
		// 1. another thread increments and decrements the counter, then
		// 2. the active handle is replaced.
		// In this case, both threads will make it into here, so access to
		// the freeHandle method must be limted to one by atomically nulling
		// the self pointer. We will still have access to the pointer for
		// freeing through the decremented copy.
		ResourceHandle nulled = decremented;
		nulled.self = NULL;
		if (INTERLOCK_EXCHANGE_PTR_DW(
			decremented.self,
			nulled,
			decremented)) {
			freeHandle(decremented.self);
		}
#else
		freeHandle(decremented.self);
#endif
	}
}

fiftyoneDegreesResourceHandle* fiftyoneDegreesResourceHandleIncUse(
	fiftyoneDegreesResourceManager *manager) {
	ResourceHandle incremented;
#ifndef FIFTYONE_DEGREES_NO_THREADING
	ResourceHandle tempHandle;
	do {
		tempHandle = *manager->active;
		incremented = tempHandle;

		incremented.counter.inUse++;
	} while (INTERLOCK_EXCHANGE_PTR_DW(
		manager->active,
		incremented,
		tempHandle) == false);
#else
	manager->active->inUse++;
	incremented = *manager->active;
#endif
	return incremented.self;
}

void fiftyoneDegreesResourceReplace(
	fiftyoneDegreesResourceManager *manager,
	void *newResource,
	fiftyoneDegreesResourceHandle **newResourceHandle) {

	ResourceHandle* oldHandle  = NULL;
	
	// Add the new resource to the manager replacing the existing one.
	setupResource(
		manager,
		newResource,
		newResourceHandle,
		manager->active->freeResource);
#ifndef FIFTYONE_DEGREES_NO_THREADING
	// Switch the active handle for the manager to the newly created one.
	do {
		if (oldHandle != NULL) {
			ResourceHandleDecUse(oldHandle);
		}
		oldHandle = ResourceHandleIncUse(manager);
	} while (INTERLOCK_EXCHANGE_PTR(
		manager->active,
		*newResourceHandle,
		oldHandle) == false);
#else
	oldHandle = ResourceHandleIncUse(manager);
	manager->active = *newResourceHandle;
#endif
	// Release the existing resource can be freed. If nothing else is
	// holding onto a reference to it then free it will be freed.
	ResourceHandleDecUse(oldHandle);
}