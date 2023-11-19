#include "input.h"

bool Input::GetAnalogStickX(SQFloat *value)
{
    // g_input.getAnalogStickX(g_input, value);
    return GetFloat(_SC("getAnalogStickX"), value);
}

bool Input::GetAnalogStickY(SQFloat *value)
{
    // g_input.getAnalogStickY(g_input, value);
    return GetFloat(_SC("getAnalogStickY"), value);
}

bool Input::GetRightAnalogStickX(SQFloat *value)
{
    // g_input.getRightAnalogStickX(g_input, value);
    return GetFloat(_SC("getRightAnalogStickX"), value);
}

bool Input::GetRightAnalogStickY(SQFloat *value)
{
    // g_input.getRightAnalogStickY(g_input, value);
    return GetFloat(_SC("getRightAnalogStickY"), value);
}
