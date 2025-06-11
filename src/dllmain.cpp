#include "m2fix.h"

DWORD WINAPI ThreadProc(LPVOID lpThreadParameter)
{
    HINSTANCE hinstDLL = reinterpret_cast<HINSTANCE>(lpThreadParameter);
    M2Fix::Main(hinstDLL);

    M2Utils::memsetRelease();
    return 1;
}

void Main(HINSTANCE hinstDLL)
{
    M2Utils::memsetHook();

    HANDLE handle = CreateThread(NULL, 0, ThreadProc, hinstDLL, NULL, 0);
    if (!handle) return;

    SetThreadPriority(handle, THREAD_PRIORITY_HIGHEST);
    CloseHandle(handle);

    // fixes the monitor going to sleep during cutscenes
    SetThreadExecutionState(ES_CONTINUOUS | ES_SYSTEM_REQUIRED | ES_DISPLAY_REQUIRED);
}

BOOL APIENTRY DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
        {
            Main(hinstDLL);
            break;
        }

        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
        case DLL_PROCESS_DETACH:
            break;
    }

    return TRUE;
}
