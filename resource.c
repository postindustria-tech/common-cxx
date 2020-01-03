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
 * Returns the handle to the resource that has been set for the manager, or
 * NULL if the handle was not successfully created.
 * @param manager of the resource
 * @param resource to be assigned to the manager
 * @parma resourceHandle reference to the handle within the resource
 */
static void setResource(
	ResourceManager *manager, 
	void *resource, 
	ResourceHandle **resourceHandle,
	void(*freeResource)(void*)) {

	// Create a new active handle for the manager.
	ResourceHandle *handle = (ResourceHandle*)Malloc(sizeof(ResourceHandle));

	// Set the number of users of the resource to zero.
	handle->inUse = 0;

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

	// Switch the active handle for the manager to the newly created one.
	manager->active = handle;
}

static void freeHandle(ResourceHandle *handle) {
	handle->freeResource((void*)handle->resource);
	Free((void*)handle);
}

void fiftyoneDegreesResourceManagerInit(
	fiftyoneDegreesResourceManager *manager,
	void *resource,
	fiftyoneDegreesResourceHandle **resourceHandle,
	void(*freeResource)(void*)) {

	// Initialise the manager with the resource ensuring that the resources
	// handle is set before it's made the active resource.
	setResource(manager, resource, resourceHandle, freeResource);
}

void fiftyoneDegreesResourceManagerFree(
	fiftyoneDegreesResourceManager *manager) {
	fiftyoneDegreesResourceHandle *resource;
	assert(manager->active->inUse >= 0);
	if (manager->active != NULL) {
		resource = ResourceHandleIncUse(manager);
		manager->active = NULL;
		ResourceHandleDecUse(resource);
	}
}

void fiftyoneDegreesResourceHandleDecUse(
	fiftyoneDegreesResourceHandle *handle) {
	assert(handle->inUse > 0);
	int inUse = 0;
#ifndef FIFTYONE_DEGREES_NO_THREADING
	inUse = FIFTYONE_DEGREES_INTERLOCK_DEC(&handle->inUse);
#else
	handle->inUse--;
	inUse = handle->inUse;
#endif
	if (inUse == 0 &&  // Am I the last user of the handle?
		handle->manager->active != handle) { // Is the handle still active?
		freeHandle(handle);
	}
}

fiftyoneDegreesResourceHandle* fiftyoneDegreesResourceHandleIncUse(
	fiftyoneDegreesResourceManager *manager) {
	ResourceHandle *handle = NULL;
#ifndef FIFTYONE_DEGREES_NO_THREADING
	do {
		if (handle != NULL) {
			ResourceHandleDecUse(handle);
		}
		handle = (ResourceHandle*)manager->active;
	} while (FIFTYONE_DEGREES_INTERLOCK_INC(&handle->inUse) == 0 ||
			 manager->active != handle);
#else
	handle = manager->active;
	handle->inUse++;
#endif
	return handle;
}

void fiftyoneDegreesResourceReplace(
	fiftyoneDegreesResourceManager *manager,
	void *newResource,
	fiftyoneDegreesResourceHandle **newResourceHandle) {
	ResourceHandle *oldHandle = ResourceHandleIncUse(manager);
	
	// Add the new resource to the manager replacing the existing one.
	setResource(
		manager,
		newResource,
		newResourceHandle, 
		oldHandle->freeResource);

	// Release the existing resource can be freed. If nothing else is
	// holding onto a reference to it then free it will be freed.
	ResourceHandleDecUse(oldHandle);
}