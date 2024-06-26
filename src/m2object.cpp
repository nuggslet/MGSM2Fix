#include "m2object.h"

template M2Object<Squirk::Standard>;
template M2Object<Squirk::AlignObject>;

template <Squirk T>
M2Object<T>::M2Object(const SQChar *name)
    : m_name(name)
    , m_vm(NULL)
{
}

template <Squirk T>
void M2Object<T>::SetVM(HSQUIRRELVM<T> v)
{
    m_vm = v;
}

template <Squirk T>
bool M2Object<T>::Get()
{
    sq_pushroottable(m_vm);
    sq_pushstring(m_vm, m_name, -1);
    if (SQ_FAILED(sq_get(m_vm, -2))) {
        sq_pop(m_vm, 1);
        return false;
    }

    if (sq_gettype(m_vm, -1) != OT_INSTANCE)
    {
        sq_pop(m_vm, 2);
        return false;
    }

    return true;
}

template <Squirk T>
bool M2Object<T>::Get(const SQChar *name, SQObjectType type)
{
    if (!Get()) return false;

    sq_pushstring(m_vm, name, -1);
    sq_get(m_vm, -2);

    if (type != OT_NULL && sq_gettype(m_vm, -1) != type)
    {
        sq_pop(m_vm, 2);
        return false;
    }

    return true;
}

template <Squirk T>
bool M2Object<T>::GetClosure(const SQChar *name)
{
    if (!Get()) return false;

    sq_pushstring(m_vm, name, -1);
    sq_get(m_vm, -2);

    if (sq_gettype(m_vm, -1) != OT_CLOSURE && sq_gettype(m_vm, -1) != OT_NATIVECLOSURE)
    {
        sq_pop(m_vm, 2);
        return false;
    }

    return true;
}

template <Squirk T>
bool M2Object<T>::Void(const SQChar *name)
{
    if (!GetClosure(name)) return false;

    sq_push(m_vm, -2);
    SQRESULT res = sq_call(m_vm, 2, false, false);
    sq_pop(m_vm, 2);

    if (SQ_SUCCEEDED(res)) return true;
    return false;
}

template <Squirk T>
bool M2Object<T>::SetBool(const SQChar *name, SQBool value)
{
    if (!GetClosure(name)) return false;

    sq_push(m_vm, -2);
    sq_pushbool(m_vm, value);
    SQRESULT res = sq_call(m_vm, 2, false, false);
    sq_pop(m_vm, 2);

    if (SQ_SUCCEEDED(res)) return true;
    return false;
}

template <Squirk T>
bool M2Object<T>::SetInteger(const SQChar *name, SQInteger value)
{
    if (!GetClosure(name)) return false;

    sq_push(m_vm, -2);
    sq_pushinteger(m_vm, value);
    SQRESULT res = sq_call(m_vm, 2, false, false);
    sq_pop(m_vm, 2);

    if (SQ_SUCCEEDED(res)) return true;
    return false;
}

template <Squirk T>
bool M2Object<T>::SetFloat(const SQChar *name, SQFloat value)
{
    if (!GetClosure(name)) return false;

    sq_push(m_vm, -2);
    sq_pushfloat(m_vm, value);
    SQRESULT res = sq_call(m_vm, 2, false, false);
    sq_pop(m_vm, 2);

    if (SQ_SUCCEEDED(res)) return true;
    return false;
}

template <Squirk T>
bool M2Object<T>::SetInfoInteger(const SQChar *name, const SQChar *key, SQInteger value)
{
    if (!GetClosure(name)) return false;

    sq_push(m_vm, -2);
    sq_pushstring(m_vm, key, -1);
    sq_pushinteger(m_vm, value);
    SQRESULT res = sq_call(m_vm, 3, false, false);
    sq_pop(m_vm, 2);

    if (SQ_SUCCEEDED(res)) return true;
    return false;
}

template <Squirk T>
bool M2Object<T>::GetBool(const SQChar *name, SQBool *value)
{
    if (!GetClosure(name)) return false;

    sq_push(m_vm, -2);
    SQRESULT res = sq_call(m_vm, 1, true, false);
    sq_getbool(m_vm, -1, value);
    sq_pop(m_vm, 3);

    if (SQ_SUCCEEDED(res)) return true;
    return false;
}

template <Squirk T>
bool M2Object<T>::GetInteger(const SQChar *name, SQInteger *value)
{
    if (!GetClosure(name)) return false;

    sq_push(m_vm, -2);
    SQRESULT res = sq_call(m_vm, 1, true, false);
    sq_getinteger(m_vm, -1, value);
    sq_pop(m_vm, 3);

    if (SQ_SUCCEEDED(res)) return true;
    return false;
}

template <Squirk T>
bool M2Object<T>::GetFloat(const SQChar *name, SQFloat *value)
{
    if (!GetClosure(name)) return false;

    sq_push(m_vm, -2);
    SQRESULT res = sq_call(m_vm, 1, true, false);
    sq_getfloat(m_vm, -1, value);
    sq_pop(m_vm, 3);

    if (SQ_SUCCEEDED(res)) return true;
    return false;
}

template <Squirk T>
bool M2Object<T>::GetInfoInteger(const SQChar *name, const SQChar *key, SQInteger *value)
{
    if (!GetClosure(name)) return false;

    sq_push(m_vm, -2);
    sq_pushstring(m_vm, key, -1);
    SQRESULT res = sq_call(m_vm, 2, true, false);
    sq_getinteger(m_vm, -1, value);
    sq_pop(m_vm, 3);

    if (SQ_SUCCEEDED(res)) return true;
    return false;
}
