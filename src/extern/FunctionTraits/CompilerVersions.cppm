/////////////////////////////////////////////////////////////////////////////
// LICENSE NOTICE
// --------------
// Copyright (c) Hexadigm Systems
//
// Permission to use this software is granted under the following license:
// https://www.hexadigm.com/GenericLib/License.html
//
// This copyright notice must be included in this and all copies of the
// software as described in the above license.
//
// DESCRIPTION
// -----------
// Module version of "CompilerVersions.h". Simply defers to
// "CompilerVersions.h" to export all public declarations in that header.
// For complete details on module support in "FunctionTraits", see
// https://github.com/HexadigmSystems/FunctionTraits#moduleusage
/////////////////////////////////////////////////////////////////////////////

module;

/////////////////////////////////////////////////////////////
// Let "CompilerVersions.h" just below know we're building
// the "CompilerVersions" module. Following is only #defined
// when we are ...
/////////////////////////////////////////////////////////////
#define STDEXT_BUILDING_MODULE_COMPILERVERSIONS
#include "CompilerVersions.h"
#undef STDEXT_BUILDING_MODULE_COMPILERVERSIONS

export module CompilerVersions;

//////////////////////////////////////////////////////////////////////
// Interface for this module. We simply rely on "using" declarations
// in the code below to export all public declarations from
// "CompilerVersions.h" above. See the following for details on using
// this module:
//
//     https://github.com/HexadigmSystems/FunctionTraits#moduleusage
//
// IMPORTANT:
// ---------
// Note that GCC is currently buggy at this writing (modules still
// under development), and fails to compile the code below (so until
// corrected, this module can't be used in GCC). See the following
// GCC bug reports (both identifying the same issue):
//
//    https://gcc.gnu.org/bugzilla/show_bug.cgi?id=109679
//    https://gcc.gnu.org/bugzilla/show_bug.cgi?id=113129
//////////////////////////////////////////////////////////////////////
export namespace StdExt
{
    using StdExt::tchar;
    using StdExt::tstring_view;
    using StdExt::tcout;
    using StdExt::GetCompilerName;
}