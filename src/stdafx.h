#pragma once

#define WIN32_LEAN_AND_MEAN

#include <cassert>
#include <Windows.h>
#include <Shlobj.h>
#include <cstdint>
#include <sstream>
#include <fstream>
#include <string>
#include <iostream>
#include <inttypes.h>
#include <mutex>

#include "external/loguru/loguru.hpp"
#include "external/inipp/inipp/inipp.h"
#include "external/length-disassembler/headerOnly/ldisasm.h"

#undef Yield
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
