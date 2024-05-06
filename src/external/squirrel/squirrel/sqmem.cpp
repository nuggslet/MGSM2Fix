/*
	see copyright notice in squirrel.h
*/
#include "sqpcheader.h"

#include <basetsd.h>
extern void *M2_malloc(size_t size);
extern void *M2_realloc(void *p, size_t length, size_t size);
extern void M2_free(void *p);

void *sq_vm_malloc(SQUnsignedInteger size) { return M2_malloc(size); }

void *sq_vm_realloc(void *p, SQUnsignedInteger oldsize, SQUnsignedInteger size){ return M2_realloc(p, oldsize, size); }

void sq_vm_free(void *p, SQUnsignedInteger size){ M2_free(p); }
