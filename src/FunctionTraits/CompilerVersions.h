#ifndef COMPILER_VERSIONS
#define COMPILER_VERSIONS

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
// Contains miscellaneous #defined constants to identify the target compiler
// in effect (see "Compiler Identification Macros" below), the version of
// C++ in effect (see "C++ Version Macros" below), and a few other
// miscellaneous compiler-related declarations. Should typically be
// #included first in all other files so they're immediately available if
// you need to test these #defined constants for anything in those files (so
// while #including it first isn't mandatory, it's usually a good idea so
// they're immediately ready for use).
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Compiler Identification Macros
// ------------------------------
// The following #ifs are used to define one and only one of the following
// #defined constants in order to indicate which compiler is currently in
// use. Only one of the following will therefore ever be #defined (they are
// all mutually exclusive):
//
//   GCC_COMPILER
//   MICROSOFT_COMPILER
//   CLANG_COMPILER
//   INTEL_COMPILER
//
// Note that we rely on the following predefined (compiler-specific) constants
// to determine the above (respectively):
//
//   __GNUC__
//   _MSC_VER
//   __clang__
//   __INTEL_LLVM_COMPILER
//
// The reason for having to #define our own constants rather than just
// rely on the native predefined compiler constants just above (though our own
// constants are determined by them), is because the predefined constants are
// sometimes #defined by other compilers as well. This makes it impossible to
// tell which compiler is actually running based on checks of these predefined
// constants alone. For instance, both the Clang compiler and Intel oneAPI
// DPC++/C compiler #define __clang__ (since the Intel compiler is based on
// Clang), so you can't rely on __clang__ to determine if the real Clang
// compiler is actually running and not the Intel compiler, assuming you need to
// know this (depending on what you're checking __clang__ for). Similarly, both
// Clang and Intel also #define _MSC_VER when running in Microsoft VC++
// compatibility mode (i.e., when compiling code for Windows), so if _MSC_VER is
// #defined then you don't know if it's actually the native (Microsoft) VC++
// compiler running or possibly Clang or Intel running in Microsoft VC++
// compatibility mode (such as when you install Clang and/or Intel on Windows
// and then select one of them to compile your app, overriding the native
// Microsoft VC++ compiler - they can also be run from other OSs to explicitly
// compile Windows apps).
//
// The following #ifs therefore #define our own constants (the first group
// above), for use when you need to know the real compiler that's actually
// in use. The predefined constants above (the native constants for each
// compiler), can still be applied however whenever the actual, real
// compiler doesn't matter (for whatever your purpose is). For instance, you
// would check _MSC_VER instead of MICROSOFT_COMPILER when the actual (real)
// Microsoft VC++ compiler doesn't matter, i.e., all that matters is that a
// compiler that's compatible with VC++ is running, namely Clang or Intel
// running in Microsoft VC++ compatibility mode (so for all intents and
// purposes if _MSC_VER is #defined then you can proceed as if the real VC++
// compiler is running - if another like Clang or Intel is actually running
// then because they've #defined _MSC_VER it means they're running in
// Microsoft compatibility VC++ mode so their behavior should be the same
// as VC++ - caution advised however as you should make sure the behavior
// is in fact the same depending on what you're doing, since it's not always
// 100% compatible). If you need to know if the real Microsoft VC++ compiler
// is running however then you would simply check MICROSOFT_COMPILER instead.
//
// IMPORTANT:
// ---------
// The order we check compilers in the following code is relevant so don't
// change it if you're not sure what you're doing. For instance, we check
// the Intel compiler first since it normally #defines __clang__ as well
// (since the Intel oneAPI DPC++/C++ compiler is based on Clang), but the
// Clang compiler never #defines __INTEL_LLVM_COMPILER so Intel needs to be
// checked first (otherwise, if __clang__ is #defined and you check it first
// then you can't tell if it's the real Clang or Intel that #defined it).
/////////////////////////////////////////////////////////////////////////////
#if defined(__INTEL_LLVM_COMPILER) // See https://www.intel.com/content/www/us/en/docs/dpcpp-cpp-compiler/developer-guide-reference/2023-2/use-predefined-macros-to-specify-intel-compilers.html
    #define INTEL_COMPILER __INTEL_LLVM_COMPILER
#elif defined(__clang__) // See https://clang.llvm.org/docs/LanguageExtensions.html#builtin-macros
    #define CLANG_COMPILER __clang__
#elif defined(__GNUC__) // See: https://gcc.gnu.org/onlinedocs/cpp/macros/predefined-macros.html#c.__GNUC__
                        //      https://gcc.gnu.org/onlinedocs/cpp/Common-Predefined-Macros.html#Common-Predefined-Macros
                        //      https://stackoverflow.com/a/55926503
    #define GCC_COMPILER __GNUC__
#elif defined(_MSC_VER) // See: https://learn.microsoft.com/en-us/cpp/preprocessor/predefined-macros?view=msvc-170
                        //      https://devblogs.microsoft.com/cppblog/visual-c-compiler-version/
                        // Note: __INTEL_COMPILER identifies the classic Intel compiler which
                        //       is now deprecated by Intel (so we longer support it)
    #define MICROSOFT_COMPILER _MSC_VER
#else
    #error "Unsupported compiler (GCC, Microsoft, Clang and Intel DPC++/C++ are the only ones supported at this writing)"
#endif

//////////////////////////////////////////////////////////////////
// C++ Version Macros
// ------------------
// The code that follows below #defines our C++ version constants
// (macros) which are all of the form CPPXX_OR_LATER,
// CPPXX_OR_EARLIER and CPPXX, where "XX" is the C++ release year
// (C++14, C++17, C++20, etc.). One such constant is #defined for
// each C++ release year starting at C++14 (read on for C++11).
// These can be used to determine which version of C++ is in
// effect (currently running). See examples just below (WYSIWYG).
// Note that we always assume C++11 or greater since we don't
// support versions prior to that. Only CPP11 is therefore
// #defined for that year, not CPP11_OR_LATER nor
// CPP11_OR_EARLIER, since the former is always assumed to be
// true so no need to ever explicitly test it (C++11 or later is
// always assumed), and the latter also never needs to be
// explicitly tested (since no version earlier than C++11 is ever
// assumed), leaving only the need for #defined constant CPP11 to
// specifically test for that version (if required but in reality
// most users will be using much later versions by now). Lastly,
// note that new constants should be added for each new release
// of C++ (CPPXX_OR_LATER, CPPXX_OR_EARLIER and CPPXX).
//
//     Example Usage (for C++17)
//     -------------------------
// 
//     #if CPP17_OR_LATER
//          // Code that targets C++17 or later
//     #else
//          // Must be C++14 or earlier (C++14 precedes C++17)
//     #endif
// 
//     #if CPP17_OR_EARLIER
//          // Code that targets C++17 or earlier
//     #else
//          // Must be C++20 or later (C++20 succeeds C++17)
//     #endif
//
//     #if CPP17
//          // Code that specifically targets C++17 only
//     #else
//          // Any other version of C++ besides C++17
//     #endif
//////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////
// MSFT? (_MSC_VER #defined, but also includes any
// other compiler we support running in Microsoft VC++
// compatibility mode). Note that _MSVC_LANG is only
// #defined however beginning in Visual Studio 2015
// Update 3 (see predefined macros in MSDN). If it's
// #defined then it's guaranteed to be 201402L or
// greater (i.e., C++14 or greater). Again, see this in
// MSDN (it's the same value as the __cplusplus macro).
////////////////////////////////////////////////////////
#if defined(_MSC_VER) && defined(_MSVC_LANG)
	#define CPP_VERSION _MSVC_LANG // For internal use only

/////////////////////////////////////////////////////////
// In MSFT (opposed to other compilers like GCC, Clang,
// etc., where __cplusplus is always assumed to be
// accurately #defined), __cplusplus is only accurately
// #defined starting in VS 2017 15.7 Preview 3, but only
// if the /Zc:__cplusplus switch is set. See:
//
//     MSVC now correctly reports __cplusplus
//     https://devblogs.microsoft.com/cppblog/msvc-now-correctly-reports-__cplusplus/
//
// It's always 199711L otherwise (erroneously indicating
// C++98). Since we already tested for the presence of
// _MSVC_LANG just above however, which is #defined
// starting in Visual Studio 2015 Update 3 (as noted
// above), then if we drop into the following #elif it
// means we must be targeting an earlier version of VC++
// (prior to Visual Studio 2015 Update 3), so
// __cplusplus is guaranteed to be 199711L at this
// point, as just described (again, erroneously
// indicating C++98). That's fine for our purposes
// however, since all the CPPXX_OR_LATER constants
// #defined below will be set to 0 in this case, and all
// CPPXX_OR_EARLIER constants will be set to 1. Since
// "XX" is always >= 14 (CPPXX_OR_LATER and
// CPPXX_OR_EARLIER are only #defined for XX >= 14, no
// such constants exists for XX=11), only C++11 should
// therefore be considered in effect by default (CPP11
// will be set to 1), not any later versions nor earlier
// versions (all CPPXX_OR_LATER constants will be set to
// 0, and all CPPXX_OR_EARLIER constants will be set to
// 1). Note that C++98 is the only other official
// earlier version though and __cplusplus actually
// targets it in this case (because it's always 199711L
// as described), but that value is erroneous since MSFT
// didn't start setting it until VS 2017 15.7 Preview 3
// (as described above), so we just ignore the value and
// always implicitly assume C++11 instead (since all
// modern MSFT compilers now support it - we don't
// support C++98 since it's ancient now, and even C++11
// is likely rarely used anymore).
/////////////////////////////////////////////////////////
#elif defined(__cplusplus)
    #define CPP_VERSION __cplusplus // For internal use only
#else
    #define CPP_VERSION 0L // For internal use only (if we arrive here then the #defined
                           // constants that follow below will now target C++11 - we don't
                           // support anything earlier, namely C++98)
#endif

//////////////////////////////////////////////////////////
// Next version of C++ after C++26? This version is still
// unknown at this writing but the version # we're
// testing here (202700L) will realistically work for our
// purposes (to see if we're targeting the next version
// after C++26). Presumably it will be 202700L or greater,
// and C++26, which hasn't been released yet at this
// writing, will presumably be "2026??" for some "??".
// We'll properly update this however once the version
// that succeeds C++26 is officially released (so
// although the version we're testing here is still
// potentially brittle, it's realistically guaranteed to
// work).
//////////////////////////////////////////////////////////
#if CPP_VERSION >= 202700L
    // #define CPP?? 1 // TBD (set "??" to the next version after C++26)
    #define CPP26 0
    #define CPP23 0
    #define CPP20 0
    #define CPP17 0
    #define CPP14 0
    #define CPP11 0
    // #define CPP??_OR_LATER 1 // TBD (set "??" to the next version after C++26)
    #define CPP26_OR_LATER 1
    #define CPP23_OR_LATER 1
    #define CPP20_OR_LATER 1
    #define CPP17_OR_LATER 1
    #define CPP14_OR_LATER 1

// C++26 (testing for > C++23 here so if true then we must be dealing with C++26 at this point)
#elif CPP_VERSION > 202302L
    //#define CPP?? 0 // TBD (set "??" to the next version after C++26)
    #define CPP26 1
    #define CPP23 0
    #define CPP20 0
    #define CPP17 0
    #define CPP14 0
    #define CPP11 0
    // #define CPP??_OR_LATER 0 // TBD (set "??" to the next version after C++26)
    #define CPP26_OR_LATER 1
    #define CPP23_OR_LATER 1
    #define CPP20_OR_LATER 1
    #define CPP17_OR_LATER 1
    #define CPP14_OR_LATER 1

// C++23 (testing for > C++20 here so if true then we must be dealing with C++23 at this point)
#elif CPP_VERSION > 202002L
    //#define CPP?? 0 // TBD (set "??" to the next version after C++26)
    #define CPP26 0
    #define CPP23 1
    #define CPP20 0
    #define CPP17 0
    #define CPP14 0
    #define CPP11 0
    // #define CPP??_OR_LATER 0 // TBD (set "??" to the next version after C++26)
    #define CPP26_OR_LATER 0
    #define CPP23_OR_LATER 1
    #define CPP20_OR_LATER 1
    #define CPP17_OR_LATER 1
    #define CPP14_OR_LATER 1

// C++20 (testing for > C++17 here so if true then we must be dealing with C++20 at this point)
#elif CPP_VERSION > 201703L
    //#define CPP?? 0 // TBD (set "??" to the next version after C++26)
    #define CPP26 0
    #define CPP23 0
    #define CPP20 1
    #define CPP17 0
    #define CPP14 0
    #define CPP11 0
    // #define CPP??_OR_LATER 0 // TBD (set "??" to the next version after C++26)
    #define CPP26_OR_LATER 0
    #define CPP23_OR_LATER 0
    #define CPP20_OR_LATER 1
    #define CPP17_OR_LATER 1
    #define CPP14_OR_LATER 1

// C++17 (testing for > C++14 here so if true then we must be dealing with C++17 at this point)
#elif CPP_VERSION > 201402L
    //#define CPP?? 0 // TBD (set "??" to the next version after C++26)
    #define CPP26 0
    #define CPP23 0
    #define CPP20 0
    #define CPP17 1
    #define CPP14 0
    #define CPP11 0
    // #define CPP??_OR_LATER 0 // TBD (set "??" to the next version after C++26)
    #define CPP26_OR_LATER 0
    #define CPP23_OR_LATER 0
    #define CPP20_OR_LATER 0
    #define CPP17_OR_LATER 1
    #define CPP14_OR_LATER 1

// C++14 (testing for this exact version)
#elif CPP_VERSION == 201402L
    //#define CPP?? 0 // TBD (set "??" to the next version after C++26)
    #define CPP26 0
    #define CPP23 0
    #define CPP20 0
    #define CPP17 0
    #define CPP14 1
    #define CPP11 0
    // #define CPP??_OR_LATER 0 // TBD (set "??" to the next version after C++26)
    #define CPP26_OR_LATER 0
    #define CPP23_OR_LATER 0
    #define CPP20_OR_LATER 0
    #define CPP17_OR_LATER 0
    #define CPP14_OR_LATER 1

// C++11 (testing for < C++14 here so we implicitly assume C++11 - only other (official) earlier version is C++98 which we don't support)
#else
    //#define CPP?? 0 // TBD (set "??" to the next version after C++26)
    #define CPP26 0
    #define CPP23 0
    #define CPP20 0
    #define CPP17 0
    #define CPP14 0
    #define CPP11 1 // Implicitly assumed at this point (we don't support C++98, the only official earlier version)
    // #define CPP??_OR_LATER 0 // TBD (set "??" to the next version after C++26)
    #define CPP26_OR_LATER 0
    #define CPP23_OR_LATER 0
    #define CPP20_OR_LATER 0
    #define CPP17_OR_LATER 0
    #define CPP14_OR_LATER 0
#endif

#undef CPP_VERSION // Done with this

//////////////////////////////////////////////////////////////////
// CPPXX_OR_EARLIER (#defined) constants that can be tested
// instead of the CPPXX_OR_LATER #defined constants above. You
// can use either the CPPXX_OR_LATER and/or CPPXX_OR_EARLIER
// constants based on your needs (to determine which version of
// C++ is in effect, C++14, C++17, C++20, etc.). We always assume
// C++11 or greater however so no specific constant exists to
// target earlier versions (C++98 the only official earlier
// version). Note that a new CPPXX_OR_EARLIER constant should be
// added for each new release of C++
//
// Example usage:
//
//     #if CPP17_OR_EARLIER
//          // Code that targets C++17 or earlier
//     #else
//          // Code that targets C++20 or greater (C++20 succeeds C++17)
//     #endif
//////////////////////////////////////////////////////////////////
//#define CPP26_OR_EARLIER (!CPP??_OR_LATER) // TBD (set "??" to the next version after C++26)
#define CPP23_OR_EARLIER (!CPP26_OR_LATER)
#define CPP20_OR_EARLIER (!CPP23_OR_LATER)
#define CPP17_OR_EARLIER (!CPP20_OR_LATER)
#define CPP14_OR_EARLIER (!CPP17_OR_LATER)

///////////////////////////////////////////////////
// Sanity check only. Should always be false (for
// internal use only - #defined further below if
// required)
///////////////////////////////////////////////////
#if defined(DECLARE_MACROS_ONLY)
    #error "DECLARE_MACROS_ONLY already #defined (for internal use only so never should be)"
#endif

#if CPP20_OR_LATER
    /////////////////////////////////////////////////////////////////////////
    // Pick up standard library feature-test macros from <version> (C++20 or
    // later). See the following 2 links (which mostly duplicate each other
    // though 1st link has more info but 2nd link explains the "Value"
    // column in the "Notes" section at the bottom - it also links back to
    // the 1st link):
    //     https://en.cppreference.com/w/cpp/feature_test#Library_features
    //     https://en.cppreference.com/w/cpp/utility/feature_test
    // 
    // Note that there are also predefined language feature-test macros
    // (opposed to "library" feature-test macros above) so <version> not
    // required (macros are always predefined). See:
    //     https://en.cppreference.com/w/cpp/feature_test#Language_features
    // 
    // See here for "__has_include" usage example (2nd link from GCC docs):
    //     https://en.cppreference.com/w/cpp/feature_test#Example
    //     https://gcc.gnu.org/onlinedocs/cpp/_005f_005fhas_005finclude.html
    /////////////////////////////////////////////////////////////////////////
    #if defined(__has_include) // C++17 feature so should always be true at this point
                               // normally (since we tested for C++20 or later just above)

        // <version> available in C++20 or later (tested for above so normally true)
        #if __has_include(<version>)
            #include <version>
        #endif
    #endif

    ////////////////////////////////////////////////////////////////
    // "import std" supported? (C++23 feature). See:
    //
    //    Modules "std" and "std.compat" (official C++23 document)
    //    https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2022/p2465r3.pdf
    //
    //    "__cpp_lib_modules" mentioned in above link (but read on for details)
    //    https://en.cppreference.com/w/cpp/feature_test#cpp_lib_modules
    //
    //    MSFT specific but occasionally provides some standard insight
    //    https://www.youtube.com/watch?v=Dk_C_E8AtRs&t=1331s
    //
    // Note that at this writing the C++ feature macro
    // __cpp_lib_modules (used to indicate that "import std" and
    // "import std.compat" are supported as of C++23) may not be
    // #defined due to (still) evolving compiler support (still in
    // a state of flux at this writing). Moreover, even if it is
    // #defined such as in the latest editions of MSVC, we can't
    // safely rely on it yet without the user's permission (at this
    // writing). It may still be experimental that is and in fact,
    // even though __cpp_lib_modules will be #defined on MSVC at
    // this writing (in recent versions of Visual Studio 2022),
    // MSVC doesn't yet support mixing "import std" and headers
    // from the "std" library (at least in the same file). This is
    // officially documented by MSFT here:
    //
    //    https://learn.microsoft.com/en-us/cpp/cpp/tutorial-import-stl-named-module?view=msvc-170#standard-library-named-module-considerations
    //
    // However, according to Stephan Lavajej of MSFT in the
    // following video (first link goes straight to the issue):
    //
    //      https://www.youtube.com/watch?v=Dk_C_E8AtRs&t=2181s // This specific issue (at 36:21 in the video)
    //      https://www.youtube.com/watch?v=Dk_C_E8AtRs&t=1331s // Full description of "import std" and "import std.compat" in general (at 22:11 in the video)
    //      https://www.youtube.com/watch?v=Dk_C_E8AtRs // Start of video (describes C++23 features in general)
    //
    // He indicates (in the 1st link) that mixing "import std"
    // and "std" headers from the "std" library is not yet
    // supported, but confirms it eventually will be (since the
    // standard itself supports it). For now then we can't just
    // implement an "import std" statement in the following code by
    // checking if __cpp_lib_modules is #defined since our use of
    // "import std" will still interfere with any "std" headers the
    // user's own project may be #including (if the user's project
    // doesn't rely on "import std" itself yet). However, if the
    // user's project does rely on "import std" then they can
    // simply grant permission for us to use it as well (by
    // #defining STDEXT_IMPORT_STD_EXPERIMENTAL which we rely on in
    // the following call). We'll likely remove
    // STDEXT_IMPORT_STD_EXPERIMENTAL in a later release however
    // and rely solely on __cpp_lib_modules instead, once "import
    // std" is fully supported by all target compilers (so issues
    // like mixing "import std" with the "std" headers in MSVC no
    // longer a problem). For now however,
    // STDEXT_IMPORT_STD_EXPERIMENTAL can #defined by users who
    // wish us to rely on "import std" instead of us #including
    // individual headers (dependencies) from the "std" library in
    // the usual C++ way. If STDEXT_IMPORT_STD_EXPERIMENTAL is
    // #defined however then it's assumed both the target compiler
    // AND the user's own project supports "import std" (i.e., it's
    // assumed the compiler itself is set up to handle it which can
    // vary among different compilers - it may not be available
    // out-of-the-box for instance depending on the compiler, and
    // the user's own project must also support it, so in MSVC
    // projects for instance it's assumed the user is also using
    // "import std" everywhere to prevent our own call to "import
    // std" below from conflicting with any #include statements
    // from the "std" library in the user's project - again, mixing
    // the two isn't currently supported by MSVC). It's the user's
    // responsibility to ensure this or compilation will likely
    // fail, usually with cryptic compiler errors. Note that the
    // following ignores STDEXT_IMPORT_STD_EXPERIMENTAL however if
    // C++23 or later isn't even targeted (via the check for
    // CPP23_OR_LATER seen below). STDEXT_IMPORT_STD_EXPERIMENTAL
    // normally implies C++23 or later however but we check for
    // CPP23_OR_LATER anyway to play it safe (in addition to the
    // check for version 19.35 when targeting MSVC as seen, which
    // is when this feature became available in MSVC, and the check
    // for STDEXT_IMPORTED_STD as well, which prevents redundant
    // calls to "import std" if it's already been previously called
    // - note that redundant "import" statements are harmless
    // though but STDEXT_IMPORTED_STD won't normally be #defined
    // at this point anyway - checking it for future use only).
    ////////////////////////////////////////////////////////////////
    #if CPP23_OR_LATER && \
        defined(STDEXT_IMPORT_STD_EXPERIMENTAL) && \
        (!defined(MICROSOFT_COMPILER) || _MSC_VER >= 1935) && \
        !defined(STDEXT_IMPORTED_STD)
            import std;
            #define STDEXT_IMPORTED_STD
    #endif

    //////////////////////////////////////////////////////////////////
    // USE_CONCEPTS. #defined constant to control whether concepts
    // should be used or not wherever required in all remaining code.
    // Concepts only became available in C++20 so by default we
    // #define USE_CONCEPTS in C++20 or later (assuming the C++
    // feature test macro __cpp_lib_concepts is also #defined which
    // we test for here - normally should test true in C++20).
    // USE_CONCEPTS is #undefined in C++17 and earlier so we
    // therefore rely on "static_assert" instead of concepts. If you
    // prefer to always rely on "static_assert" even in C++20 and
    // later however then you can just #undef it below. This may even
    // be preferable since I often find "static_assert" messages more
    // visible and clear than concept messages (in some situations
    // anyway). In any case, it's your responsibility to explicitly
    // test if USE_CONCEPTS is #defined and apply concepts if so or
    // "static_assert" otherwise (or you can just explicitly check
    // the feature-test macro __cpp_lib_concepts instead but unlike
    // the latter, USE_CONCEPTS can be turned off by #undefining it
    // here - you wouldn't want to do that for __cpp_lib_concepts).
    // One way to do this (rely on USE_CONCEPTS) that we consistently
    // rely on ourselves is as per the following example (though you
    // need not follow this model but it works well IMHO).
    //
    // Let's say you want a concept to ensure some type is "const"
    // using "std::is_const". You could therefore declare a concept
    // like so (note that I've applied the suffix "_c" to the concept
    // name as a convention to indicate it's a concept but it's up to
    // you if you want to follow this convention):
    //
    //     #if defined(USE_CONCEPTS)
    //         template <typename T>
    //         concept IsConst_c = std::is_const_v<T>;
    //
    //         #define IS_CONST_C StdExt::IsConst_c // Assuming the "StdExt" namespace for this example
    //     #else
    //         #define STATIC_ASSERT_IS_CONST(T) static_assert(std::is_const_v<T>, \
    //                                                         "\"" #T "\" must be const")
    //         #define IS_CONST_C typename
    //     #endif
    //
    // Then in any code that requires this concept you can simply do
    // this:
    //
    //     template<IS_CONST_C T>
    //     struct Whatever
    //     {
    //         #if !defined(USE_CONCEPTS)
    //             ///////////////////////////////////////////////
    //             // Kicks in if concepts are NOT supported,
    //             // otherwise IS_CONST_C concept kicks in just
    //             // above instead (latter simply resolves to
    //             // the "typename" keyword when concepts aren't
    //             // supported)
    //             ///////////////////////////////////////////////
    //             STATIC_ASSERT_IS_CONST(T);
    //         #endif
    //
    //         // ...
    //     }
    //
    // Template arg "T" in the above struct therefore utilizes the
    // IS_CONST_C concept if concepts are supported but if not, then
    // IS_CONST_C simply resolves to the "typename" keyword instead,
    // and the STATIC_ASSERT_IS_CONST macro then kicks in to do a
    // "static_assert". You can therefore apply the above technique
    // to all your concepts and we do throughout all remaining code
    // where applicable.
    //////////////////////////////////////////////////////////////////
    #if defined(__cpp_lib_concepts) // See https://en.cppreference.com/w/cpp/feature_test#cpp_lib_concepts
                                    // There's also cpp_concepts: https://en.cppreference.com/w/cpp/feature_test#cpp_concepts
        #define USE_CONCEPTS
    #endif

    /////////////////////////////////////////////////////////////////////////////
    // If both conditions we're testing here are true then this header is now
    // being #included by a client in the module version of the header (the
    // module named "CompilerVersions" in "CompilerVersions.cppm" or whatever
    // the user may have renamed the latter file's extension to). In this case
    // we know modules (a C++20 feature) are supported (we already tested
    // CPP20_OR_LATER above), but we'll also add a check for the official C++
    // feature constant "__cpp_modules" in a later release since it's not yet
    // supported by all compilers (we'll add that check to the following). The
    // 2nd condition just below then checks if STDEXT_USE_MODULES was #defined
    // by the user themself, indicating they've added the ".cppm" files to their
    // project (in this case the "CompilerVersions.cppm" module) and wish to use
    // it. In that case we'll go on to declare only #defined (public) macros in
    // this header instead of all other C++ declarations that are also normally
    // declare (when STDEXT_USE_MODULES isn't #defined), since those C++
    // declarations will now originate from the module itself (which we import
    // via "import CompilerVersions" in the code just below, though the user
    // might also do this themself). Therefore, when a user #includes this
    // header in the module version, only the macros in the header will need to
    // be declared since C++ modules don't export #defined macros (we #define an
    // internal constant DECLARE_MACROS_ONLY below to facilitate this). If the
    // user hasn't #defined STDEXT_USE_MODULES however then they wish to use
    // this header normally (declare everything as usual), even if they've also
    // added the module to their project as well (which will still be
    // successfully compiled), though that would be very unlikely (why add the
    // module if they're not going to use it? - if they've added it then they'll
    // normally #define STDEXT_USE_MODULES as well). As for the 2nd condition
    // we're testing here, STDEXT_BUILDING_MODULE_COMPILERVERSIONS (an internal
    // constant), this is #defined by the module itself (CompilerVersions.cppm)
    // before #including this header in its global fragment section. The module
    // then simply exports all public declarations from this header in its
    // purview section via "using" declarations.
    // STDEXT_BUILDING_MODULE_COMPILERVERSIONS is therefore #defined only if the
    // module itself is now being built. If #defined then it must be the module
    // itself now #including us and if not #defined then it must be a client of
    // the module #including us instead (which is what the following tests for
    // in its 2nd condition - if both conditions are true then it must be a user
    // #including us, not the module itself, so we only declare the #defined
    // macros in this header instead of all other declarations, as described
    // above).
    /////////////////////////////////////////////////////////////////////////////
    #if defined(STDEXT_USE_MODULES) && !defined(STDEXT_BUILDING_MODULE_COMPILERVERSIONS)
        //////////////////////////////////////////////////////
        // Importing the "CompilerVersions" module as a
        // convenience to module clients. This way clients
        // can simply #include this header without having to
        // explicitly import the "CompilerVersions" module
        // themselves (since this header does it for them).
        // It's harmless though if they've already imported
        // the "CompilerVersions" module on their own (which
        // is more natural anyway - relying on this header
        // may even confuse some users who might not realize
        // that a "#include CompilerVersions.h" statement is
        // actually importing the "CompilerVersions" module
        // to pick up all public declarations in the header
        // instead of declaring them in the header itself -
        // this is how the header behaves when
        // STDEXT_USE_MODULES is #defined). #including this
        // header however will also pick up all public macros
        // in the header since modules don't export macros
        // (so if clients simply import the
        // "CompilerVersions" module and don't #include this
        // header, they'll have access to all exported
        // declarations in the module which originate from
        // this header, but none of the macros in the header
        // - fine if they don't use any of them though).
        //////////////////////////////////////////////////////
        import CompilerVersions;

        //////////////////////////////////////////////////////
        // All declarations in this module are now available
        // via the import statement just above. Only macros
        // will be missing since modules don't export them
        // (so the above import statement doesn't include
        // them). The following will therefore ensure that
        // only (public) macros will be #defined in the
        // remaining code that follows. All other
        // declarations are preprocessed out because they're
        // already available via the import statement just
        // above.
        //////////////////////////////////////////////////////
        #define DECLARE_MACROS_ONLY
    #endif
#endif // #if CPP20_OR_LATER

#if !defined(DECLARE_MACROS_ONLY)
    // "import std" not currently in effect? (C++23 or later)
    #if !defined(STDEXT_IMPORTED_STD)
        // Standard C/C++ headers
        #include <iostream> // For declaring "tcout" later on (as
                            // an inline variable if C++17 or later
                            // or a macro otherwise - don't really
                            // need to #include in the macro case
                            // but we do anyway as a convenience
                            // to users)

        //////////////////////////////////////////////////////////////
        // Go on to #include <string_view> if available, a C++17
        // feature so we check for that first (using our own
        // CPP17_OR_LATER constant #defined earlier). Note that for
        // our purposes and likely those of most users, if C++17 or
        // later is detected, then it's assumed that <string_view> is
        // also available even though it might not be (due to
        // compliancy issues with a given compiler, in particular
        // early versions of C++17, where <string_view> may not have
        // been available in some compilers yet). For most users who
        // #include "CompilerVersions.h" however, it's just easier to
        // test CPP17_OR_LATER and if true, assume that <string_view>
        // is always available, otherwise they'd have to check if
        // "__has_include" is both #defined (it might not be in early
        // versions of C++17), AND returns true for <string_view>
        // (also not guaranteed). In very rare cases <string_view>
        // might even be available even if "__has_include" isn't
        // #defined in which case there's no standard way to test if
        // <string_view> is available or not.
        //
        // To deal with the hassle and inconvenience of this
        // situation, the following code goes on to check if
        // "__has_include" is both #defined AND returns true for
        // <string_view>, aborting compilation with an #error message
        // if both don't hold (meaning we're dealing with a
        // non-compliant version of C++17 or later). If compilation
        // isn't aborted (normally), then CPP17_OR_LATER can be
        // checked by all users from here on and if true, it
        // guarantees that <string_view> is available (no need to
        // check "__has_include" anymore). Very few users should take
        // issue with this approach, i.e., aborting compilation if
        // CPP17_OR_LATER is detected but "__has_include" either
        // isn't #defined or returns false for <string_view>. Neither
        // should normally happen in versions of C++17 or later that
        // conform with the C++ standard (unless they're early
        // versions of C++17 or perhaps some special compiler
        // switch/setting is in effect). We just abort compilation in
        // this case and won't worry about the rare user that might
        // take issue with the situation (the design is intended to
        // make life easier for the vast majority of users instead).
        //////////////////////////////////////////////////////////////
        #if CPP17_OR_LATER
            //////////////////////////////////////////////////////////
            // "__has_include" is a C++17 feature so should normally
            // be #defined at this point (from C++17 check just
            // above). If not then we're not dealing with a compliant
            // C++17 compiler so we abort compilation as described
            // above (would be very rare though these days). Note
            // that in C++20 and later the feature testing constant
            // "__cpp_lib_string_view" could also be checked as an
            // alternative (Google this for details) but the
            // following does the job.
            //////////////////////////////////////////////////////////
            #if defined(__has_include)
                ///////////////////////////////////////////////////////
                // <string_view> is a C++17 header so should normally
                // be available at this point as well (again, from
                // C++17 check above). If not then again, we're not
                // dealing with a compliant C++17 compiler so we abort
                // compilation as described above (would be very rare
                // though these days).
                ///////////////////////////////////////////////////////
                #if __has_include(<string_view>)
                    #include <string_view>
                #else
                    #error "C++17 or later was detected but <string_view> isn't available (a C++17 feature so it normally should be). The target compiler isn't compliant with C++17 and therefore not supported (see code comments for further details)"
                #endif
            #else
                #error "C++17 or later was detected but '__has_include' isn't #defined (a C++17 feature so it normally should be). The target compiler isn't compliant with C++17 and therefore not supported (see code comments for further details)"
            #endif
        #endif
    #endif // #if !defined(STDEXT_IMPORTED_STD)
#endif // #if !defined(DECLARE_MACROS_ONLY)

///////////////////////////////////////////////////////
// MSFT? (or any other compiler we support running in
// Microsoft VC++ compatibility mode - "Clang" and
// "Intel" are the only ones we currently support that
// are affected)
///////////////////////////////////////////////////////
#if defined(_MSC_VER)
    ////////////////////////////////////////////////////////////////
    // Mainly to pick up TCHAR (typedef) and _T macro (for our
    // specific needs). These are well-known to Microsoft
    // developers. Google for the details if you're not already
    // familiar, starting with Microsoft's own article:
    //
    //    Generic-Text Mappings in tchar.h
    //    https://learn.microsoft.com/en-us/cpp/text/generic-text-mappings-in-tchar-h?view=msvc-170).
    //
    // However, we'll later create an alias named "tchar" and set
    // it to TCHAR rather than rely on TCHAR directly (since TCHAR
    // is in the global namespace while "tchar" will be in our own
    // "StdExt" namespace which is what we want). On Microsoft
    // platforms TCHAR and tchar will therefore always refer to the
    // same type, either "wchar_t" or "char", though normally the
    // former in modern Windows (which is based on Unicode so both
    // UNICODE and _UNICODE will be #defined normally which results
    // in "wchar_t" - again, see the above article for starters).
    //
    // Note that it's expected that <tchar.h> will be found in the
    // #include search path which is normally the case when
    // compiling for Windows. For non-Microsoft compilers however,
    // unless they're running in Microsoft VC++ compatibility mode
    // in which case they'll come through here as well (Clang and
    // Intel compilers are the only ones we currently support that
    // might), then we'll set "tchar" to "char" further below (for
    // our purposes "tchar" will always be "char" on non-Microsoft
    // platforms), and also #define _T as well. See these
    // declarations further below. We can then rely on "tchar" for
    // all character types and the _T macro as well when required,
    // no different from how it's done on Microsoft platforms (even
    // though TCHAR is normally used on Microsoft platforms instead
    // of tchar but when targeting Microsoft they now alias the
    // same type).
    ////////////////////////////////////////////////////////////////
    #include <tchar.h>

    //////////////////////////////////////////////////////////////////
    // Macros to extract the major and minor version numbers from the
    // predefined Microsoft constant _MSC_VER (pass the latter
    // constant to macros MSC_GET_VER_MAJOR or MSC_GET_VER_MINOR as
    // required), or the build number from the predefined Microsoft
    // constant _MSC_FULL_VER (pass the latter constant to macro
    // MSC_GET_VER_BUILD). For instance, if you're currently running
    // Visual C++ 2019 and _MSC_VER is 1920 and _MSC_FULL_VER is
    // 192027508, then the following will display "19.20.27508" (but
    // you normally shouldn't pass _MSC_VER and _MSC_FULL_VER
    // directly as seen in this example, since the constants
    // described further below are easier - read on):
    //
    //     // Displays "19.20.27508"
    //     tcout << MSC_GET_VER_MAJOR(_MSC_VER) << "." <<
    //              MSC_GET_VER_MINOR(_MSC_VER) << "." <<
    //              MSC_GET_VER_BUILD(_MSC_FULL_VER);
    //
    // Note that you need not call these macros directly however (as
    // seen in the above example) when you wish to pass _MSC_VER
    // itself (to extract the major or minor version number), or
    // _MSC_FULL_VER (to extract the build number), opposed to
    // passing some other constants instead (besides _MSC_VER or
    // _MSC_FULL_VER). When passing _MSC_VER or _MSC_FULL_VER that
    // is, you can simply rely on the constants MSC_VER_MAJOR,
    // MSC_VER_MINOR or MSC_VER_BUILD (respectively) instead, which
    // are #defined further below. They in turn call the following
    // "MSC_GET_VER_?" macros for you, passing _MSC_VER or
    // _MSC_FULL_VER as required. The above example can therefore be
    // more cleanly written like so (yielding the same result as the
    // above but less verbose):
    //
    //     // Displays "19.20.27508"
    //     tcout << MSC_VER_MAJOR << "." <<
    //              MSC_VER_MINOR << "." <<
    //              MSC_VER_BUILD;
    //
    // You therefore only need to call the following "MSC_GET_VER_?"
    // macros directly when you need to explicitly pass arguments
    // other than the predefined Microsoft constants _MSC_VER and
    // _MSC_FULL_VER. For instance, if you wish to extract the major,
    // minor and build numbers for, say, the first release version of
    // Visual Studio 2022 (version 19.30.30705), then you can pass
    // the following Visual Studio 2022 constants (#defined in this
    // header later on) to these "MSC_GET_VER_?" macros like so:
    //
    //     // Displays "19.30.30705"
    //     tcout << MSC_GET_VER_MAJOR(MSC_VER_2022) << _T(".") <<
    //              MSC_GET_VER_MINOR(MSC_VER_2022) << _T(".") <<
    //              MSC_GET_VER_BUILD(MSC_FULL_VER_2022);
    //
    // Again, you normally only need to call these "MSC_GET_VER_?"
    // macros directly when passing args other than the predefined
    // Microsoft constants _MSC_VER and _MSC_FULL_VER (since in the
    // latter case you can just rely on the constants MSC_VER_MAJOR,
    // MSC_VER_MINOR and MSC_VER_BUILD instead, as described above).
    //////////////////////////////////////////////////////////////////
    #define MSC_GET_VER_MAJOR(MSC_VER) ((MSC_VER) / 100) // E.g, if MSC_VER is 1920 then returns 19
    #define MSC_GET_VER_MINOR(MSC_VER) ((MSC_VER) % 100) // E.g, if MSC_VER is 1920 then returns 20
    #define MSC_GET_VER_BUILD(MSC_FULL_VER) ((MSC_FULL_VER) % 100000) // E.g, if MSC_FULL_VER is 192027508 then returns 27508

    ///////////////////////////////////////////////////////////////////
    // #defined constants storing the compiler's major, minor, build
    // and revision numbers (for Microsoft C++). We simply strip these
    // out of the predefined Microsoft constants _MSC_FULL_VER, _MSC_VER, 
    // and _MSC_BUILD. See these here at this writing:
    //
    //    Predefined macros
    //    https://learn.microsoft.com/en-us/cpp/preprocessor/predefined-macros?view=msvc-170&redirectedfrom=MSDN
    //
    // Given the following example then, where the version number is
    // (arbitrarily) 19.20.27508.01, the predefined Microsoft constants
    // would be:
    //
    //    _MSC_FULL_VER = 192027508 (major, minor and build number)
    //    _MSC_VER = 1920 (major and minor number)
    //    _MSC_BUILD = 1 (revision number)
    //
    // The major, minor, build and revision numbers are then extracted
    // from the above to populate the following constants.
    //
    //     Example (using the above values)
    //     --------------------------------
    //     // Displays "19.20.27508.1"
    //     tcout << MSC_VER_MAJOR << _T(".") <<
    //              MSC_VER_MINOR << _T(".") <<
    //              MSC_VER_BUILD << _T(".") <<
    //              MSC_VER_REVISION;
    ///////////////////////////////////////////////////////////////////
    #define MSC_VER_MAJOR MSC_GET_VER_MAJOR(_MSC_VER) // E.g, if _MSC_VER is 1920 then returns 19
    #define MSC_VER_MINOR MSC_GET_VER_MINOR(_MSC_VER) // E.g, if _MSC_VER is 1920 then returns 20
    #define MSC_VER_BUILD MSC_GET_VER_BUILD(_MSC_FULL_VER) // E.g, if _MSC_FULL_VER is 192027508 then returns 27508
    #define MSC_VER_REVISION _MSC_BUILD // Just return _MSC_BUILD as-is (1 in the above example) since it stores
                                        // the actual revision number (so doesn't have to be calculated by us)

    /////////////////////////////////////////////////////////////
    // #defined constants corresponding to the predefined MSFT
    // constants _MSC_VER and _MSC_FULL_VER. One such constant
    // exists for each modern version of Visual Studio. Note
    // while MSFT does publish the values for _MSC_VER here at
    // this writing:
    //
    //     Predefined macros
    //     https://learn.microsoft.com/en-us/cpp/preprocessor/predefined-macros?view=msvc-170
    //
    // Unfortunately they don't publish each _MSC_FULL_VER at
    // this writing (after extensive searching - go figure), so
    // the constants below were picked up from the following
    // (unofficial) links instead (not sure where they got it
    // from but hopefully accurate):
    //
    //     Microsoft Visual C++
    //     https://en.wikipedia.org/wiki/Microsoft_Visual_C%2B%2B#Internal_version_numbering
    //
    //     Pre-defined Compiler Macros (Note: no longer being updated!)
    //     https://sourceforge.net/p/predef/wiki/Compilers/
    //
    // Also see the following for details on how to use the macros
    // _MSC_VER and _MSC_FULL_VER in general (they can be compared
    // against the #defined constants below):
    //
    //     Visual C++ Compiler Version (By Gabriel Dos Reis of MSFT)
    //     https://devblogs.microsoft.com/cppblog/visual-c-compiler-version/
    /////////////////////////////////////////////////////////////////

    // Visual Studio 2005
    #define MSC_VER_2005                1400
    #define MSC_FULL_VER_2005           140050727

    // Visual Studio 2008
    #define MSC_VER_2008                1500
    #define MSC_FULL_VER_2008           150021022
    #define MSC_FULL_VER_2008_SP1       150030729

    // Visual Studio 2010
    #define MSC_VER_2010                1600
    #define MSC_FULL_VER_2010           160030319
    #define MSC_FULL_VER_2010_SP1       160040219

    // Visual Studio 2012
    #define MSC_VER_2012                1700
    #define MSC_FULL_VER_2012           170050727

    // Visual Studio 2013
    #define MSC_VER_2013                1800
    #define MSC_FULL_VER_2013           180021005

    // Visual Studio 2015
    #define MSC_VER_2015                1900
    #define MSC_FULL_VER_2015           190023026
    #define MSC_FULL_VER_2015_UPDATE_1  190023506
    #define MSC_FULL_VER_2015_UPDATE_2  190023918
    #define MSC_FULL_VER_2015_UPDATE_3  190024210

    // Visual Studio 2017
    #define MSC_VER_2017                1910
    #define MSC_FULL_VER_2017           191025017
    #define MSC_VER_2017_V15_3          1911
    #define MSC_VER_2017_V15_5          1912
    #define MSC_VER_2017_V15_6          1913
    #define MSC_VER_2017_V15_7          1914
    #define MSC_VER_2017_V15_8          1915
    #define MSC_VER_2017_V15_9          1916

    // Visual Studio 2019
    #define MSC_VER_2019                1920
    #define MSC_FULL_VER_2019           192027508
    #define MSC_VER_2019_V16_1          1921
    #define MSC_VER_2019_V16_2          1922
    #define MSC_VER_2019_V16_3          1923
    #define MSC_VER_2019_V16_4          1924
    #define MSC_VER_2019_V16_5          1925
    #define MSC_VER_2019_V16_6          1926
    #define MSC_VER_2019_V16_7          1927
    #define MSC_VER_2019_V16_8_AND_9    1928
    #define MSC_FULL_VER_2019_16_9      192829500 // Search for this value here: https://learn.microsoft.com/en-us/cpp/preprocessor/predefined-macros?view=msvc-170
    #define MSC_VER_2019_V16_10_AND_11  1929
    #define MSC_FULL_VER_2019_16_11     192930100 // Search for this value here: https://learn.microsoft.com/en-us/cpp/preprocessor/predefined-macros?view=msvc-170

    // Visual Studio 2022
    #define MSC_VER_2022                1930
    #define MSC_FULL_VER_2022           193030705
    #define MSC_VER_2022_V17_1          1931
    #define MSC_VER_2022_V17_2          1932
    #define MSC_VER_2022_V17_3          1933
    #define MSC_VER_2022_V17_4          1934
    #define MSC_VER_2022_V17_5          1935
    #define MSC_VER_2022_V17_6          1936
    #define MSC_VER_2022_V17_7          1937
    #define MSC_VER_2022_V17_8          1938
    #define MSC_VER_2022_V17_9          1939
    #define MSC_VER_2022_V17_10         1940
#else
    ///////////////////////////////////////////////////////
    // Native (very ancient and ugly) MSFT specific macro
    // we'll adopt for other compilers as well (so we
    // #define it here for GCC/Clang/Intel and any others
    // we may support in the future). Used on MSFT
    // platforms to append an "L" to all string literals
    // (and less frequently character constants) so that,
    // say, "Your string" becomes L"Your string" instead
    // (when compiling for UTF-16 on Windows, as is
    // normally the case there - all string literals are
    // therefore wchar_t-based on MSFT platforms). We
    // #define it here to do nothing however meaning
    // everything will remain "char" based on non-MSFT
    // platforms (see tchar further below). Only when
    // compiling for MSFT will we rely on their own (real)
    // _T macro from <tchar.h> (#included above when
    // targeting MSFT), which appends an "L" as described
    // for Unicode builds. tchar-based string literals
    // will therefore normally be "wchar_t" in most MSFT
    // apps (since almost all MSFT apps in the real world
    // are compiled that way now - again, "wchar_t" is
    // used to store UTF-16 on all modern MSFT platforms)
    ///////////////////////////////////////////////////////
    #define _T(x) x
#endif // #if defined(_MSC_VER)

#if !defined(DECLARE_MACROS_ONLY)
    namespace StdExt
    {
        ///////////////////////////////////////////////////////
        // MSFT? (or any other compiler we support running in
        // Microsoft VC++ compatibility mode - "Clang" and
        // "Intel" are the only ones we currently support that
        // are affected)
        ///////////////////////////////////////////////////////
        #if defined(_MSC_VER)

            //////////////////////////////////////////
            // TCHAR declared in <tchar.h> #included
            // further above (Microsoft only)
            //////////////////////////////////////////
            using tchar = TCHAR;

            //////////////////////////////////////////////////////
            // _UNICODE or UNICODE #defined in caller's build
            // settings? Normally both are #defined in modern
            // versions of Windows meaning all strings will be
            // based on UTF-16 (we only check for either of these
            // constants though which is fine), but if neither is
            // #defined then user is compiling for ANSI normally
            // (ancient now so very rare)
            //////////////////////////////////////////////////////
            #if defined(_UNICODE) || defined(UNICODE)
                #if CPP17_OR_LATER // Inline variables not available until C++17 ...
                    inline decltype(std::wcout)& tcout = std::wcout;
                #else
                    #define tcout std::wcout
                #endif
            #else
                #if CPP17_OR_LATER // Inline variables not available until C++17 ...
                    inline decltype(std::cout)& tcout = std::cout;
                #else
                    #define tcout std::cout
                #endif
            #endif
        #else
            /////////////////////////////////////////////////////////
            // Ancient technique from MSFT we'll adopt for non-MSFT
            // compilers (see earlier comments before <tchar.h>
            // which we #include on Microsoft platforms). All
            // strings and other character-based types we need will
            // be based on "tchar" from here on. We always set this
            // to char here for non-Microsoft compilers (and TCHAR
            // on Microsoft platforms - see #if above), so only the
            // MSFT compiler (or other compilers running in VC++
            // compatibility mode) will use "wchar_t" instead
            // normally (for Unicode builds), or "char" for
            // non-Unicode builds (very rare these days).
            //
            // IMPORTANT: Note that you should not change this type
            // to anything else on non-MSFT platforms. It must
            // remain "char" in this release. If it were changed to
            // something else like "wchar_t" we'd have to deal with
            // "char <==> wchar_t" conversions at times which this
            // release is not currently designed to do. The use of
            // tchar for non-MSFT platforms simply allows us to more
            // easily deal with character types in a consistent way
            // (by always using tchar for any character types we
            // process whether we're targeting MSFT or not). On
            // MSFT platforms it will normally be "wchar_t" as noted
            // above, but "char" is also supported on MSFT if someone
            // compiles their program that way (though it's highly
            // unlikely on Windows anymore - Windows apps are
            // normally compiled for UTF-16 where tchar is "wchar_t"
            // but "char" is also supported if compiled for ANSI).
            // On non-MSFT platforms however only "char" is
            // currently supported ...
            /////////////////////////////////////////////////////////
            using tchar = char; // IMPORTANT - Don't change this. See comments above.

            ///////////////////////////////////////////////////////
            // Always char-based in this release. See tchar alias
            // above. Callers still need to explicitly #include
            // <iostream> in their own files however if using
            // "tcout" (to pick up "std::cout" since we don't
            // #include <iostream> in this file)
            ///////////////////////////////////////////////////////
            #if CPP17_OR_LATER // Inline variables not available until C++17 ...
                inline decltype(std::cout)& tcout = std::cout;
            #else
                #define tcout std::cout
            #endif
        #endif // #if defined(_MSC_VER)

        // "basic_string_view" not available until C++17
        #if CPP17_OR_LATER
            ///////////////////////////////////////////////////////
            // All uses of "std::basic_string_view" we need will
            // rely on this from here on (less verbose than using
            // "basic_string_view<tchar>" directly everywhere). It
            // therefore always resolves to "std::string_view" on
            // non-MSFT platforms (since tchar must *always* be
            // "char" in this release - see its declaration
            // further above), and normally (usually)
            // "std::wstring" on MSFT platforms (since tchar
            // normally resolves to "wchar_t" on MSFT platforms -
            // again, see this further above).
            //
            // Note that in theory we should add a similar "using"
            // statement for other types like "std::basic_string"
            // as well (naming it "tstring"), among other classes,
            // but for now the following is the only type we ever
            // use (but we can add more using statements later on
            // an as-needed basis - trying to minimize our use of
            // such using statements here regardless but
            // "tstring_view" in particular will likely be
            // frequently used by many so we declare it).
            //
            // Lastly, we could convert all functions, etc. that
            // deal with tchars into templates instead, with a
            // template arg specifying the character type (rather
            // than rely on "tstring_view"), and it's arguably the
            // "correct" way to do things (many would say), but in
            // my experience relying on "tstring_view" instead has
            // some benefits of its own I won't get into here (but
            // the situation can be revisited if ever required).
            // For now it normally works well.
            ///////////////////////////////////////////////////////
            using tstring_view = std::basic_string_view<tchar>;
        #endif // #if CPP17_OR_LATER

        ///////////////////////////////////////////////////////
        // GetCompilerName(). WYSIWYG. Note that these names
        // are hardcoded to provide the basic compile-time
        // name of each compiler we support and aren't
        // intended to replace the official name of each
        // compiler by its vendor should you require this (no
        // such function to retrieve that is currently
        // available since none of them provide any predefined
        // macros with the name at this writing)
        ///////////////////////////////////////////////////////
        inline constexpr const tchar * GetCompilerName() noexcept
        {
            #if defined(GCC_COMPILER)
                return _T("GCC or compatible");
            #elif defined(MICROSOFT_COMPILER)
                return _T("Microsoft Visual C++");
            #elif defined(CLANG_COMPILER)
                return _T("Clang");
            #elif defined(INTEL_COMPILER)
                return _T("Intel oneAPI DPC++/C++");
            #else
                /////////////////////////////////////////////////////
                // Note: Shouldn't be possible to come through here
                // at this writing since the same #error message
                // would have already been displayed earlier (when
                // the compiler constants just above were #defined)
                /////////////////////////////////////////////////////
                #error "Unsupported compiler"
            #endif
        }
    } // namespace StdExt
#else
    #undef DECLARE_MACROS_ONLY
#endif // #if !defined(DECLARE_MACROS_ONLY)
#endif // #ifndef COMPILER_VERSIONS (#include guard)