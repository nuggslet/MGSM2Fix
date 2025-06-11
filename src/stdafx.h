#pragma once

#include "safetyhook.hpp"
#include "TypeTraits.h"
using namespace StdExt;
#include "Zydis.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <cassert>
#include <cstdint>
#include <cstring>
#include <cinttypes>
#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <string_view>
#include <filesystem>
#include <vector>
#include <array>
#include <stack>
#include <utility>
#include <numeric>
#include <optional>
#include <variant>
#include <any>
#include <map>
#include <unordered_map>
#include <set>
#include <unordered_set>
#include <mutex>

#undef Yield
#undef GetObject
#undef LoadImage

#include "resource.h"

#include "sqpcheader.h"
#include "sqcompiler.h"
#include "sqvm.h"
#include "sqarray.h"
#include "sqtable.h"
#include "sqclass.h"
#include "sqclosure.h"
#include "sqfuncproto.h"
#include "squserdata.h"
#include "sqstring.h"
#include "sqstdstring.h"

#include "sqrat.h"

#include "inipp.h"
#include "spdlog.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/stdout_sinks.h"
#include "spdlog/sinks/wincolor_sink.h"
