#include "emutask.h"

template <Squirk T>
bool EmuTask<T>::SetSmoothing(SQBool enable)
{
    // g_emu_task.setSmoothing(g_emu_task, enable);
    return M2Object<T>::SetBool(_SC("setSmoothing"), enable);
}

template <Squirk T>
bool EmuTask<T>::SetScanline(SQBool enable)
{
    // g_emu_task.setScanline(g_emu_task, enable);
    return M2Object<T>::SetBool(_SC("setScanline"), enable);
}

template <Squirk T>
bool EmuTask<T>::SetInputDirectionMerge(SQInteger mode)
{
    // g_emu_task.setInputDirectionMerge(g_emu_task, mode);
    return M2Object<T>::SetInteger(_SC("setInputDirectionMerge"), mode);
}

template <Squirk T>
bool EmuTask<T>::SetInputDeadzone(SQFloat value)
{
    // g_emu_task.setInputDeadzone(g_emu_task, value);
    return M2Object<T>::SetFloat(_SC("setInputDeadzone"), value);
}

template <Squirk T>
bool EmuTask<T>::SetInfoInteger(const SQChar *key, SQInteger value)
{
    // g_emu_task.setInfoInteger(g_emu_task, key, value);
    return M2Object<T>::SetInfoInteger(_SC("setInfoInteger"), key, value);
}

template <Squirk T>
bool EmuTask<T>::GetInfoInteger(const SQChar *key, SQInteger *value)
{
    // g_emu_task.getInfoInteger(g_emu_task, key, value);
    return M2Object<T>::GetInfoInteger(_SC("getInfoInteger"), key, value);
}

template <Squirk T>
bool EmuTask<T>::GetRamValue(SQInteger width, SQInteger offset, SQInteger *value)
{
    if (!M2Object<T>::GetClosure(_SC("getRamValue"))) return false;

    sq_push<T>(M2Object<T>::m_vm, -2);
    sq_pushinteger<T>(M2Object<T>::m_vm, width);
    sq_pushinteger<T>(M2Object<T>::m_vm, offset);
    SQRESULT res = sq_call<T>(M2Object<T>::m_vm, 3, true, false);
    sq_getinteger<T>(M2Object<T>::m_vm, -1, value);
    sq_pop<T>(M2Object<T>::m_vm, 3);

    if (SQ_SUCCEEDED(res)) return true;
    return false;
}

template <Squirk T>
bool EmuTask<T>::SetRamValue(SQInteger width, SQInteger offset, SQInteger value)
{
    if (!M2Object<T>::GetClosure(_SC("setRamValue"))) return false;

    sq_push<T>(M2Object<T>::m_vm, -2);
    sq_pushinteger<T>(M2Object<T>::m_vm, width);
    sq_pushinteger<T>(M2Object<T>::m_vm, offset);
    sq_pushinteger<T>(M2Object<T>::m_vm, value);
    SQRESULT res = sq_call<T>(M2Object<T>::m_vm, 4, false, false);
    sq_pop<T>(M2Object<T>::m_vm, 2);

    if (SQ_SUCCEEDED(res)) return true;
    return false;
}

template <Squirk T>
bool EmuTask<T>::EntryCdRomPatch(SQInteger offset, SQArray<T> *data)
{
    SQObjectPtr<T> dataObj(data);
    if (!M2Object<T>::GetClosure(_SC("entryCdRomPatch"))) return false;

    sq_push<T>(M2Object<T>::m_vm, -2);
    sq_pushinteger<T>(M2Object<T>::m_vm, offset);
    sq_pushobject<T>(M2Object<T>::m_vm, dataObj);
    SQRESULT res = sq_call<T>(M2Object<T>::m_vm, 3, false, false);
    sq_pop<T>(M2Object<T>::m_vm, 2);

    if (SQ_SUCCEEDED(res)) return true;
    return false;
}

template <Squirk T>
bool EmuTask<T>::ReleaseCdRomPatch()
{
    return M2Object<T>::Void(_SC("releaseCdRomPatch"));
}

template <Squirk T>
bool EmuTask<T>::UpdateEmuScreen()
{
    return M2Object<T>::Void(_SC("updateEmuScreen"));
}
