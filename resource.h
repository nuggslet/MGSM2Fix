#pragma once
// ReSharper disable CppClangTidyModernizeMacroToEnum

// Core name & version
#define FIX_NAME          "MGSM2Fix"
#define PRIMARY_REPO_URL  "https://github.com/nuggslet/MGSM2Fix"
#define FALLBACK_REPO_URL "https://github.com/h-i-d-e-o/MGSM2Fix"

#define VERSION_MAJOR     3
#define VERSION_MINOR     2
#define VERSION_PATCH     1

#define STRINGIFY_HELPER(x) #x
#define STRINGIFY(x) STRINGIFY_HELPER(x)
#ifdef VERSION_LABEL
#define VERSION_STRING STRINGIFY(VERSION_MAJOR) "." STRINGIFY(VERSION_MINOR) "." STRINGIFY(VERSION_PATCH) "-" VERSION_LABEL
#else
#define VERSION_STRING STRINGIFY(VERSION_MAJOR) "." STRINGIFY(VERSION_MINOR) "." STRINGIFY(VERSION_PATCH)
#endif

// Metadata
#define COMPANY_NAME      "nuggslet"
#define PRODUCT_NAME      FIX_NAME
#define FILE_DESCRIPTION  FIX_NAME " ASI Plugin"
#define INTERNAL_NAME     FIX_NAME ".asi"
#define ORIGINAL_FILENAME FIX_NAME ".asi"
#define PRODUCT_VERSION   VERSION_STRING
#define FILE_VERSION      VERSION_STRING
#define LEGAL_COPYRIGHT   "(C) 2025 nuggslet. Licensed under the MIT License."
#define LEGAL_TRADEMARKS  ""
#define COMMENTS          ""


//{{NO_DEPENDENCIES}}
// Microsoft Visual C++ generated include file.
// Used by MGSM2Fix.rc
//
#define IDR_NUT1                        105
#define IDR_HLSL1                       106
#define IDR_HLSL2                       107

// Next default values for new objects
// 
#ifdef APSTUDIO_INVOKED
#ifndef APSTUDIO_READONLY_SYMBOLS
#define _APS_NEXT_RESOURCE_VALUE        108
#define _APS_NEXT_COMMAND_VALUE         40001
#define _APS_NEXT_CONTROL_VALUE         1001
#define _APS_NEXT_SYMED_VALUE           101
#endif
#endif
