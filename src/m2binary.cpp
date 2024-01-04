#include "m2binary.h"

M2Binary::M2Binary(HSQUIRRELVM v, HSQOBJECT obj)
    : m_vm(v)
    , m_obj(obj)
{
}

bool M2Binary::GetClosure(const SQChar *name)
{
    sq_pushobject(m_vm, m_obj);
    sq_pushstring(m_vm, name, -1);
    sq_get(m_vm, -2);

    if (sq_gettype(m_vm, -1) != OT_CLOSURE && sq_gettype(m_vm, -1) != OT_NATIVECLOSURE)
    {
        sq_pop(m_vm, 2);
        return false;
    }

    return true;
}

bool M2Binary::At(SQInteger offset, SQInteger *value)
{
    if (!GetClosure("at")) return false;

    sq_push(m_vm, -2);
    sq_pushinteger(m_vm, offset);
    SQRESULT res = sq_call(m_vm, 2, true, false);
    sq_getinteger(m_vm, -1, value);
    sq_pop(m_vm, 3);

    if (SQ_SUCCEEDED(res)) return true;
    return false;
}

bool M2Binary::Size(SQInteger *size)
{
    if (!GetClosure("size")) return false;

    sq_push(m_vm, -2);
    SQRESULT res = sq_call(m_vm, 1, true, false);
    sq_getinteger(m_vm, -1, size);
    sq_pop(m_vm, 3);

    if (SQ_SUCCEEDED(res)) return true;
    return false;
}
