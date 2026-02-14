#pragma once

#include "m2hook.h"
#include "stdafx.h"

class M2Utils
{
public:
	M2Utils() {}

	static std::filesystem::path EnsureAppData();

	static void LogSystemInfo();

	static void CompatibilityWarnings();

	static void memsetHook();
	static void * __cdecl memsetWait(void *str, int c, size_t n);
	static void memsetRelease();

	static void nullsub();

	static inline std::mutex memsetHookMutex = {};
	static inline bool memsetHookCalled = false;

	static inline std::mutex mainThreadFinishedMutex = {};
	static inline std::condition_variable mainThreadFinishedVar = {};
	static inline bool mainThreadFinished = false;

	static bool IsSteamOS();
	static std::string GetSteamOSVersion();
};
