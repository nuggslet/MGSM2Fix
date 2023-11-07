/*
	see copyright notice in squirrel.h
*/
#include "sqpcheader.h"

#include <basetsd.h>
extern DWORD32 M2_mallocAddress;
extern DWORD32 M2_reallocAddress;
extern DWORD32 M2_freeAddress;

/*
void *sq_vm_malloc(SQUnsignedInteger size) { return malloc(size); }

void *sq_vm_realloc(void *p, SQUnsignedInteger oldsize, SQUnsignedInteger size){ return realloc(p, size); }

void sq_vm_free(void *p, SQUnsignedInteger size){	free(p); }
*/

void *sq_vm_malloc(SQUnsignedInteger size)
{
	void* (*MGS1_malloc)(size_t) = (void* (*)(size_t))M2_mallocAddress;
	return MGS1_malloc(size);
}

void *sq_vm_realloc(void *p, SQUnsignedInteger oldsize, SQUnsignedInteger size)
{
	void* (*MGS1_realloc)(void*, size_t) = (void* (*)(void*, size_t))M2_reallocAddress;
	return MGS1_realloc(p, size);
}

void sq_vm_free(void *p, SQUnsignedInteger size)
{
	void* (*MGS1_free)(void*) = (void* (*)(void*))M2_freeAddress;
	MGS1_free(p);
}
