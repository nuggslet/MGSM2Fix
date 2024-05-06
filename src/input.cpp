#include "input.h"

template <Squirk T>
bool Input<T>::GetAnalogStickX(SQFloat *value)
{
    // g_input.getAnalogStickX(g_input, value);
    return M2Object<T>::GetFloat(_SC("getAnalogStickX"), value);
}

template <Squirk T>
bool Input<T>::GetAnalogStickY(SQFloat *value)
{
    // g_input.getAnalogStickY(g_input, value);
    return M2Object<T>::GetFloat(_SC("getAnalogStickY"), value);
}

template <Squirk T>
bool Input<T>::GetRightAnalogStickX(SQFloat *value)
{
    // g_input.getRightAnalogStickX(g_input, value);
    return M2Object<T>::GetFloat(_SC("getRightAnalogStickX"), value);
}

template <Squirk T>
bool Input<T>::GetRightAnalogStickY(SQFloat *value)
{
    // g_input.getRightAnalogStickY(g_input, value);
    return M2Object<T>::GetFloat(_SC("getRightAnalogStickY"), value);
}
