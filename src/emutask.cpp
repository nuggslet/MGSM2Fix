#include "emutask.h"

bool EmuTask::SetSmoothing(SQBool enable)
{
    // g_emu_task.setSmoothing(g_emu_task, enable);
    return SetBool(_SC("setSmoothing"), enable);
}

bool EmuTask::SetScanline(SQBool enable)
{
    // g_emu_task.setScanline(g_emu_task, enable);
    return SetBool(_SC("setScanline"), enable);
}

bool EmuTask::SetInputDirectionMerge(SQInteger mode)
{
    // g_emu_task.setInputDirectionMerge(g_emu_task, mode);
    return SetInteger(_SC("setInputDirectionMerge"), mode);
}

bool EmuTask::SetInputDeadzone(SQFloat value)
{
    // g_emu_task.setInputDeadzone(g_emu_task, value);
    return SetFloat(_SC("setInputDeadzone"), value);
}

bool EmuTask::SetInfoInteger(const SQChar *key, SQInteger value)
{
    // g_emu_task.setInfoInteger(g_emu_task, key, value);
    return M2Object::SetInfoInteger(_SC("setInfoInteger"), key, value);
}

bool EmuTask::GetInfoInteger(const SQChar *key, SQInteger *value)
{
    // g_emu_task.getInfoInteger(g_emu_task, key, value);
    return M2Object::GetInfoInteger(_SC("getInfoInteger"), key, value);
}

bool EmuTask::GetRamValue(SQInteger width, SQInteger offset, SQInteger *value)
{
    if (!GetClosure(_SC("getRamValue"))) return false;

    sq_push(m_vm, -2);
    sq_pushinteger(m_vm, width);
    sq_pushinteger(m_vm, offset);
    SQRESULT res = sq_call(m_vm, 3, true, false);
    sq_getinteger(m_vm, -1, value);
    sq_pop(m_vm, 3);

    if (SQ_SUCCEEDED(res)) return true;
    return false;
}

bool EmuTask::SetRamValue(SQInteger width, SQInteger offset, SQInteger value)
{
    if (!GetClosure(_SC("setRamValue"))) return false;

    sq_push(m_vm, -2);
    sq_pushinteger(m_vm, width);
    sq_pushinteger(m_vm, offset);
    sq_pushinteger(m_vm, value);
    SQRESULT res = sq_call(m_vm, 4, false, false);
    sq_pop(m_vm, 2);

    if (SQ_SUCCEEDED(res)) return true;
    return false;
}

bool EmuTask::EntryCdRomPatch(SQInteger offset, SQArray *data)
{
    SQObjectPtr dataObj(data);
    if (!GetClosure(_SC("entryCdRomPatch"))) return false;

    sq_push(m_vm, -2);
    sq_pushinteger(m_vm, offset);
    sq_pushobject(m_vm, dataObj);
    SQRESULT res = sq_call(m_vm, 3, false, false);
    sq_pop(m_vm, 2);

    if (SQ_SUCCEEDED(res)) return true;
    return false;
}

bool EmuTask::ReleaseCdRomPatch()
{
    return Void(_SC("releaseCdRomPatch"));
}

bool EmuTask::UpdateEmuScreen()
{
    return Void(_SC("updateEmuScreen"));
}
