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
