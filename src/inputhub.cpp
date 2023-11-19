#include "inputhub.h"

bool InputHub::SetDirectionMerge(SQInteger mode)
{
    // g_inputHub.setDirectionMerge(g_inputHub, mode);
    return SetInteger(_SC("setDirectionMerge"), mode);
}

bool InputHub::SetDeadzone(SQFloat value)
{
    // g_inputHub.setDeadzone(g_inputHub, value);
    return SetFloat(_SC("setDeadzone"), value);
}
