#pragma once

#include "m2fixbase.h"

template<typename T>
class M2InteropBasicString;
using M2InteropString = M2InteropBasicString<char>;

class Config : public M2FixBase
{
public:
	static auto & GetInstance()
	{
		static Config instance;
		return instance;
	}

	static void LoadInstance() {
		GetInstance().Load();
	}

	virtual void Load() override;

private:
#ifdef _WIN64
	static const char *GetCfgValue(uintptr_t *ctx, const M2InteropString id);
	static int GetCfgValueEx(uintptr_t *ctx, const M2InteropString *id);

#else
	static const char * __fastcall GetCfgValue(uintptr_t *ctx, uintptr_t _EDX, const M2InteropString id, uintptr_t index);
	static int __fastcall GetCfgValueEx(uintptr_t *ctx, uintptr_t _EDX, const M2InteropString *id);
#endif
};
