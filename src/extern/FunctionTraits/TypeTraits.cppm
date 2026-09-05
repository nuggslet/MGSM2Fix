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
// Module version of "TypeTraits.h". Simply defers to "TypeTraits.h" to
// export all public declarations in that header, in particular
// "FunctionTraits". Other public declarations unrelated to "FunctionTraits"
// itself are also available in "TypeTraits.h" however and are therefore
// also exported by this module (mostly support declarations that
// "FunctionTraits" itself relies on but sometimes useful in their own
// right). The focus of the module for now however is "FunctionTraits". For
// complete details on module support in "FunctionTraits", see
// https://github.com/HexadigmSystems/FunctionTraits#moduleusage
/////////////////////////////////////////////////////////////////////////////

module;

////////////////////////////////////////////////////////
// Let "TypeTraits.h" just below know we're building
// the "TypeTraits" module. Following is only #defined
// when we are ...
////////////////////////////////////////////////////////
#define STDEXT_BUILDING_MODULE_TYPETRAITS
#include "TypeTraits.h"
#undef STDEXT_BUILDING_MODULE_TYPETRAITS

export module TypeTraits;

////////////////////////////////////////////////////////////////////////
// Export "CompilerVersions" to make the module version of "TypeTraits"
// consistent with the non-module version. In the non-module version, a
// call to #include "TypeTraits.h" automatically #includes
// "CompilerVersions.h" as well, so in the module version, a call to
// "import TypeTraits" automatically imports "CompilerVersions" via the
// following call (so for most intents and purposes it's consistent
// with the behavior of the non-module version). To pick up the macros
// in "CompilerVersions.h" as well however (since macros aren't
// exported by C++ modules), just #include either "TypeTraits.h" or
// "CompilerVersions.h" directly instead (and if so you don't even have
// to directly code your own "import" statement, as each header does
// this for you when the constant STDEXT_USE_MODULES is #defined as it
// normally should be (when using the module version of
// "FunctionTraits"). See the following for complete details:
//
//    https://github.com/HexadigmSystems/FunctionTraits#moduleusage
////////////////////////////////////////////////////////////////////////
export import CompilerVersions;

//////////////////////////////////////////////////////////////////////
// Interface for this module. We simply rely on "using" declarations
// in the code below to export all public declarations from
// "TypeTraits.h" above (internal declarations from "TypeTraits.h"
// not intended for public use are all declared in namespace
// "StdExt::Private" and are not exported below - all others are).
// Note that only declarations associated with "FunctionTraits" are
// documented at https://github.com/HexadigmSystems/FunctionTraits
// however. All other exported declarations below are still available
// for public use however though they're not the focus of the above
// link. The declarations specifically related to "FunctionTraits"
// are (though the "FunctionTraits" declarations rely on the other
// declarations to carry out their work). The upshot is that while
// all other declarations below are undocumented at the above link,
// users who wish to use them for their own purposes may safely do
// so (just consult them in "TypeTraits.h" itself for details, not
// the above link). See the following for details on using this
// module:
// 
//     https://github.com/HexadigmSystems/FunctionTraits#moduleusage
//
// IMPORTANT:
// ---------
// Note that GCC is currently buggy at this writing (modules still under
// development), and fails to compile the code below (so until corrected,
// this module can't be used in GCC). See the following (effectively
// identical) GCC bug reports (now showing the bug has in fact been flagged
// as corrected but an updated version of GCC with the fix hasn't been
// released yet at this writing - it may be before too long though):
//
//    https://gcc.gnu.org/bugzilla/show_bug.cgi?id=109679
//    https://gcc.gnu.org/bugzilla/show_bug.cgi?id=113129
//////////////////////////////////////////////////////////////////////
export namespace StdExt
{
    using StdExt::AlwaysFalse_v;
    using StdExt::AlwaysTrue_v;
    using StdExt::TypeName_v;
    #if defined(USE_CONCEPTS)
        using StdExt::IsClass_c;
    #endif
    using StdExt::IsConstOrVolatile_v;
    using StdExt::IsSpecialization;
    using StdExt::IsSpecialization_v;
    using StdExt::RemoveCvRef_t;
    using StdExt::RemovePtrRef_t;
    using StdExt::ReplaceNthType;
    using StdExt::ReplaceNthType_t;
    using StdExt::IsTuple;
    using StdExt::IsTuple_v;
    #if defined(USE_CONCEPTS)
        using StdExt::Tuple_c;
    #endif
    using StdExt::IsTraitsFunction_v;
    #if defined(USE_CONCEPTS)
        using StdExt::TraitsFunction_c;
    #endif
    using StdExt::CallingConvention;
    using StdExt::CallingConventionCount_v;
    using StdExt::CallingConventionToString;
    using StdExt::CallingConventionReplacedWithCdecl;
    using StdExt::RefQualifier;
    using StdExt::RefQualifierToString;
    using StdExt::FunctionTraits;
    using StdExt::IsFunctionTraits;
    using StdExt::IsFunctionTraits_v;
    #if defined(USE_CONCEPTS)
        using StdExt::FunctionTraits_c;
    #endif

    /////////////////////////////////////////////////////
    // "FunctionTraits" helper templates (read traits)
    // taking a "FunctionTraits" template arg. Rarely
    // used as most will rely on the "FunctionTraits"
    // helper templates taking a function template arg
    // "F" instead (which simply defer to the following)
    /////////////////////////////////////////////////////
    using StdExt::FunctionTraitsArgCount_v;
    using StdExt::FunctionTraitsArgType_t;
    using StdExt::FunctionTraitsArgTypeName_v;
    using StdExt::FunctionTraitsArgTypes_t;
    using StdExt::FunctionTraitsCallingConvention_v;
    using StdExt::FunctionTraitsCallingConventionName_v;
    using StdExt::FunctionTraitsFunctionType_t;
    using StdExt::FunctionTraitsTypeName_v;
    using StdExt::FunctionTraitsIsFreeFunction_v;
    using StdExt::FunctionTraitsIsFunctor_v;
    using StdExt::FunctionTraitsIsMemberFunction_v;
    using StdExt::FunctionTraitsIsMemberFunctionConst_v;
    using StdExt::FunctionTraitsIsMemberFunctionVolatile_v;
    using StdExt::FunctionTraitsIsNoexcept_v;
    using StdExt::FunctionTraitsIsVariadic_v;
    using StdExt::FunctionTraitsIsVoidReturnType_v;
    using StdExt::FunctionTraitsMemberFunctionClass_t;
    using StdExt::FunctionTraitsMemberFunctionClassName_v;
    using StdExt::FunctionTraitsMemberFunctionRefQualifier_v;
    using StdExt::FunctionTraitsMemberFunctionRefQualifierName_v;
    using StdExt::FunctionTraitsReturnType_t;
    using StdExt::FunctionTraitsReturnTypeName_v;
    using StdExt::FunctionTraitsIsEmptyArgList_v;

    /////////////////////////////////////////////////////
    // "FunctionTraits" helper templates (write traits)
    // taking a "FunctionTraits" template arg. Rarely
    // used as most will rely on the "FunctionTraits"
    // helper templates taking a function template arg
    // "F" instead (which simply defer to the following)
    /////////////////////////////////////////////////////
    #if defined(FUNCTION_WRITE_TRAITS_SUPPORTED)
        using StdExt::FunctionTraitsAddVariadicArgs_t;
        using StdExt::FunctionTraitsRemoveVariadicArgs_t;
        using StdExt::FunctionTraitsMemberFunctionAddConst_t;
        using StdExt::FunctionTraitsMemberFunctionRemoveConst_t;
        using StdExt::FunctionTraitsMemberFunctionAddVolatile_t;
        using StdExt::FunctionTraitsMemberFunctionRemoveVolatile_t;
        using StdExt::FunctionTraitsMemberFunctionAddCV_t;
        using StdExt::FunctionTraitsMemberFunctionRemoveCV_t;
        using StdExt::FunctionTraitsMemberFunctionAddLValueReference_t;
        using StdExt::FunctionTraitsMemberFunctionAddRValueReference_t;
        using StdExt::FunctionTraitsMemberFunctionRemoveReference_t;
        using StdExt::FunctionTraitsAddNoexcept_t;
        using StdExt::FunctionTraitsRemoveNoexcept_t;
        using StdExt::FunctionTraitsReplaceCallingConvention_t;
        using StdExt::FunctionTraitsMemberFunctionReplaceClass_t;
        using StdExt::FunctionTraitsReplaceReturnType_t;
        using StdExt::FunctionTraitsReplaceArgs_t;
        using StdExt::FunctionTraitsReplaceArgsTuple_t;
        using StdExt::FunctionTraitsReplaceNthArg_t;
    #endif

    ////////////////////////////////////////////////////
    // "FunctionTraits" helper templates (read traits)
    // taking a function template arg "F". These are
    // the templates most will normally rely on as
    // fully documented here:
    // 
    //   https://github.com/HexadigmSystems/FunctionTraits/#readtraits
    ////////////////////////////////////////////////////
    using StdExt::ArgCount_v;
    using StdExt::ArgType_t;
    using StdExt::ArgTypeName_v;
    using StdExt::ArgTypes_t;
    using StdExt::CallingConvention_v;
    using StdExt::CallingConventionName_v;
    using StdExt::FunctionType_t;
    using StdExt::FunctionTypeName_v;
    using StdExt::IsEmptyArgList_v;
    using StdExt::IsFreeFunction_v;
    using StdExt::IsFunctor_v;
    using StdExt::IsMemberFunction_v;
    using StdExt::IsMemberFunctionConst_v;
    using StdExt::IsMemberFunctionVolatile_v;
    using StdExt::IsNoexcept_v;
    using StdExt::IsVariadic_v;
    using StdExt::IsVoidReturnType_v;
    using StdExt::MemberFunctionClass_t;
    using StdExt::MemberFunctionClassName_v;
    using StdExt::MemberFunctionRefQualifier_v;
    using StdExt::MemberFunctionRefQualifierName_v;
    using StdExt::ReturnType_t;
    using StdExt::ReturnTypeName_v;

    ////////////////////////////////////////////////////
    // "FunctionTraits" helper templates (write traits)
    // taking a function template arg "F". These are
    // the templates most will normally rely on as
    // fully documented here:
    // 
    //   https://github.com/HexadigmSystems/FunctionTraits/#writetraits
    ////////////////////////////////////////////////////
    #if defined(FUNCTION_WRITE_TRAITS_SUPPORTED)
        using StdExt::AddVariadicArgs_t;
        using StdExt::RemoveVariadicArgs_t;
        using StdExt::MemberFunctionAddConst_t;
        using StdExt::MemberFunctionRemoveConst_t;
        using StdExt::MemberFunctionAddVolatile_t;
        using StdExt::MemberFunctionRemoveVolatile_t;
        using StdExt::MemberFunctionAddCV_t;
        using StdExt::MemberFunctionRemoveCV_t;
        using StdExt::MemberFunctionAddLValueReference_t;
        using StdExt::MemberFunctionAddRValueReference_t;
        using StdExt::MemberFunctionRemoveReference_t;
        using StdExt::AddNoexcept_t;
        using StdExt::RemoveNoexcept_t;
        using StdExt::ReplaceCallingConvention_t;
        using StdExt::MemberFunctionReplaceClass_t;
        using StdExt::ReplaceReturnType_t;
        using StdExt::ReplaceArgs_t;
        using StdExt::ReplaceArgsTuple_t;
        using StdExt::ReplaceNthArg_t;
    #endif

    using StdExt::IsForEachFunctor;
    using StdExt::IsForEachFunctor_v;
    #if defined(USE_CONCEPTS)
        using StdExt::ForEachFunctor_c;
    #endif
    using StdExt::ForEach;
    using StdExt::IsForEachTupleFunctor;
    using StdExt::IsForEachTupleFunctor_v;
    #if defined(USE_CONCEPTS)
        using StdExt::ForEachTupleFunctor_c;
    #endif
    using StdExt::ForEachTupleType;
    using StdExt::ForEachFunctionTraitsArg;
    using StdExt::ForEachArg;
}