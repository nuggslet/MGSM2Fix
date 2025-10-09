#pragma once

#include "m2hook.h"
#include "stdafx.h"

class M2Utils
{
public:
	M2Utils() {}

	static void LogSystemInfo();

	static void CompatibilityWarnings();

	static void memsetHook();
	static void *memsetWait(void *str, int c, size_t n);
	static void memsetRelease();

	static std::mutex memsetHookMutex;
	static bool memsetHookCalled;

	static std::mutex mainThreadFinishedMutex;
	static std::condition_variable mainThreadFinishedVar;
	static bool mainThreadFinished;

	static bool IsSteamOS();
	static std::string GetSteamOSVersion();
};
