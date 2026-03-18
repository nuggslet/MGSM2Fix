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

template<>
struct fmt::formatter<SQObjectType> {
	constexpr auto parse(format_parse_context & ctx) {
		return ctx.begin();
	}

	template <typename FormatContext>
	constexpr auto format(const SQObjectType & input, FormatContext & ctx) const -> decltype(ctx.out())
	{
		static const std::unordered_map<SQObjectType, std::string> types = {
			{ OT_NULL,          "OT_NULL"          },
			{ OT_INTEGER,       "OT_INTEGER"       },
			{ OT_FLOAT,         "OT_FLOAT"         },
			{ OT_BOOL,          "OT_BOOL"          },
			{ OT_STRING,        "OT_STRING"        },
			{ OT_TABLE,         "OT_TABLE"         },
			{ OT_ARRAY,         "OT_ARRAY"         },
			{ OT_USERDATA,      "OT_USERDATA"      },
			{ OT_CLOSURE,       "OT_CLOSURE"       },
			{ OT_NATIVECLOSURE, "OT_NATIVECLOSURE" },
			{ OT_GENERATOR,     "OT_GENERATOR"     },
			{ OT_USERPOINTER,   "OT_USERPOINTER"   },
			{ OT_THREAD,        "OT_THREAD"        },
			{ OT_FUNCPROTO,     "OT_FUNCPROTO"     },
			{ OT_CLASS,         "OT_CLASS"         },
			{ OT_INSTANCE,      "OT_INSTANCE"      },
			{ OT_WEAKREF,       "OT_WEAKREF"       },
		};

		if (types.contains(input)) {
			return format_to(ctx.out(), "{}", types.at(input));
		}
		return format_to(ctx.out(), "OT_{:08x}", static_cast<unsigned>(input));
	}
};
