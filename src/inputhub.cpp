#include "inputhub.h"

template <Squirk T>
bool InputHub<T>::SetDirectionMerge(SQInteger mode)
{
    // g_inputHub.setDirectionMerge(g_inputHub, mode);
    return M2Object<T>::SetInteger(_SC("setDirectionMerge"), mode);
}

template <Squirk T>
bool InputHub<T>::SetDeadzone(SQFloat value)
{
    // g_inputHub.setDeadzone(g_inputHub, value);
    return M2Object<T>::SetFloat(_SC("setDeadzone"), value);
}
