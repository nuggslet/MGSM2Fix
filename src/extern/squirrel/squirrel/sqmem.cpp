/*
	see copyright notice in squirrel.h
*/
#include "sqpcheader.h"

#include <basetsd.h>

#pragma runtime_checks("", off)

void * (__fastcall *_sq_vm_realloc)(void *, SQUnsignedInteger, SQUnsignedInteger);

void *sq_vm_malloc(SQUnsignedInteger size)
{
    if (_sq_vm_realloc) {
        void *p = _sq_vm_realloc(nullptr, 0, size);
#ifndef _WIN64
        __asm { add esp, 4 } // "fastcall" but with caller cleanup of the stack argument - LTO?!
#endif
        return p;
    }

    return nullptr;
}

void *sq_vm_realloc(void *p, SQUnsignedInteger oldsize, SQUnsignedInteger size)
{
    if (_sq_vm_realloc) {
        p = _sq_vm_realloc(p, oldsize, size);
#ifndef _WIN64
        __asm { add esp, 4 } // "fastcall" but with caller cleanup of the stack argument - LTO?!
#endif
        return p;
    }

    return nullptr;
}

void sq_vm_free(void *p, SQUnsignedInteger size)
{
    if (_sq_vm_realloc) {
        _sq_vm_realloc(p, size, 0);
#ifndef _WIN64
        __asm { add esp, 4 } // "fastcall" but with caller cleanup of the stack argument - LTO?!
#endif
    }
}

#pragma runtime_checks("", restore)
