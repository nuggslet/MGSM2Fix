#ifndef TYPETRAITS
#define TYPETRAITS

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
// Header file implementing "FunctionTraits" (fully) described here
// https://github.com/HexadigmSystems/FunctionTraits. More generally
// however, the header contains miscellaneous type traits similar to those
// in the native C++ header <type_traits>. Its main focus for now however is
// "FunctionTraits". All other "public" declarations are used to support it
// (read on for what "public" means), and while they can also be used by end
// users, they're not documented at the above link (but are largely
// documented in this header). Only declarations directly related to
// "FunctionTraits" are (fully) documented at the above link.
//
// Note that this header is supported by GCC, Microsoft, Clang and Intel
// compilers only at this writing (C++17 and later - the check for
// CPP17_OR_LATER just below causes all code to be preprocessed out
// otherwise). A separate check below also ensures the minimum supported
// versions of these compilers are running or a #error terminates
// compilation. Note that this file depends on (#includes)
// "CompilerVersions.h" as well. All other dependencies are native C++
// headers with the exception of the native Windows header "tchar.h" on MSFT
// platforms (or any other supported compiler running in Microsoft VC++
// compatibility mode). "tchar.h" must be in the #include search path (and
// normally will be when targeting MSFT).
//
// Lastly, note that all declarations in this file are in namespace
// "StdExt". Everything is available for public use except declarations in
// (nested) namespace "StdExt::Private" (reserved for internal use). Macros
// however don't respect namespaces but all macros #defined for internal use
// only are documented as such. All others are available for public use (if
// not explicitly documented for internal use).
/////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
// Our header containing mostly #defined C++ version constants
// indicating which version of C++ is in effect (which we can
// test for below as required - see "C++ Version Macros" in the
// following file). A few other compiler-related declarations
// also exist however. Note that we #include this first so we
// can immediately start using these version constants (in
// particular CPP17_OR_LATER seen just below - we don't support
// earlier versions)
////////////////////////////////////////////////////////////////
#include "CompilerVersions.h"

//////////////////////////////////////////////////////////////
// This header supports C++17 and later only. All code below
// ignored otherwise (preprocessed out).
//////////////////////////////////////////////////////////////
#if CPP17_OR_LATER

//////////////////////////////////////////////////////////////
// GCC compiler running? (the real one or any one compatible
// with it but *not* any compiler we explicitly support which
// includes Clang and Intel only at this writing - we
// independently test for them below but other GCC compatible
// compilers that we don't explicitly support will be handled
// by the following call instead, since they claim to be GCC
// compatible) ...
//////////////////////////////////////////////////////////////
#if defined(GCC_COMPILER)  
    ///////////////////////////////////////////////////
    // We only support GCC >= 10.2. Abort compilation
    // otherwise.
    ///////////////////////////////////////////////////
    #if __GNUC__ <= 10 && \
        (__GNUC__ != 10 || __GNUC_MINOR__ < 2)
        #error "Unsupported version of GCC detected. Must be >= V10.2 (this header doesn't support earlier versions)"

        /////////////////////////////////////////////////////////////
        // #including this non-existent file forces compilation to
        // immediately stop. Otherwise more errors will occur before
        // compilation finally stops. Just a hack and not really
        // guaranteed to work but testing shows it normally does
        // (for GCC and all other compilers we support at this
        // writing). Other alternative techniques such as #pragmas
        // to stop compilation on the first error aren't guaranteed
        // to be reliable either. They (generally) don't affect the
        // preprocessor for instance so compilation doesn't stop
        // when the #error statement above is encountered. The
        // #error message is displayed but compilation resumes,
        // resulting in additional (extraneous) error messages
        // before compilation finally stops. The upshot is that we'll
        // just rely on the following for now to prevent these
        // additional errors from being displayed after the #error
        // message above (since the extra error messages are just
        // a distraction from the actual #error message above).
        /////////////////////////////////////////////////////////////
        #include <StopCompilingNow> // Hack to force compilation to immediately stop.
                                    // See comments above for details.
    #endif
///////////////////////////////////////////////////////////////////
// Microsoft compiler (VC++) running? (the real VC++, not another
// compiler running in Microsoft VC++ compatibility mode)
///////////////////////////////////////////////////////////////////
#elif defined(MICROSOFT_COMPILER)
    ///////////////////////////////////////////////////////////
    // Are we now running VC++ from a VS2017 release (opposed
    // to VS2019 or later). Can't be from VS2015 or earlier at
    // this point since CPP17_OR_LATER is currently #defined
    // (we checked for it earlier), so we must be targeting
    // VS2017 or later (since C++17 only became available in
    // VS2017). If the following is true then the version of
    // VC++ now running must therefore be from VS2017 (no
    // earlier version of VS possible at this point).
    ///////////////////////////////////////////////////////////
    #if _MSC_VER < MSC_VER_2019
        //////////////////////////////////////////////////////////
        // We only support VC++ >= 19.16 which was released with
        // Visual Studio 2017 version 15.9. Abort compilation
        // otherwise.
        //////////////////////////////////////////////////////////
        #if _MSC_VER >= MSC_VER_2017_V15_9 // VC++ 19.16
            ////////////////////////////////////////////////////////
            // For internal use only (we #undef it later). Compiler
            // is a VS2017 release of MSVC.
            ////////////////////////////////////////////////////////
            #define MSVC_FROM_VISUAL_STUDIO_2017
        #else
            ////////////////////////////////////////////////////
            // Note that VC++ normally stops compiling as soon
            // as this is encountered, so we don't rely on the
            // #include <StopCompilingNow> hack used for the
            // other compilers (above and below). See:
            //
            //     #error directive (C/C++)
            //     https://learn.microsoft.com/en-us/cpp/preprocessor/hash-error-directive-c-cpp?view=msvc-170
            ////////////////////////////////////////////////////
            #error "Unsupported version of Microsoft Visual C++ detected. Must be >= V19.16 (normally released with VS2017 V15.9). This header doesn't support earlier versions."
        #endif
    #endif
// Clang compiler running? (possibly in Microsoft VC++ compatibility mode - ok)
#elif defined(CLANG_COMPILER)
    /////////////////////////////////////////////////////
    // We only support Clang >= 11.0. Abort compilation
    // otherwise.
    /////////////////////////////////////////////////////
    #if __clang_major__ < 11
        #error "Unsupported version of Clang detected. Must be >= V11.0 (this header doesn't support earlier versions)"
        #include <StopCompilingNow> // Hack to force compilation to immediately stop.
                                    // See comments for GCC compiler above for details.
    #endif
// Intel compiler (DPC++/C++) running? (possibly in Microsoft VC++ compatibility mode - ok)
#elif defined(INTEL_COMPILER)
    ////////////////////////////////////////////////////////
    // We only support Intel >= 2021.4.0. Abort compilation
    // otherwise.
    ////////////////////////////////////////////////////////
    #if __INTEL_LLVM_COMPILER < 20210400
        #error "Unsupported version of Intel oneAPI DPC++/C++ detected. Must be >= V2021.4.0 (this header doesn't support earlier versions)"
        #include <StopCompilingNow> // Hack to force compilation to immediately stop.
                                    // See comments for GCC compiler above for details.
    #endif
#else
    #error "Unsupported compiler (GCC, Microsoft, Clang and Intel are the only ones supported at this writing)"
#endif

///////////////////////////////////////////////////
// Sanity check only. Should always be false (for
// internal use only - #defined below if required)
///////////////////////////////////////////////////
#if defined(DECLARE_MACROS_ONLY)
    #error "DECLARE_MACROS_ONLY already #defined (for internal use only so never should be)"
#endif

/////////////////////////////////////////////////////////////////////////////
// If all 3 conditions we're testing here are true then this header is now
// being #included by a client in the module version of the header (the
// module named "TypeTraits" in "TypeTraits.cppm" or whatever the user may
// have renamed the latter file's extension to). In this case we know
// modules (a C++20 feature), are supported (the 1st condition tests
// CPP20_OR_LATER for this but will be replaced with the official C++
// feature constant "__cpp_modules" in a later release since it's not yet
// supported by all compilers), and the 2nd condition then checks if
// STDEXT_USE_MODULES was #defined by the user themself, indicating they've
// added the ".cppm" files to their project (in this case the
// "TypeTraits.cppm" module) and wish to use it. In that case we'll go on to
// declare only #defined (public) macros in this header instead of all other
// C++ declarations that are also normally declared (when STDEXT_USE_MODULES
// isn't #defined), since those C++ declarations will now originate from the
// module itself (which we import via "import TypeTraits" in the code just
// below, though the user might also do this themself). Therefore, when a
// user #includes this header in the module version, only the macros in the
// header will need to be declared since C++ modules don't export #defined
// macros (we #define an internal constant DECLARE_MACROS_ONLY below to
// facilitate this). If the user hasn't #defined STDEXT_USE_MODULES however
// then they wish to use this header normally (declare everything as usual),
// even if they've also added the module to their project as well (which
// will still be successfully compiled), though that would be very unlikely
// (why add the module if they're not going to use it? - if they've added it
// then they'll normally #define STDEXT_USE_MODULES as well). As for the 3rd
// condition we're testing here, STDEXT_BUILDING_MODULE_TYPETRAITS (an
// internal constant), this is #defined by the module itself
// (TypeTraits.cppm) before #including this header in its global fragment
// section. The module then simply exports all public declarations from this
// header in its purview section via "using" declarations.
// STDEXT_BUILDING_MODULE_TYPETRAITS is therefore #defined only if the
// module itself is now being built. If #defined then it must be the module
// itself now #including us and if not #defined then it must be a client of
// the module #including us instead (which is what the following tests for
// in its 3rd condition - if all 3 conditions are true then it must be a
// user #including us, not the module itself, so we only declare the
// #defined macros in this header instead of all other declarations, as
// described above).
/////////////////////////////////////////////////////////////////////////////
#if CPP20_OR_LATER && defined(STDEXT_USE_MODULES) && !defined(STDEXT_BUILDING_MODULE_TYPETRAITS) // See comments about CPP20_OR_LATER above (to be
                                                                                                 // replaced by "__cpp_modules" in a future release)
    // "import std" not currently in effect? (C++23 or later)
    #if !defined(STDEXT_IMPORTED_STD)
        ///////////////////////////////////////////////////
        // Always pick up <type_traits> even when clients
        // are #including us in the module version of
        // "TypeTraits" (i.e., when "TypeTraits.cppm" has
        // been added to the project). Most clients that
        // #include "TypeTraits.h" would expect
        // <type_traits> to also be picked up so in the
        // module version we still #include it here as a
        // convenience to clients only. Note that when
        // STDEXT_IMPORTED_STD is true however (we just
        // tested it above and it's false), then
        // "import std" is currently in effect so
        // <type_traits> is already available.
        ///////////////////////////////////////////////////
        #include <type_traits>
    #endif

    //////////////////////////////////////////////////////
    // Importing the "TypeTraits" module as a convenience
    // to module clients. This way clients can simply
    // #include this header without having to explicitly
    // import the "TypeTraits" module themselves (since
    // this header does it for them). It's harmless
    // though if they've already imported the
    // "TypeTraits" module on their own (which is more
    // natural anyway - relying on this header may even
    // confuse some users who might not realize that a
    // "#include TypeTraits.h" statement is actually
    // importing the "TypeTraits" module to pick up all
    // public declarations in the header instead of
    // declaring them in the header itself - this is how
    // the header behaves when STDEXT_USE_MODULES is
    // #defined). #including this header however will
    // also pick up all public macros in the header since
    // modules don't export macros (so if clients simply
    // import the "TypeTraits" module and don't #include
    // this header, they'll have access to all exported
    // declarations in the module which originate from
    // this header, but none of the macros in the header
    // - fine if they don't use any of them though).
    //////////////////////////////////////////////////////
    import TypeTraits;

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
//////////////////////////////////////////////////////////////
// We're now coming through either because the module version
// of this header isn't installed ("TypeTraits.cppm" or
// whatever the user may have renamed the extension to), so
// the header is handled normally (we declare everything
// below as usual), or it is installed but the latter file
// (module) is now #including us during its own build (it
// always does so the header is also treated normally when
// that occurs - again, everything is declared below as
// usual).
//////////////////////////////////////////////////////////////
#else

// "import std" not currently in effect? (C++23 or later)
#if !defined(STDEXT_IMPORTED_STD)
    // Standard C/C++ headers
    #include <array>
    #include <cstddef>
    #include <iostream>
    #include <string_view>
    #include <tuple>
    #include <type_traits>
    #include <utility>
#endif

// Everything below in this namespace
namespace StdExt
{
    /////////////////////////////////////////////////////
    // AlwaysFalse_v. Used in templates only normally
    // where you need to always pass "false" for some
    // purpose but in a way that's dependent on a
    // template arg (instead of passing false directly).
    // In almost all real-world cases however it will be
    // used as the 1st arg to "static_assert", where
    // the 1st arg must always be false. Can't use
    // "false" directly or it will always trigger the
    // "static_assert" (even if that code is never
    // called or instantiated). Making it a (template)
    // dependent name instead eliminates the problem
    // (use the following IOW instead of "false"
    // directly). See:
    //
    //    // always_false<T>
    //    https://artificial-mind.net/blog/2020/10/03/always-false    // 
    // 
    //    How can I create a type-dependent expression that is always false?
    //    https://devblogs.microsoft.com/oldnewthing/20200311-00/?p=103553
    /////////////////////////////////////////////////////
    template <typename...>
    inline constexpr bool AlwaysFalse_v = false;

    ////////////////////////////////////////////////////////
    // AlwaysTrue_v. Similar to "AlwaysFalse_v" just above
    // but used wherever you require "true" in a way that's
    // dependent on a template arg. Very rarely used in
    // practice however (not many uses for it), unlike
    // "AlwaysFalse_v" just above which is more frequently
    // used to trigger a "static_assert" where required.
    // See "AlwaysFalse_v" above.
    ////////////////////////////////////////////////////////
    template <typename...>
    inline constexpr bool AlwaysTrue_v = true;

    /////////////////////////////////////////////////////////////////////
    // Private namespace (for internal use only) used to implement
    // "TypeName_v" declared just after this namespace. Allows you to
    // retrieve the type name for any type as a compile-time string
    // (std::basic_string_view). See "TypeName_v" for full details (it
    // just defers to "TypeNameImpl::Get()" in the following namespace).
    /////////////////////////////////////////////////////////////////////
    namespace Private
    {
        ///////////////////////////////////////////////////////
        // Identical to "tstring_view::operator==()" but this
        // has a bug in the VS2017 release of VC++ (details
        // don't matter since VS2017 is getting old now
        // anyway). The bug was fixed in the VS2019 release of
        // VC++. If (when) the VS2017 release of VC++ is
        // eventually dropped, the function can be removed and
        // calls to it can simply be replaced with a call to
        // "str1 == str2" (i.e., the first return statement
        // seen below).
        ///////////////////////////////////////////////////////
        inline constexpr bool IsEqualTo(tstring_view str1,
                                        tstring_view str2) noexcept
        {
            ////////////////////////////////////////////////
            // Any compiler other than VC++ from Microsoft
            // Visual Studio 2017 ...
            ////////////////////////////////////////////////
            #if !defined(MSVC_FROM_VISUAL_STUDIO_2017)
                return (str1 == str2);
            //////////////////////////////////////////////////
            // VC++ from Visual Studio 2017. Code just above
            // should work there as well but it's buggy
            // (can return false even when true). The
            // following is a manual work-around (equivalent
            // to code above).
            //////////////////////////////////////////////////
            #else
                if (str1.size() == str2.size())
                {
                    for (tstring_view::size_type i = 0; i < str1.size(); ++i)
                    {
                        if (str1[i] != str2[i])
                        {
                            return false;
                        }
                    }

                    return true;
                }
                else
                {
                    return false;
                }
            #endif
        };

        ///////////////////////////////////////////////////
        // Identical to "tstring_view::ends_with()" but
        // this member isn't "constexpr" until C++20. We
        // therefore just defer to the latter in C++20 or
        // later or roll our own (equivalent) otherwise
        // (in C++17 - earlier versions not possible at
        // this point - check for CPP17_OR_LATER at top of
        // file ensures this). Note that this function can
        // be removed and simply replaced with calls to
        // "str.ends_with()" if we ever drop support for
        // C++17.
        ///////////////////////////////////////////////////
        inline constexpr bool EndsWith(tstring_view str,
                                       tstring_view suffix) noexcept
        {
            #if CPP20_OR_LATER
                // Not available until C++20
                return str.ends_with(suffix);
            #else
                ///////////////////////////////////////////////////////
                // Roll our own in C++17 (no earlier version possible
                // at this stage - checked for CPP17_OR_LATER at top
                // of file and preprocessed out any earlier versions).
                ///////////////////////////////////////////////////////
                return str.size() >= suffix.size() &&
                       IsEqualTo(str.substr(str.size() - suffix.size()),
                                 suffix);
            #endif
        }

        ////////////////////////////////////////////////////////////
        // Converts "str" to a "std::array" consisting of all chars
        // in "str" at the indexes specified by the 2nd arg (a
        // collection of indexes in "str" indicating which chars in
        // "str" will be copied into the array). A NULL terminator
        // is also added at the end. The size of the returned
        // "std::array" is therefore the number of indexes in the
        // 2nd arg plus 1 for the NULL terminator. The array itself
        // is populated with all chars in "str" at these particular
        // indexes (again, with a NULL terminator added). The type
        // of the returned array is therefore:
        //
        //     std::array<tchar, sizeof...(I) + 1>
        //
        // Note that in practice this function is usually called to
        // convert an entire constexpr string to a "std::array" so
        // the function is usually called this way:
        //
        //     // Any "constexpr" string
        //     constexpr tstring_view str = _T("Testing");
        //
        //     ///////////////////////////////////////////////////////
        //     // Convert above to a "std::array". The 2nd arg is
        //     // just the sequential sequence {0, 1, 2, ..., N - 1}
        //     // where "N" is "str.size()", so all chars in "str"
        //     // are copied over (in the expected order 0 to N - 1).
        //     ///////////////////////////////////////////////////////
        //     constexpr auto strAsArray = StrToNullTerminatedArray(str, std::make_index_sequence<str.size()>());
        //
        // The 2nd arg above is therefore just the sequence of
        // (std::size_t) integers in the range 0 to the number of
        // characters in "str" - 1. The function therefore returns
        // a "std::array" containing a copy of "str" itself since
        // the 2nd arg specifies every index in "str" (so each
        // character at these indexes is copied to the array). The
        // returned type in the above example is therefore the
        // following (7 characters in the above string plus 1 for
        // the NULL terminator which we always manually add):
        //
        //     std::array<tchar, 8>;
        //
        // Note that having to pass "std::make_index_sequence" as
        // seen in the above example is syntactically ugly however.
        // It's therefore cleaner to call the other
        // "StrToNullTerminatedArray" overload just below instead,
        // which is designed for this purpose (to copy all of "str"
        // which most will usually be doing - see overload below
        // for details). It simply defers to the overload you're
        // now reading but it's a bit cleaner.
        //
        // Even the cleaner overload below is still syntactically
        // ugly however (both overloads are), but unfortunately C++
        // doesn't support "constexpr" parameters at this writing,
        // which would eliminate the issue. The issue is that the
        // 2nd template arg of "std::array" is the size of the
        // array itself and this arg must be known at compile-time
        // of course (since it's a template arg). Since the "str"
        // parameter of both overloads isn't "constexpr" however
        // (since C++ doesn't currently support "constexpr"
        // parameters), neither function can directly pass
        // "str.size()" as the 2nd template parameter to
        // "std::array", even though "str" itself may be
        // "constexpr" at the point of call. It means that even
        // when "str" is "constexpr" at the point of call, the user
        // is still forced to pass "str.size()" as a template
        // parameter at the point of call since the function itself
        // can't legally do it (it can't pass it as the 2nd
        // template arg of "std::array" since "str" isn't
        // "constexpr" inside the function - it must therefore be
        // passed as a template parameter at the point of call
        // instead, which makes the syntax of these functions
        // unnatural). The situation is ugly and unwieldy since it
        // would be much cleaner and more natural to just grab the
        // size from "str.size()" inside the function itself, but
        // the language doesn't support it in a "constexpr" context
        // at this writing (maybe one day).
        //
        // The upshot is that the following overload is designed to
        // circumvent this C++ deficiency but will rarely be called
        // directly by most users (unless it's needed to copy a
        // different sequence of characters from "str" to the
        // returned array other than all of them). If you need to
        // copy all of "str" to the returned array (usually the
        // case), then it's cleaner to call the other overload just
        // below instead. See this for details.
        ////////////////////////////////////////////////////////////
        template <std::size_t... I>
        inline constexpr auto StrToNullTerminatedArray(tstring_view str,
                                                       std::index_sequence<I...>) noexcept
        {
            ///////////////////////////////////////////////////////////
            // Creates a "std::array" and initializes it to all chars
            // in "str" (just a fancy way to copy it but in the same
            // order given by "I" - usually sequential), but also
            // adding a NULL terminator as seen, guaranteeing the
            // array is NULL terminated (so callers can safely depend
            // on this if required - note that whether "str" itself is
            // already NULL terminated is therefore irrelevant to us -
            // harmless if it is but we just ignore it).
            //
            // Note BTW that we could rely on CTAD (Class Template
            // Argument Deduction) in the following call if we wish,
            // and therefore avoid explicitly passing the "std::array"
            // template args in the return value as we're now doing
            // (since it's more clear what's being returned IMHO). If
            // we relied on CTAD instead then we could just return the
            // following instead (less verbose than manually passing
            // the "std::array" template args as we're now doing but
            // again, it's not as clear what's being returned IMHO):
            //
            //     return std::array{str[I]..., _T('\0')};
            //
            // The type of "std:array" would then resolve to the
            // following, where "N" is the number of (std::size_t)
            // integers in parameter pack "I", and there's one extra
            // tchar for the NULL terminator we're manually adding
            // (hence the "+ 1"):
            //
            //     std::array<tchar, N + 1>;
            //
            // In either case, whether we explicitly pass the
            // "std::array" template args or rely on CTAD, parameter
            // pack expansion of "I" occurs as always so if "I" is
            // {0, 1, 2} for instance (though it doesn't have to be in
            // sequential order if a caller requires a different order
            // but usually not), then it resolves to the following
            // (again, note that we manually pass the NULL terminator
            // as seen):
            //
            //     return std::array<tchar, 4>{str[0], str[1], str[2], _T('\0')};
            //
            // Note that things also work if "I" is empty (so its size
            // is zero), normally because "str" is empty so there's
            // nothing to copy. In this case the call simply resolves
            // to the following:
            //
            //     return std::array<tchar, 1>{_T('\0')};
            //
            // The above array therefore stores the NULL terminator
            // only (as would be expected).
            ///////////////////////////////////////////////////////////
            return std::array<tchar, sizeof...(I) + 1>{str[I]..., _T('\0')};
        }

        ////////////////////////////////////////////////////////////
        // See other overload just above. The following overload
        // just defers to it in order to convert "str" to a
        // "std::array" which it then returns (with a NULL
        // terminator always added - see overload above). Note
        // that the following overload exists to clean up the
        // syntax of the above overload a bit when you need to
        // convert the entire "str" arg to a "std::array", as
        // most will usually be doing (though to copy any other
        // sequence of characters from "str" to a "std::array"
        // you'll need to call the above overload directly
        // instead, passing the specific indexes in "str" you
        // wish to copy - very rare though).
        //
        // Note that as explained in the overload above, the
        // syntax for both overloads is ugly due to "constexpr"
        // shortcomings in current versions of C++. The following
        // overload is nevertheless a cleaner version than the
        // one above when you simply need to convert "str" to a
        // "std::array" (again, as most will usually be doing).
        //
        //     Example
        //     -------
        //     // Any "constexpr" string
        //     constexpr tstring_view str = _T("Testing");
        //
        //     //////////////////////////////////////////////////
        //     // The type of "array" returned by the following
        //     // call is therefore the following:
        //     //
        //     //     std::array<tchar, 8>
        //     //
        //     // Note that its size, 8, is the length of
        //     // "Testing" above, 7, plus 1 for the NULL
        //     // terminator, but the NULL terminator doesn't
        //     // originate from the one in the "Testing" string
        //     // literal above. Instead we always manually add
        //     // our own. "str" above therefore need not be NULL
        //     // terminated even though it is in this case
        //     // (since it's initialized from a string literal
        //     // above which does include one - if initialized
        //     // from another source that's not NULL terminated
        //     // however then it doesn't matter since we always
        //     // manually add our own).
        //     //
        //     // Note that as seen in the following call,
        //     // "str.size()" should always be passed as the 1st
        //     // template parameter to "StrToNullTerminatedArray()",
        //     // but having to do this is ugly. That is,
        //     // "StrToNullTerminatedArray()" can simply get
        //     // hold of this by invoking "size()" on the same
        //     // "str" parameter we're passing it, so having to
        //     // separately pass it as a template parameter is
        //     // really unnecessary. However, since the "size()"
        //     // member is required in a "constexpr" context and
        //     // C++ doesn't support "constexpr" function
        //     // arguments at this writing, the function can't
        //     // call "size()" on its "str" arg in a "constexpr"
        //     // context so we're forced to pass it as a
        //     // template parameter instead (for now - maybe C++
        //     // will support "constexpr" function parameters
        //     // one day)
        //     //////////////////////////////////////////////////
        //     constexpr auto strAsArray = StrToNullTerminatedArray<str.size()>(str);
        ////////////////////////////////////////////////////////////
        template <std::size_t Size> // Always pass "str.size()"
        inline constexpr auto StrToNullTerminatedArray(tstring_view str) noexcept
        {
            ////////////////////////////////////////////////
            // Defer to overload just above, passing all
            // indexes that need to be copied from "str"
            // into the resulting array (via the 2nd arg,
            // i.e., zero to the number of indexes in "str"
            // - 1). Note that the "Size" template arg
            // we're passing to "std::make_index_sequence"
            // must always be "str.size()" itself (callers
            // should always call us this way - see
            // function comments above)
            //////////////////////////////////////////////////
            return StrToNullTerminatedArray(str, std::make_index_sequence<Size>());
        }

        ///////////////////////////////////////////////////////////////
        // "TypeNameImplBase". Base class of "TypeNameImpl" that
        // follows just after it. Latter class is a template but the
        // following class isn't. Therefore contains functions that
        // don't depend on template arg "T" of "TypeNameImpl" so no
        // need to declare them there. "TypeNameImpl" simply inherits
        // from the following class instead and defers to it to carry
        // out the actual work (of extracting the type name for its
        // "T" template arg from __PRETTY_FUNCTION__ or (on MSFT
        // platforms) __FUNCSIG__)
        ///////////////////////////////////////////////////////////////
        class TypeNameImplBase
        {
        protected:
            //////////////////////////////////////////////////////////////////
            // *** IMPORTANT ***
            //
            // Must be declared before 1st use or compilation will fail in
            // some compilers (declaration order of function templates
            // matters).
            //
            // GetPrettyFunction(). Returns the predefined string constant
            // __PRETTY_FUNCTION__ or (for MSFT) __FUNCSIG__ (the latter if
            // _MSC_VER is #defined so it also applies to non-Microsoft
            // compilers running in Microsoft VC++ compatibility mode - see
            // "Non-Microsoft compilers targeting Windows" further below).
            // Returns these as a "tstring_view" which lives for the life of
            // the app since it's just a view into the latter strings which
            // are always statically defined (though compilers will normally
            // remove these static strings from the final compiled binary if
            // they determine the returned "tstring_view" is used in a
            // compile-time only context - they're not used at runtime IOW so
            // they can safely be removed).
            //
            // Assuming template arg "T" is a float for instance, each
            // resolves to the following at this writing (where the first
            // three rows show the offsets in the strings for your guidance
            // only, and the rows just after show the actual value of the
            // above strings for the compilers we currently support). Note
            // that each string originates from __PRETTY_FUNCTION__ if
            // _MSC_VER is not #defined or __FUNCSIG__ otherwise (note that
            // when non-Microsoft compilers are running in Microsoft VC++
            // compatibility mode then _MSC_VER will always be #defined so we
            // rely on __FUNCSIG__ as noted, but __PRETTY_FUNCTION__ is still
            // #defined by those compilers as well, even though the format
            // differs from __FUNCSIG__ a bit - we just ignore
            // __PRETTY_FUNCTION__ altogether in this case however):
            //                                                                                                                                       1         1         1         1         1         1         1
            //                                             1         2         3         4         5         6         7         8         9         0         1         2         3         4         5         6
            //                                   012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901
            //   GCC (1 of 2 - described below): static constexpr auto StdExt::Private::TypeNameImplBase::GetPrettyFunction() [with T = float]
            //   GCC (2 of 2 - described below): static constexpr StdExt::tstring_view StdExt::Private::TypeNameImplBase::GetPrettyFunction() [with T = float; StdExt::tstring_view = std::basic_string_view<char>]
            //   Clang:                          static auto StdExt::Private::TypeNameImplBase::GetPrettyFunction() [T = float]
            //   Clang (if _MSC_VER #defined):   static auto __cdecl StdExt::Private::TypeNameImplBase::GetPrettyFunction(void) [T = float]
            //   Intel:                          static auto StdExt::Private::TypeNameImplBase::GetPrettyFunction() [T = float]
            //   Intel (if _MSC_VER #defined):   static auto __cdecl StdExt::Private::TypeNameImplBase::GetPrettyFunction(void) [T = float]
            //   Microsoft VC++:                 auto __cdecl StdExt::Private::TypeNameImplBase::GetPrettyFunction<float>(void) noexcept
            //
            // GCC
            // ---
            // Note that unlike the other compilers, two possible formats
            // exist for GCC as seen above (the others have one), where the
            // one used depends on the return type of "GetPrettyFunction()"
            // itself. GCC format "1 of 2" above is the one currently in
            // effect at this writing since "GetPrettyFunction()" currently
            // returns "auto", which just resolves to
            // "std::basic_string_view<tchar>" (unless someone changed the
            // return type since this writing which is safely handled but
            // read on). Because "GetPrettyFunction()" is *not* returning an
            // alias for another type in this case (it's returning "auto"
            // which is not treated as an alias by GCC), GCC uses format "1
            // of 2". If the return type of "GetPrettyFunction()" is ever
            // changed however so that it returns an alias instead, such as
            // "tstring_view" seen in "2 of 2" above (which is an alias for
            // "std::basic_string_view<tchar>" - note that tchar always
            // resolves to "char" on GCC), then GCC uses format "2 of 2"
            // above instead. In this case it includes (resolves) this alias
            // at the end of __PRETTY_FUNCTION__ itself, as seen. This refers
            // to the "tstring_view = std::basic_string_view<char>" portion
            // of the "2 of 2" string above. It removes this portion of the
            // string in "1 of 2" however since the return value of
            // "GetPrettyFunction()" is not returning an alias in this case
            // but "auto" (or you can change "auto" directly to
            // "std::basic_string_view<tchar>" if you wish since it's still
            // not an alias). The "tstring_view =
            // std::basic_string_view<char>" portion of the "2 of 2" string
            // is no longer required IOW since there's no "tstring_view"
            // alias anymore so GCC removes it. The upshot is that the offset
            // to the type we're trying to extract from __PRETTY_FUNCTION__,
            // a "float" in the above example but whatever the type happens
            // to be, can change depending on the return type of
            // "GetPrettyFunction()" itself. Therefore, unlike the case for
            // all other compilers we currently support, whose offset to the
            // type is always at the same consistent location from the start
            // of __PRETTY_FUNCTION__ or (when _MSC_VER is #defined)
            // __FUNCSIG__, we need to check which format is in effect for
            // GCC in order to protect against possible changes to the return
            // type of "GetPrettyFunction()" itself. "GetTypeNameOffset()",
            // which calculates the offset of "T" into the string (offset of
            // "float" in the above examples), therefore takes this situation
            // into account for GCC only (checking which of these two formats
            // is in effect).
            //
            // Reliance on "float" to calculate the offset and length
            // ------------------------------------------------------
            // Note that float isn't just used for the examples above, it's
            // also used by members "GetTypeNameOffset()" and
            // "GetTypeNameLen()" below to determine the offset into each
            // string of the type name to extract (whatever template arg "T",
            // resolves to), and the type's length, both of which are called
            // by member "ExtractTypeNameFromPrettyFunction()" below to
            // extract the type name from the string. Since type float always
            // returns "float" for all compilers we support (which will
            // likely be the case for any future compiler as well normally,
            // though any type whose string is the same for all supported
            // compilers will do - read on), we can leverage this knowledge
            // to easily determine the offset of type "T" in the string and
            // its length, regardless of what "T" is, without any complicated
            // parsing of the above strings (in order to locate "T" within
            // the string and determine its length). Note that parsing would
            // be a much more complicated approach because type "T" itself
            // can potentially contain any character including angled
            // brackets, square brackets, equal signs, etc., each of which
            // makes it harder to distinguish between those particular
            // characters in "T" itself from their use as delimiters in the
            // actual "pretty" strings (when they aren't part of "T"). It
            // would usually be rare in practice but can happen. For
            // instance, "T" might be a template type (class) called, say,
            // "Bracket", with a non-type template arg of type "char" so
            // "Bracket" can be instantiated (and therefore appear in
            // __PRETTY_FUNCTION__ or __FUNCSIG__) like so (ignoring the
            // class' namespace if any to keep things simple):
            //
            //     Bracket<']'>
            //     Bracket<'>'>
            //     Bracket<'='>
            //     Bracket<';'>
            //
            // This makes it more difficult to parse __PRETTY_FUNCTION__ and
            // __FUNCSIG__ looking for the above type and determining its
            // length because the above strings contain the same characters
            // (potentially) used as delimiters elsewhere in
            // __PRETTY_FUNCTION__ and __FUNCSIG__ (where applicable), so any
            // parsing code would have to deal with this as required (i.e.,
            // distinguishing between these characters in "T" itself and
            // delimiters elsewhere in the "pretty" string is non-trivial).
            // For example, determining if ']' is part of the type itself or
            // an actual delimiter in __PRETTY_FUNCTION__ is less simple than
            // it first appears (not rocket science but a pain nevertheless).
            //
            // To circumvent having to do this (parse the string to deal with
            // this issue), there's a much easier alternative. For the offset
            // of "T" itself, note that it's always the same within each
            // supported compiler regardless of "T" (i.e., the same within
            // GCC, the same within Microsoft, the same within Clang, etc. -
            // it will normally be different for each particular compiler of
            // course but all that matters for our purposes is that it's the
            // same within each one). Within each particular compiler we can
            // therefore simply rely on a known type like "float" to
            // determine it (for that compiler), not "T" itself (since the
            // offset to "T" for any particular compiler will be the same as
            // the offset to "float" or any other type for that matter
            // (within that compiler), so we can arbitrarily just rely on
            // "float" - more on why "float" was chosen later).
            //
            // While the offset to "T" within the "pretty" string is always
            // identical no matter what "T" is (within each compiler), the
            // length of "T" itself will differ of course (depending on what
            // "T" is). However, we know that for any two different types of
            // "T", say "int" and "float" (any two types will do), the
            // "pretty" string containing "int" will be identical to the
            // "pretty" containing "float" except for the difference between
            // "int" and "float" themselves. That's the only difference
            // between these "pretty" strings, i.e., one contains "int" and
            // one contains "float", but the remainder of the "pretty"
            // strings are identical. Therefore, since the string "int" is 3
            // characters long and the string "float" is 5 characters long (2
            // characters longer than "int"), then the length of the "pretty"
            // string containing "int" must be shorter than the "pretty"
            // string containing "float" by 2 characters, since the strings
            // themselves are identical except for the presence of "int" and
            // "float" (within their respective strings). So by simply
            // subtracting the length of the "pretty" string for a "float"
            // from the length of the "pretty" string for any type "T" (i.e.,
            // by simply computing this delta), we know how much longer
            // (positive delta) or shorter (negative delta) the length of "T"
            // must be compared to a "float", since the latter is always 5
            // characters long. We therefore just add this (positive or
            // negative) delta to 5 to arrive at the length of "T" itself.
            // For an "int" for instance, the delta is -2 (the length of its
            // "pretty" string minus the length of the "pretty" string for
            // "float" is always -2), so 5 - 2 = 3 is the length of an "int".
            // For a type longer than "float" it works the same way only the
            // delta is positive in this case. If "T" is an "unsigned int"
            // for instance then the delta is 7 (the length of its "pretty"
            // string minus the length of the "pretty" string for "float" is
            // always 7), so 5 + 7 = 12 is the length of "unsigned int". We
            // can do this for any arbitrary "T" of course to get its own
            // length. We simply compute the delta for its "pretty" string in
            // relation to the "pretty" string for a "float" as described.
            //
            // Note that float was chosen over other types we could have used
            // since the name it generates in its own pretty string is always
            // "float" for all our supported compilers. It's therefore
            // consistent among all supported compilers and its length is
            // always 5, both situations making it a (normally) reliable type
            // to work with in our code. In practice however all the
            // fundamental types or (possibly) some user-defined type could
            // have been used (each of which normally generates the same
            // consistent string as well), but going forward "float" seemed
            // (potentially) less vulnerable to issues like signedness among
            // integral types for instance, or other possible issues. If an
            // integral type like "int" was chosen instead for instance (or
            // "char", or "long", etc.), some future compiler (or perhaps
            // some compiler option) might display it as "signed int" or
            // "unsigned int" instead, depending on the default signedness in
            // effect (and/or some compiler option but regardless, it might
            // not result in a consistent string among all compilers we
            // support or might support in the future).
            //
            // Similarly, if we chose "double" instead it might be displayed
            // as "double" as you would normally expect, but "long double"
            // might also be possible based on some compiler option (so the
            // string "double" may not be consistent either).
            //
            // Or if "bool" were chosen it might normally appear as "bool"
            // but "unsigned char" might be possible too if some backwards
            // compatibility option is turned on for a given compiler (since
            // bools may have been internally declared that as "unsigned
            // char" once-upon-a-time and someone may turn the option on for
            // backwards compatibility reasons if required).
            //
            // In reality it doesn't seem likely this is actually going to
            // happen for any of the fundamental types however, and even
            // "float" itself could potentially become a "double" (or
            // whatever) under some unknown circumstance but for now it seems
            // to be potentially more stable than the other fundamental types
            // so I chose it for that reason only (even if these reasons are
            // a bit flimsy). It normally works.
            //
            // Non-Microsoft compilers targeting Windows
            // -----------------------------------------
            // Note that non-Microsoft compilers targeting Windows will
            // #define _MSC_VER just like the Microsoft compiler does (VC++),
            // indicating they're running in Microsoft VC++ compatibility
            // mode (i.e., will compile VC++ Windows applications). For most
            // intents and purposes we can therefore (usually) just test if
            // _MSC_VER is #defined throughout our code and if true it means
            // that either VC++ is running or some other non-Microsoft
            // compiler is but it's running as if it were VC++. We can
            // therefore usually just ignore the fact that it's not the real
            // VC++ itself since it's running as if it were (so we can just
            // carry on as if the real VC++ is running). However, instead of
            // checking _MSC_VER we can also explicitly check the actual
            // (real) compiler that's running by using our own #defined
            // constants MICROSOFT_COMPILER, CLANG_COMPILER and
            // INTEL_COMPILER instead (the only compilers we currently
            // support that might #define _MSC_VER - MICROSOFT_COMPILER
            // itself refers to the real VC++ compiler so it always #defines
            // it but the other two only #define it when running in Microsoft
            // compatibility mode). For many (actually most) purposes
            // however we can just check _MSC_VER as described above when we
            // don't care whether it's the real VC++ compiler
            // (MICROSOFT_COMPILER #defined) vs the Clang compiler
            // (CLANG_COMPILER #defined) or the Intel compiler
            // (INTEL_COMPILER #defined), where the latter two cases simply
            // mean these non-Microsoft compilers are running in Microsoft
            // VC++ compatibility mode (since _MSC_VER is also #defined).
            //
            // Unfortunately the behavior of these non-Microsoft compilers
            // isn't always 100% compatible with the actual (real) Microsoft
            // compiler itself however. In particular, for the purposes of
            // the function you're now reading, note that the code relies on
            // the predefined Microsoft macro __FUNCSIG__ instead of
            // __PRETTY_FUNCTION__ whenever _MSC_VER is #defined, so the
            // actual (real) compiler doesn't matter to us (since __FUNCSIG__
            // is also #defined for non-Microsoft compilers running in
            // Microsoft VC++ compatibility mode). However, __FUNCSIG__ isn't
            // a string literal or even a macro when using non-Microsoft
            // compilers, unlike when using the actual Microsoft VC++
            // compiler itself. It normally should be a string literal even
            // on non-Microsoft compilers (if they were 100% compatible with
            // VC++) but unfortunately it's not. It's just an array of
            // "const" char when using non-Microsoft compilers but not an
            // actual string literal. We therefore normally shouldn't be able
            // to apply the native Microsoft macro _T to it since _T is just
            // the C++ string literal prefix "L" when compiling for UTF-16 in
            // Windows (usually the case), and the "L" prefix can only be
            // applied to string literals. See the following for details
            // (also consult _T in the MSFT docs if you're not already
            // familiar with it):
            //
            //     Wide string literals
            //     https://learn.microsoft.com/en-us/cpp/cpp/string-and-character-literals-cpp?view=msvc-170#wide-string-literals
            //
            // Since the "L" prefix can only be applied to string literals
            // then it shouldn't work with __FUNCSIG__ when using
            // non-Microsoft compilers, since the latter constant isn't a
            // string literal in non-Microsoft compilers as described. It's
            // only a string literal when using VC++ itself (as officially
            // documented by MSFT), so "L" should only work when compiling
            // with VC++. However, it turns out that "L" does work with
            // __FUNCSIG__ in non-Microsoft compilers, at least the ones we
            // support, even though it's not a string literal in those
            // compilers. While I'm not sure why without further digging
            // (read on), some type of special work-around was apparently
            // introduced by these particular compiler vendors to handle
            // __FUNCSIG__ as a string literal even though it's apparently
            // not defined that way (and presumably applicable to other
            // Microsoft predefined constants in these non-Microsoft
            // compilers as well I assume, though it doesn't impact out
            // situation here). See here for details about the situation but
            // it requires reading through the details to better understand
            // the fix (I haven't myself):
            //
            //    Clang + -fms-extensions: __FUNCSIG__ is not a literal
            //    https://github.com/llvm/llvm-project/issues/114
            //
            //    /////////////////////////////////////////////////////////
            //    // The issue is then cited as closed by "aeubanks" in
            //    // the above link (git commit 856f384), but he later
            //    // indicates that "patch was reverted, reopening", only
            //    // to close it again later on (git commit 878e590, which
            //    // the following links to). Apparently that was the
            //    // final fix.
            //    /////////////////////////////////////////////////////////
            //    https://github.com/llvm/llvm-project/commit/878e590503dff0d9097e91c2bec4409f14503b82
            //
            // Also see the following link which shows changes in a file called
            // "clang/docs/ReleaseNotes.rst"
            //
            //     [clang] Make predefined expressions string literals under -fms-extensions
            //     https://reviews.llvm.org/rG856f384bf94513c89e754906b7d80fbe5377ab53
            //
            // There's a comment in the "clang/docs/ReleaseNotes.rst" file
            // itself that indicates:
            //
            //     "Some predefined expressions are now treated as string
            //      literals in MSVC compatibility mode."
            //
            // I haven't looked into the situation in detail but the upshot
            // is that in the code just below we rely on __FUNCSIG__ whenever
            // _MSC_VER is #defined regardless of the compiler, and then
            // apply the _T macro to __FUNCSIG__ as seen, which works even
            // when using non-Microsoft compilers whose __FUNCSIG__ isn't a
            // string literal (in the compilers we currently support). It's
            // still unclear though if any compiler-specific options might
            // impact the situation but we'll have to live with the
            // uncertainty for now (though the above links would seem to
            // suggest the issue is fixed - __FUNCSIG__ is apparently treated
            // as a string literal again in all non-Microsoft compilers we
            // support).
            //
            // Note that relying on __PRETTY_FUNCTION__ instead when
            // targeting non-Microsoft compilers can't be used as a
            // (viable/practical) alternative to __FUNCSIG__, even though
            // __PRETTY_FUNCTION__ is also defined on non-Microsoft compilers
            // (in addition to __FUNCSIG__, though they each resolve to
            // different formats however). That's because the _T macro can't
            // be applied to __PRETTY_FUNCTION__ since it won't compile
            // because __PRETTY_FUNCTION__ isn't treated as a string literal
            // like __FUNCSIG__ is. _T only works on __FUNCSIG__ itself,
            // again, because a fix was specifically introduced to handle it
            // as described above (to make it compatible with VC++). Without
            // the _T macro we therefore can't (easily) convert
            // __PRETTY_FUNCTION__ to UTF-16 when using non-Microsoft
            // compilers, in particular since the following function (and all
            // others in the class) is "constexpr" so the conversion to
            // UTF-16 needs to be done at compile-time (which is harder to do
            // than at runtime so more trouble than it's worth).
            // __PRETTY_FUNCTION__ therefore can't be used unless we work
            // around the _T situation. For more information on _T in general
            // see here at this writing (for starters):
            //
            //     Unicode Programming Summary
            //     https://learn.microsoft.com/en-us/cpp/text/unicode-programming-summary?view=msvc-170
            //
            // The bottom line is that when _MSC_VER is #defined, the
            // following function assumes the build environment is compatible
            // with VC++ even if a non-Microsoft compiler is actually being
            // used (such as when the GCC, Clang or Intel compiler is
            // explicitly installed as an optional component in Microsoft
            // Visual Studio on Windows, and then selected as the compiler to
            // use by going into your C++ project's properties, selecting
            // "Configuration properties -> General" and choosing the
            // applicable compiler from the "Platform toolset" dropdown). If
            // "LLVM (clang-cl)" is selected for instance (the documented
            // name of the Clang compiler at this writing - see
            // https://learn.microsoft.com/en-us/cpp/build/clang-support-msbuild?view=msvc-170
            // though the name is subject to change by MSFT of course), then
            // the Clang compiler will be used instead of the native VC++
            // compiler. Although our own #defined constant CLANG_COMPILER
            // will then be defined in this case (instead of the
            // MICROSOFT_COMPILER since VC++ itself is no longer the chosen
            // compiler), the native Microsoft constant _MSC_VER will still
            // be #defined which we check for just below (instead of
            // CLANG_COMPILER). __FUNCSIG__ will then be used as seen below
            // (and _T applied to it as seen), since _T will still correctly
            // work on it as described above, even though __FUNCSIG__ isn't a
            // string literal (unlike when the actual Microsoft VC++ compiler
            // is used). Again, we can't rely on __PRETTY_FUNCTION__ in this
            // case even though it will also be #defined since _T can't be
            // applied to it (since it's not a string literal and no
            // work-around was made by the compiler vendors to treat it like
            // a string literal, unlike in the __FUNCSIG__ case).
            //
            // Lastly, note that __FUNCSIG__ isn't formatted the same way for
            // non-Microsoft compilers as it is for VC++ itself. In
            // non-Microsoft compilers, __FUNCSIG__ is formatted with the
            // trailing "[T = float]" syntax (using the examples seen at the
            // top of this comment block), so it follows the same basic
            // format as __PRETTY_FUNCTION__ does in this regard (though the
            // overall format of __FUNCSIG__ does differ slightly from
            // __PRETTY_FUNCTION__ but the differences are cosmetic only so
            // it doesn't affect our code in anyway - __FUNCSIG__ includes
            // the calling convention for instance and __PRETTY_FUNCTION__
            // doesn't but this has no impact on any of the code in this or
            // any other function in this class).
            //////////////////////////////////////////////////////////////////
            template <typename T>
            #if defined(MICROSOFT_COMPILER)
            static constexpr auto GetPrettyFunction(T) noexcept
            #else
            static constexpr auto GetPrettyFunction() noexcept
            #endif
            {
                ///////////////////////////////////////////////////////////////
                // MSFT? Note the following (_MSC_VER) is also #defined by
                // non-Microsoft compilers when they're targeting Windows
                // (i.e., when they're running in Microsoft VC++ compatibility
                // mode). We therefore use __FUNCSIG__ in those cases as well.
                // even though those compilers also declare __PRETTY_FUNCTION__
                // (but we don't rely on the latter when _MSC_VER is #defined
                // since we just assume we're dealing with VC++ - the
                // behavior of other compilers running in Microsoft
                // compatibility mode should normally be close enough for our
                // purpose that any differences won't impact us - if they do
                // then it's normally rare and we provide special handling to
                // deal with it). See the section "Non-Microsoft compilers
                // targeting Windows" in long comments above.
                ///////////////////////////////////////////////////////////////
                #if defined(_MSC_VER)
                    return tstring_view(_T(__FUNCSIG__));
                ////////////////////////////////////////////////////////////////
                // Just some useful info on the subject of __PRETTY_FUNCTION__
                // (mainly to do with GCC but provides additional insight):
                //
                //    https://gcc.gnu.org/onlinedocs/gcc/Function-Names.html
                //    https://gcc.gnu.org/onlinedocs/gcc-3.2.3/gcc/Function-Names.html // Earlier version of above but with a bit more info
                //    https://stackoverflow.com/questions/55850013/pretty-function-in-constant-expression
                //    https://gcc.gnu.org/bugzilla/show_bug.cgi?id=66639
                //    https://github.com/gcc-mirror/gcc/blob/41d6b10e96a1de98e90a7c0378437c3255814b16/gcc/cp/NEWS#L337
                //
                // Note that no official documentation seems to exist for Clang
                // or Intel but the latter effectively relies on the Clang
                // front-end anyway now (according to the Intel DPC++/C++ docs
                // so presumably we should rely on the Clang docs but again,
                // nothing about __PRETTY_FUNCTION is mentioned in those docs
                // at this writing).
                ////////////////////////////////////////////////////////////////
                #elif defined(GCC_COMPILER) || \
                      defined(CLANG_COMPILER) || \
                      defined(INTEL_COMPILER)
                    return tstring_view(__PRETTY_FUNCTION__); // Can't use _T on __PRETTY_FUNCTION__ here
                                                              // as described in (long) function comments
                                                              // above. __PRETTY_FUNCTION__ and "tstring_view"
                                                              // are both char-based" strings at this point
                                                              // however so we're ok (still potentially
                                                              // brittle though but we'll live with it for
                                                              // now - should be safe unless this ever
                                                              // changes)
                #else
                    /////////////////////////////////////////////////////
                    // Note: Shouldn't be possible to come through here
                    // at this writing since the same #error message
                    // would have already been displayed earlier (when
                    // the compiler constants just above were #defined
                    // in "CompilerVersions.h")
                    /////////////////////////////////////////////////////
                    #error "Unsupported compiler (GCC, Microsoft, Clang and Intel are the only ones supported at this writing)"
                #endif
            }

    ///////////////////////////////////////////////////////////
    // #define TYPENAME_V_IMPL_2 to use a slightly different
    // implementation. Both produce the same results however
    // and there's no significant advantage of using one over
    // the other. By default it's not #defined which might
    // yield a slightly more efficient implementation (I only
    // suspect) but the difference is negligible. Either
    // implementation is fine (have never investigated which
    // is really better since the difference is miniscule)
    ///////////////////////////////////////////////////////////
    #if !defined(TYPENAME_V_IMPL_2)
        private:
            template <typename T = void>
            static constexpr auto GetOffsetAndLen(tstring_view prettyFunction) noexcept
            {
                //////////////////////////////////////////////////
                // Always passing a "float" here since any type
                // will do (offset we'll be calculating below
                // is always the same regardless of the type).
                // See comments above.
                //////////////////////////////////////////////////
                #if defined(MICROSOFT_COMPILER)
                constexpr tstring_view prettyFunctionFloat = GetPrettyFunction<float>(0.0);
                #else
                constexpr tstring_view prettyFunctionFloat = GetPrettyFunction<float>();
                #endif

                #if defined(GCC_COMPILER)
                    //////////////////////////////////////////////////////////////////
                    // See format of __PRETTY_FUNCTION__ string for "GCC" near the
                    // top of the comments in "GetPrettyFunction()". There are 2
                    // possible formats labeled "1 of 2" and "2 of 2" in the comments
                    // (with an explanation of when each kicks in - see comments for
                    // details). The offset to the type in this string is the same
                    // regardless of these two formats however (for whatever format
                    // is in use), and regardless of the type itself, so we arbitrarily
                    // use float in the code below (again, see comments in
                    // "GetPrettyFunction()" for details). For format "1 of 2" the
                    // string always ends with "= float]" so the offset to the "f" in
                    // this string (i.e., the offset to the type we're after) is
                    // always 6 characters from the end of the string (unless the
                    // compiler vendor makes a breaking change to the string but the
                    // "static_assert" seen just after this class will trap it). This
                    // is identical to the "Clang" and "Intel" case further below so
                    // we do the same check here for GCC. If the following check for
                    // format "1 of 2" is true then the offset will be the same no
                    // matter what the type so we're good (i.e., we can just rely on
                    // the one for float and immediately return this). Unlike "Clang"
                    // and "Intel" however, where the following check must always be
                    // true so we don't need to check for it in those cases (we simply
                    // "static_assert" on it instead), if it's not true for GCC then
                    // we must be dealing with format "2 of 2" instead. In this case
                    // we need to locate "= float;" within the string and the offset
                    // to the type itself is therefore 2 characters after the '='
                    // sign. Again, just like the "1 of 2" case, the offset to the
                    // type will be the same in the "2 of 2" case no matter what the
                    // type so we can just rely on the one for float and immediately
                    // return this.
                    //////////////////////////////////////////////////////////////////

                    // GCC format 1 of 2 (see "GetPrettyFunction()" for details)
                    if constexpr (EndsWith(prettyFunctionFloat, _T("= float]")))
                    {
                        ///////////////////////////////////////////
                        // Offset to the "f" in "= float]" (i.e.,
                        // the 1st character of the type we're
                        // after). Always the same regardless of
                        // the type so our use of "float" here to
                        // calculate it will work no matter what
                        // the type.
                        ///////////////////////////////////////////
                        constexpr auto offset = prettyFunctionFloat.size() - 6;

                        //////////////////////////////////////////////
                        // Length of the type (subtracting 1 targets
                        // the ']' in "= float]", i.e., we get an
                        // offset to that ']' so subtracting "offset"
                        // from that yields the length of the type
                        // itself, i.e. everything from "offset"
                        // INclusive to ']' EXclusive)
                        //////////////////////////////////////////////
                        const auto len = prettyFunction.size() - 1 - offset;

                        return std::make_pair(offset, len);
                    }
                    else // GCC format 2 of 2 (see "GetPrettyFunction()" for details)
                    {
                        constexpr tstring_view::size_type offsetOfEqualSign = prettyFunctionFloat.rfind(_T("= float;"));
                        static_assert(offsetOfEqualSign != tstring_view::npos &&
                                      AlwaysTrue_v<T>); // "AlwaysTrue_v" required to make the "static_assert" depend
                                                        // on template arg "T" so that the "static_assert" won't
                                                        // trigger when the "if constexpr" call above is true (and
                                                        // it always will trigger in older versions of C++ in
                                                        // particular - in newer versions however the call to
                                                        // "AlwaysTrue_v" is no longer necessary since the language
                                                        // or possibly GCC itself (not certain, it's a fuzzy area)
                                                        // was changed so that the call to "static_assert" now
                                                        // implicitly relies on "T" without having to call
                                                        // "AlwaysTrue_v" - we continue to rely on it anyway for
                                                        // now)

                        ///////////////////////////////////////////
                        // Offset to the "f" in "= float;" (i.e.,
                        // the 1st character of the type we're
                        // after). Always the same regardless of
                        // the type so our use of "float" above to
                        // calculate it will work no matter what
                        // the type.
                        ///////////////////////////////////////////
                        constexpr auto offset = offsetOfEqualSign + 2;

                        ////////////////////////////////////////////////////
                        // Length of the type. Note that this calculation
                        // works for all compilers but we don't use it for
                        // them. It's faster and more efficient to
                        // calculate the length as seen for those compilers
                        // though the difference is negligible (likely on
                        // the order of nanoseconds). For this particular
                        // GCC scenario however (GCC format 2 of 2), we do
                        // rely on it since we'd otherwise have to search
                        // for the closing ';' that ends the type, which is
                        // non-trivial since the type name itself can
                        // potentially contain a ';' character (rare as
                        // this will normally be). A forward search for the
                        // closing ';' would therefore have to bypass any
                        // possible ';' characters in the type itself,
                        // again, which is non-trivial. A backwards search
                        // from the end of the string for the ';' would
                        // therefore be easier but even this is potentially
                        // brittle and a waste of time anyway, since the
                        // the following technique easily calculates the
                        // length without having to deal with these issues.
                        // Note that we don't use this calculation for all
                        // other compiler scenarios however (even though it
                        // would also work) since the type's terminating
                        // character in those cases is always at a known,
                        // fixed location so no searching for it is ever
                        // required. It's therefore trivial to calculate
                        // the length for all other compiler scenarios
                        // using the techniques seen for them, though the
                        // following technique would yield the same result
                        // as noted (and the following technique is trivial
                        // in its own right, though a tiny bit less so than
                        // the technique we rely on for all other compiler
                        // scenarios - in any case these are micro-optimizations
                        // only so the impact isn't relevant).
                        ////////////////////////////////////////////////////
                        const auto len = prettyFunction.size() > prettyFunctionFloat.size()
                                         ? (5 + (prettyFunction.size() - prettyFunctionFloat.size()))
                                         : (5 - (prettyFunctionFloat.size() - prettyFunction.size()));

                        return std::make_pair(offset, len);
                    }

                ////////////////////////////////////////////////////////
                // Note: Not checking for _MSC_VER here by design, only
                // for the native Microsoft VC+++ compiler itself. Code
                // just below therefore applies to VC++ only, not to
                // other compilers we support that also #define
                // _MSC_VER (when they're running in Microsoft VC++
                // compatibility mode). They don't format __FUNCSIG__
                // the same way VC++ does so we can't rely on the VC++
                // code just below for them. Note that they *should*
                // format it the same way since they're supposed to be
                // compatible with VC++ when _MSC_VER is #defined but
                // they're not.
                ////////////////////////////////////////////////////////
                #elif defined(MICROSOFT_COMPILER)
                    ///////////////////////////////////////////////////////////////////
                    // See format of __FUNCSIG__ string for "Microsoft (VC++)" near
                    // the top of the comments in "GetPrettyFunction()". Offset to the
                    // type in this string is the same regardless of the type so we
                    // arbitrarily use "float" here (see comments preceding latter
                    // function for details). The __FUNCSIG__ string with type "float"
                    // always ends with "<float>(void) noexcept" so the offset to the
                    // "f" in this string (i.e., the offset to the type we're after)
                    // is always 21 characters from the end (unless MSFT makes a
                    // breaking change to the string). The offset will be the same no
                    // matter what the type so we're good (i.e., we can just rely on
                    // the one for float and immediately return this).
                    ///////////////////////////////////////////////////////////////////
                    static_assert(EndsWith(prettyFunctionFloat, _T("<float>(float) noexcept")));

                    /////////////////////////////////////////////////////
                    // Offset to the "f" in "<float>(void) noexcept"
                    // (i.e., the 1st character of the type we're
                    // after). Always the same regardless of the type so
                    // our use of "float" to calculate it will work no
                    // matter what the type. Note that hardcoding 21
                    // here may seem brittle but it is in fact stable.
                    // If it ever breaks because MSFT changes the format
                    // of __FUNCSIG__ (or we encounter some previously
                    // unknown situation with its current format) than
                    // the "static_assert" just above will trigger so
                    // we're safe. I therefore consider it unnecessary
                    // to search through the string itself to find the
                    // offset to the start of the type when the
                    // following should always work normally (and
                    // quicker too though the difference would be
                    // negligible). Searching though the string is both
                    // a waste of time and potentially brittle as well,
                    // since if the format of __FUNCSIG__ ever changes
                    // then the search might fail or return some
                    // erroneous value. You would therefore still need
                    // to trigger a "static_assert" to trap it. If our
                    // own "static_assert" just above ever triggers we
                    // can review the situation then.
                    /////////////////////////////////////////////////////
                    constexpr auto offset = prettyFunctionFloat.size() - 22;

                    ///////////////////////////////////////////////////
                    // Length of the type (subtracting 16 targets the
                    // '>' in "<float>(void) noexcept", i.e., we get
                    // an offset to that '>' so subtracting "offset"
                    // from that yields the length of the type itself,
                    // i.e., everything between '<' and '>')
                    ///////////////////////////////////////////////////
                    const auto len = prettyFunction.size() - 17 - offset;

                    return std::make_pair(offset, len);
                #elif defined(CLANG_COMPILER) || defined(INTEL_COMPILER)
                    ///////////////////////////////////////////////////////////////////
                    // See format of the strings for Clang and Intel near the top of
                    // the comments in "GetPrettyFunction()" (using __PRETTY_FUNCTION
                    // when _MSC_VER isn't #defined or __FUNCSIG__ otherwise). Offset
                    // to the type in this string is the same regardless of the type
                    // so we arbitrarily use "float" here (see "GetPrettyFunction()"
                    // comments for details). Both __PRETTY_FUNCTION__ and __FUNCSIG__
                    // (based on type "float" we're applying here) always ends with
                    // "= float]" so the offset to the "f" in this string (i.e., the
                    // offset to the type we're after) is always 6 characters from the
                    // end (unless the compiler vendors makes a breaking change to the
                    // string, triggering this "static_assert"). The offset will be the
                    // same no matter what the type so we can just rely on the one for
                    // float and immediately return this.
                    ///////////////////////////////////////////////////////////////////
                    static_assert(EndsWith(prettyFunctionFloat, _T("= float]")));

                    //////////////////////////////////////////////////////
                    // Offset to the "f" in "= float]" (i.e., the 1st
                    // character of the type we're after). Always the
                    // same regardless of the type so our use of "float"
                    // to calculate it will work no matter what the
                    // type. Note that hardcoding 6 here may seem brittle
                    // but it's normally very stable. See the comments in
                    // the MICROSOFT_COMPILER code above (in its own
                    // declaration of "offset" - same thing applies here
                    // as well though applicable to "Clang" and "Intel"
                    // in this case)
                    //////////////////////////////////////////////////////
                    constexpr auto offset = prettyFunctionFloat.size() - 6;

                    //////////////////////////////////////////////
                    // Length of the type (subtracting 1 targets
                    // the ']' in "= float]", i.e., we get an
                    // offset to that ']' so subtracting "offset"
                    // from that yields the length of the type
                    // itself, i.e. everything from "offset"
                    // INclusive to ']' EXclusive)
                    //////////////////////////////////////////////
                    const auto len = prettyFunction.size() - 1 - offset;

                    return std::make_pair(offset, len);
                #else
                    /////////////////////////////////////////////////////
                    // Note: Shouldn't be possible to come through here
                    // at this writing since the same #error message
                    // would have already been displayed earlier (when
                    // the compiler constants just above were #defined
                    // in "CompilerVersions.h")
                    /////////////////////////////////////////////////////
                    #error "Unsupported compiler (GCC, Microsoft, Clang and Intel DPC++/C++ are the only ones supported at this writing)"
                #endif
            }

        protected:
            template <typename T = void>
            static constexpr tstring_view ExtractTypeNameFromPrettyFunction(tstring_view prettyFunction) noexcept
            {
                //////////////////////////////////////////
                // Returns a "std::pair" containing the
                // offset and length of the type name in
                // "prettyFunction" (in members "first"
                // and "second" respectively).
                //////////////////////////////////////////
                const auto offsetAndLen = GetOffsetAndLen(prettyFunction);

                return prettyFunction.substr(offsetAndLen.first, // Offset of the type name in "prettyFunction"
                                             offsetAndLen.second); // Length of the type name in "prettyFunction"
            }
    #else
        private:
            //////////////////////////////////////////////////////////////////////
            // GetTypeNameOffset(). Returns the offset within __PRETTY_FUNCTION__
            // or (when _MSC_VER is #defined) __FUNCSIG__, to the start of the
            // type name within the latter string. Note that as explained in
            // the comments preceding "GetPrettyFunction()" (see this for
            // details), the offset to the type's name within the latter
            // strings is always identical regardless of the type so we can
            // calculate it using any type. No need to do it for any specific
            // type that is but we create the following function as a template
            // anyway, with template arg "T" supposedly representing the type
            // we're targeting (but it's not actually used for that purpose -
            // read on). IOW, we ignore "T" for purposes of calculating the
            // type name's offset and always rely on "float" instead, as
            // described in the "GetPrettyFunction" comments (again, since the
            // type doesn't matter - the result is always the same). We create
            // the following function as a template anyway not because we need
            // to know the type to calculate its offset, but only so that we
            // can make any "static_asserts" in the code below depend on
            // template arg "T" when required. That is, because of how
            // "static_assert" works in C++, if it's used outside a
            // template-based context it will always trigger if its first arg
            // is false. For instance, see the "if constexpr" clause in the
            // GCC_COMPILER code below. When that "if constexpr" condition is
            // true, the "static_assert" seen in the corresponding "else"
            // clause would trigger if this function wasn't a template because
            // the arg being passed to that "static_assert" is false whenever
            // the "if constexpr" condition itself is true. If not for the
            // function's dependence on template arg "T" itself in the
            // "static_assert" (the 1st arg of "static_assert" depends on "T"
            // via a call to "AlwaysTrue_v<T>" - see the call to this below for
            // details), the "static_assert" would always trigger even when
            // the "else" clause isn't in effect (because the corresponding
            // "if constexpr" is true). Because of the "static_assert"
            // dependence on a template arg however (again, via the call seen
            // to "AlwaysTrue_v<T>" in that "static_assert"), the
            // "static_assert" in the "else" clause therefore won't trigger,
            // which is what we require. The bottom line is that while it
            // shouldn't be necessary to make the following function a
            // template since we never use the "T" template arg (we always
            // rely on "float" a described), in order to prevent the
            // "static_assert" from always triggering we make it depend on
            // template arg "T" (so the function itself needs to be a template
            // to accommodate this - yet another C++ joy).
            //////////////////////////////////////////////////////////////////////
            template <typename T = void>
            static constexpr tstring_view::size_type GetTypeNameOffset() noexcept
            {
                //////////////////////////////////////////////////
                // Always passing a "float" here since any type
                // will do (offset we'll be calculating below
                // is always the same regardless of the type).
                // See comments above.
                //////////////////////////////////////////////////
                constexpr tstring_view prettyFunctionFloat = GetPrettyFunction<float>();

                #if defined(GCC_COMPILER)
                    //////////////////////////////////////////////////////////////////
                    // See format of __PRETTY_FUNCTION__ string for "GCC" near the
                    // top of the comments in "GetPrettyFunction()". There are 2
                    // possible formats labeled "1 of 2" and "2 of 2" in the comments
                    // (with an explanation of when each kicks in - see comments for
                    // details). The offset to the type in this string is the same
                    // regardless of these two formats however (for whatever format
                    // is in use), regardless of the type itself, so we arbitrarily
                    // use float in the code below (again, see comments in
                    // "GetPrettyFunction()" for details). For format "1 of 2" the
                    // string always ends with "= float]" so the offset to the "f" in
                    // this string (i.e., the offset to the type we're after) is
                    // always 6 characters from the end of the string (unless the
                    // compiler vendor makes a breaking change to the string but the
                    // "static_assert" seen just after this class will trap it). This
                    // is identical to the "Clang" and "Intel" case further below so
                    // we do the same check here for GCC. If the following check for
                    // format "1 of 2" is true then the offset will be the same no
                    // matter what the type so we're good (i.e., we can just rely on
                    // the one for float and immediately return this). Unlike "Clang"
                    // and "Intel" however, where the following check must always be
                    // true so we don't need to check for it in those cases (we simply
                    // "static_assert" on it instead), if it's not true for GCC then
                    // we must be dealing with format "2 of 2" instead. In this case
                    // we need to locate "= float;" within the string and the offset
                    // to the type itself is therefore 2 characters after the '='
                    // sign. Again, just like the "1 of 2" case, the offset to the
                    // type will be the same in the "2 of 2" case no matter what the
                    // type so we can just rely on the one for float and immediately
                    // return this.
                    //////////////////////////////////////////////////////////////////

                    // GCC format 1 of 2 (see "GetPrettyFunction()" for details)
                    if constexpr (EndsWith(prettyFunctionFloat, _T("= float]")))
                    {
                        ///////////////////////////////////////////
                        // Offset to the "f" in "= float]" (i.e.,
                        // the 1st character of the type we're
                        // after). Always the same regardless of
                        // the type so our use of "float" here to
                        // calculate it will work no matter what
                        // the type.
                        ///////////////////////////////////////////
                        return prettyFunctionFloat.size() - 6;
                    }
                    else // GCC format 2 of 2 (see "GetPrettyFunction()" for details)
                    {
                        constexpr tstring_view::size_type offsetOfEqualSign = prettyFunctionFloat.rfind(_T("= float;"));
                        static_assert(offsetOfEqualSign != tstring_view::npos &&
                                      AlwaysTrue_v<T>); // "AlwaysTrue_v" required to make the "static_assert" depend
                                                        // on template arg "T" so that the "static_assert" won't
                                                        // trigger when the "if constexpr" call above is true (and
                                                        // it always will trigger in older versions of C++ in
                                                        // particular - in newer versions however the call to
                                                        // "AlwaysTrue_v" is no longer necessary since the language
                                                        // or possibly GCC itself (not certain, it's a fuzzy area)
                                                        // was changed so that the call to "static_assert" now
                                                        // implicitly relies on "T" without having to call
                                                        // "AlwaysTrue_v" - we continue to rely on it anyway for
                                                        // now)

                        ///////////////////////////////////////////
                        // Offset to the "f" in "= float;" (i.e.,
                        // the 1st character of the type we're
                        // after). Always the same regardless of
                        // the type so our use of "float" above to
                        // calculate it will work no matter what
                        // the type.
                        ///////////////////////////////////////////
                        return offsetOfEqualSign + 2;
                    }
                ////////////////////////////////////////////////////////
                // Note: Not checking for _MSC_VER here by design, only
                // for the native Microsoft VC+++ compiler itself. Code
                // just below therefore applies to VC++ only, not to
                // other compilers we support that also #define
                // _MSC_VER (when they're running in Microsoft VC++
                // compatibility mode). They don't format __FUNCSIG__
                // the same way VC++ does so we can't rely on the VC++
                // code just below for them. Note that they *should*
                // format it the same way since they're supposed to be
                // compatible with VC++ when _MSC_VER is #defined but
                // they're not.
                ////////////////////////////////////////////////////////
                #elif defined(MICROSOFT_COMPILER)
                    ///////////////////////////////////////////////////////////////////
                    // See format of __FUNCSIG__ string for "Microsoft (VC++)" near
                    // the top of the comments in "GetPrettyFunction()". Offset to the
                    // type in this string is the same regardless of the type so we
                    // arbitrarily use "float" here (see comments preceding latter
                    // function for details). The __FUNCSIG__ string with type "float"
                    // always ends with "<float>(void) noexcept" so the offset to the
                    // "f" in this string (i.e., the offset to the type we're after)
                    // is always 21 characters from the end (unless MSFT makes a
                    // breaking change to the string). The offset will be the same no
                    // matter what the type so we're good (i.e., we can just rely on
                    // the one for float and immediately return this).
                    ///////////////////////////////////////////////////////////////////
                    static_assert(EndsWith(prettyFunctionFloat, _T("<float>(void) noexcept")));

                    //////////////////////////////////////////////////
                    // Offset to the "f" in "<float>(void) noexcept"
                    // (i.e., the 1st character of the type we're
                    // after). Always the same regardless of the type
                    // so our use of "float" to calculate it will
                    // work no matter what the type.
                    //////////////////////////////////////////////////
                    return prettyFunctionFloat.size() - 21;
                #elif defined(CLANG_COMPILER) || defined(INTEL_COMPILER)
                    ///////////////////////////////////////////////////////////////////
                    // See format of the strings for Clang and Intel near the top of
                    // the comments in "GetPrettyFunction()" (using __PRETTY_FUNCTION
                    // when _MSC_VER isn't #defined or __FUNCSIG__ otherwise). Offset
                    // to the type in this string is the same regardless of the type
                    // so we arbitrarily use "float" here (see "GetPrettyFunction()"
                    // comments for details). Both __PRETTY_FUNCTION__ and __FUNCSIG__
                    // (based on type "float" we've applied here) always ends with
                    // "= float]" so the offset to the "f" in this string (i.e., the
                    // offset to the type we're after) is always 6 characters from the
                    // end (unless the compiler vendors makes a breaking change to the
                    // string, triggering this "static_assert"). The offset will be the
                    // same no matter what the type so we can just rely on the one for
                    // float and immediately return this.
                    ///////////////////////////////////////////////////////////////////
                    static_assert(EndsWith(prettyFunctionFloat, _T("= float]")));

                    //////////////////////////////////////////////////////
                    // Offset to the "f" in "= float]" (i.e., the 1st
                    // character of the type we're after). Always the
                    // same regardless of the type so our use of "float"
                    // to calculate it will work no matter what the type.
                    //////////////////////////////////////////////////////
                    return prettyFunctionFloat.size() - 6;
                #else
                    /////////////////////////////////////////////////////
                    // Note: Shouldn't be possible to come through here
                    // at this writing since the same #error message
                    // would have already been displayed earlier (when
                    // the compiler constants just above were #defined
                    // in "CompilerVersions.h")
                    /////////////////////////////////////////////////////
                    #error "Unsupported compiler (GCC, Microsoft, Clang and Intel are the only ones supported at this writing)"
                #endif
            }

            ////////////////////////////////////////////////////////////////
            // *** IMPORTANT ***
            //
            // Must be declared before 1st use or compilation will fail in
            // some compilers like Clang (declaration order of function
            // templates matters).
            //
            // GetTypeNameLen(). Returns the length of the type name within
            // "prettyFunction" where the latter string was returned via a
            // call to "GetPrettyFunction()" (so it originates from
            // __PRETTY_FUNCTION__ or (when _MSC_VER is #defined)
            // __FUNCSIG__). If the type name within "prettyFunction" is
            // (literally) "int" for instance then it returns 3. Note that
            // as explained in the comments preceding function
            // "GetPrettyFunction()", we leverage our knowledge of type
            // float to calculate the length of the type name in
            // "prettyFunction". See "GetPrettyFunction()" for details.
            ////////////////////////////////////////////////////////////////
            static constexpr tstring_view::size_type GetTypeNameLen(tstring_view prettyFunction) noexcept
            {
                constexpr tstring_view prettyFunctionFloat = GetPrettyFunction<float>();

                ///////////////////////////////////////////////////////////////
                // See full explanation in "GetPrettyFunction()" comments.
                // Consult section entitled:
                //
                //     Reliance on "float" to calculate the offset and length
                //
                // As described there, the length of the type name in
                // "prettyFunction" is determined by simply taking the
                // difference (delta) in the length of "prettyFunction" and
                // the length of the pretty function for a "float", and adding
                // this delta to the length of "float" itself (5). "float" was
                // chosen for this purpose but any type with a consistent
                // length among all supported compilers will do. Again, see
                // comments in "GetPrettyFunction()" for details.
                //
                // Lastly, note that since the following is dealing with
                // unsigned numbers, I've coded things to avoid the
                // possibility of a negative number when the length of the
                // pretty strings are subtracted from each other (if we simply
                // did the subtraction without checking for a possible
                // negative result). Subtracting a potentially larger unsigned
                // number from a smaller one gets into the murky area of
                // "negative" unsigned numbers which are never actually
                // negative of course, but simply large unsigned numbers (due
                // to a wrap-around). It's a confusing area and also gets into
                // the possibility of undefined behavior (won't get into that
                // here), so it's much simpler to just avoid this subtraction
                // and code things as seen instead (checking to make sure we
                // don't wind up with a "negative" result so the whole
                // situation avoided).
                ///////////////////////////////////////////////////////////////

                /////////////////////////////////////////////////////////////////
                // If true then the length of the type name in "prettyFunction"
                // is greater than the length of "float" itself (i.e., > 5)
                /////////////////////////////////////////////////////////////////
                if (prettyFunction.size() > prettyFunctionFloat.size())
                {
                    return 5 + (prettyFunction.size() - prettyFunctionFloat.size());
                }
                ////////////////////////////////////////////////////////////
                // Length of type name in "prettyFunction" is less than or
                // equal to the length of "float" itself (i.e., <= 5)
                ////////////////////////////////////////////////////////////
                else
                {
                    return 5 - (prettyFunctionFloat.size() - prettyFunction.size());
                }
            }

        protected:
            static constexpr tstring_view ExtractTypeNameFromPrettyFunction(tstring_view prettyFunction) noexcept
            {
                return prettyFunction.substr(GetTypeNameOffset(), // Offset of the type name in "prettyFunction"
                                             GetTypeNameLen(prettyFunction)); // Length of the type name in "prettyFunction"
            }
        
    #endif // if !defined(TYPENAME_V_IMPL_2)
        }; // class TypeNameImplBase

        //////////////////////////////////////////////////////////////////////
        // Implementation class for variable template "TypeName_v" declared
        // just after this class but outside of this "Private" namespace (so
        // for public use). The latter variable just invokes static member
        // "Get()" below which carries out all the work. See "TypeName_v" for
        // complete details.
        //
        // IMPORTANT:
        // ---------
        // Note that the implementation below relies on the predefined string
        // __PRETTY_FUNCTION__ or (when _MSC_VER is #defined) __FUNCSIG__
        // (Google these for details). All implementations you can find on
        // the web normally rely on these as does our own "Get()" member
        // below, but unlike most other implementations I've seen, ours
        // doesn't require any changes should you modify any part of the
        // latter function's fully-qualified name or signature (affecting the
        // value of the above predefined strings). Most other implementations
        // I've seen would require changes, even though they should normally
        // be very simple changes (trivial usually but changes nevertheless).
        // Our implementation doesn't require any so users can move the
        // following code to another namespace if they wish, change the
        // function's class name and/or any of its member functions without
        // breaking anything (well, except for its "noexcept" specifier on
        // MSFT platforms only, but nobody will ever need to change this
        // anyway, regardless of platform). Note that the code is fairly
        // small and clean notwithstanding first impressions, lengthy only
        // because of the many comments (the code itself is fairly short and
        // digestible however). It will only break normally if a compiler
        // vendor makes a breaking change to __PRETTY_FUNCTION__ or (when
        // _MSC_VER is #defined) __FUNCSIG__, but this will normally be
        // caught by judicious use of "static_asserts" in the implementation
        // and just after the following class itself (where we arbitrarily
        // test it with a float to make sure it returns "float", a quick and
        // dirty test but normally reliable).
        //////////////////////////////////////////////////////////////////////
        template <typename T>
        class TypeNameImpl : public TypeNameImplBase
        {
        public:
            ////////////////////////////////////////////////////////
            // Implementation function called by variable template
            // "TypeName_v" just after the "Private" namespace this
            // class is declared in. The following function does
            // all the work. See "TypeName_v" for details.
            ////////////////////////////////////////////////////////
            static constexpr tstring_view Get() noexcept
            {
                ///////////////////////////////////////////////////////
                // TYPENAME_V_DONT_MINIMIZE_REQD_SPACE isn't #defined
                // by default so the following normally tests true. If
                // so then the type's name is extracted from
                // __PRETTY_FUNCTION__ or (on MSFT platforms)
                // __FUNCSIG__ and statically stored in "m_TypeName"
                // (a "std::array" just big enough to store the
                // extracted type name plus 1 extra character for a
                // NULL terminator - we always add one in case callers
                // require it). We then simply return a "tstring_view"
                // that wraps "m_TypeName" though its "size()" member
                // doesn't include the NULL terminator itself, as
                // would normally be expected (but the NULL terminator
                // is still safely present in case someone invokes the
                // "data()" member for instance - the string is
                // therefore always safely NULL terminated). The
                // upshot is that __PRETTY_FUNCTION__ or __FUNCSIG__
                // is therefore *not* stored in the compiled binary as
                // they normally would be as compile time (static)
                // strings. They're removed by all supported compilers
                // entirely since they're not being used at runtime
                // and "m_TypeName" is stored instead, which only
                // takes up the minimum space required to store the
                // type name (plus a NULL terminator). We therefore
                // make this the default behavior though the savings
                // is usually negligible (but no point storing all of
                // __PRETTY_FUNCTION__ or __FUNCSIG__ when we only
                // ever target that portion of it containing the type
                // name itself - "m_TypeName" contains a copy of it so
                // there's zero overhead). However, should it ever
                // become necessary for some reason (not normally),
                // users can simply #define
                // TYPENAME_V_DONT_MINIMIZE_REQD_SPACE which removes
                // "m_TypeName" (preprocesses it out), and resorts to
                // storing __PRETTY_FUNCTION__ or __FUNCSIG__ again
                // (as static strings in the compiled binary). We then
                // return a "tstring_view" into these strings instead,
                // targeting the type name within the string. Caution
                // advised however (!) since it's no longer NULL
                // terminated as a result (because it's just a
                // substring view of the type name within
                // __PRETTY_FUNCTION__ or __FUNCSIG__ so no NULL
                // terminator will be present in the latter strings at
                // the character following the substring). Note that
                // the unused remainder of __PRETTY_FUNCTION__ or
                // __FUNCSIG__ within the compiled binary also becomes
                // a waste of space (since the caller is only ever
                // targeting the type name within these strings).            
                // Contrast this behavior with the usual default
                // behavior described above, where the only string
                // stored in the binary is "m_TypeName" (always NULL
                // terminated), and this only consumes the actual
                // (minimal) required space to store the type name
                // (plus a NULL terminator). The returned
                // "tstring_view" targets that instead so most users
                // should stick with this behavior and not #define
                // TYPENAME_V_DONT_MINIMIZE_REQD_SPACE unless there's
                // a compelling reason to (but if you do, it's no
                // longer a null-terminated string as noted so
                // exercise caution).
                ///////////////////////////////////////////////////////
                #if !defined(TYPENAME_V_DONT_MINIMIZE_REQD_SPACE)
                    //////////////////////////////////////////////////////
                    // Return static member "m_TypeName" (a "std::array"
                    // storing the type name) as a "tstring_view" (latter
                    // more convenient for end-users to work with than a
                    // "std::array"). Note that current C++ rules don't
                    // allow static objects to be defined in constexpr
                    // functions so we can't define it here (within this
                    // function). Static class members *are* allowed
                    // however so we do that instead.
                    //////////////////////////////////////////////////////
                    return TypeNameArrayToStringView();
                #else
                    /////////////////////////////////////////////////
                    // Extract template arg "T" (whatever string it
                    // resolves to) from __PRETTY_FUNCTION__ or (for
                    // MSFT only) __FUNCSIG__. Returns it as a
                    // "tstring_view" which is safe to return since
                    // the above strings remain alive for the life
                    // of the app (since they're always statically
                    // defined).
                    /////////////////////////////////////////////////
                    return ExtractTypeNameFromPrettyFunction();
                #endif // !defined(TYPENAME_V_DONT_MINIMIZE_REQD_SPACE)
            }

        private:
            using BaseClass = TypeNameImplBase;

            static constexpr tstring_view ExtractTypeNameFromPrettyFunction() noexcept
            {
                #if defined(MICROSOFT_COMPILER)
                return BaseClass::ExtractTypeNameFromPrettyFunction(GetPrettyFunction<T>(T()));
                #else
                return BaseClass::ExtractTypeNameFromPrettyFunction(GetPrettyFunction<T>());
                #endif
            }

            ///////////////////////////////////////////////
            // Normally tests true (constant not #defined
            // by default). See the same call in "Get()"
            // function above for details (the comments
            // there)
            ///////////////////////////////////////////////
            #if !defined(TYPENAME_V_DONT_MINIMIZE_REQD_SPACE)
                static constexpr tstring_view TypeNameArrayToStringView() noexcept
                {
                    ////////////////////////////////////////////
                    // Return a "tstring_view" wrapping our
                    // "m_TypeName" array member but
                    // subtracting 1 from the array's size so
                    // the NULL terminator isn't included in
                    // the returned "tstring_view" (i.e., its
                    // "size()" member doesn't include it).
                    // However, it's still always present so
                    // users can safely depend on the "data()"
                    // member of the returned "tstring_view"
                    // always pointing to a NULL terminated
                    // string (should they require this). The
                    // "size()" member of the returned
                    // "tstring_view doesn't include the NULL
                    // terminator however, as would normally be
                    // expected (it returns the length of the
                    // actual string only).
                    ////////////////////////////////////////////
                    return tstring_view(m_TypeName.data(), m_TypeName.size() - 1);
                }

                static constexpr auto InitTypeName() noexcept
                {
                    constexpr tstring_view typeName = ExtractTypeNameFromPrettyFunction();

                    ////////////////////////////////////////////////
                    // Create a "std::array" from "typeName" (same
                    // size as "typeName" plus 1 for the NULL
                    // terminator the following function always
                    // manually adds - "typeName" itself is copied
                    // into the array and it's then NULL
                    // terminated). Note that having to pass
                    // "typeName.size()" as a template arg is ugly
                    // given that the function can just call this
                    // itself, but not in a "constexpr" context
                    // which we require (since C++ doesn't support
                    // "constexpr" function parameters at this
                    // writing - see function for details).
                    ////////////////////////////////////////////////
                    return StrToNullTerminatedArray<typeName.size()>(typeName);
                }

                ////////////////////////////////////////////////////////////
                // "auto" resolves to a "std::array" of tchar that stores
                // the type name, where the array's size is just large
                // enough to hold it (but we also add a NULL terminator
                // since some users might require it). If template parameter
                // "T" is an "int" for instance so its length is 3, then
                // "m_TypeName" will be "std::array<tchar, 4>" (one extra
                // character added for the NULL terminator).
                // 
                // Note: This member is implicitly "inline". Standard
                // currently states:
                //
                //    "A function or static data member declared with the
                //    constexpr specifier is implicitly an inline function
                //    or variable"
                ////////////////////////////////////////////////////////////
                static constexpr auto m_TypeName = InitTypeName();
            #endif // #if !defined(TYPENAME_V_DONT_MINIMIZE_REQD_SPACE)
        }; // class TypeNameImpl

        static_assert(IsEqualTo(TypeNameImpl<float>::Get(), _T("float")),
                      "A breaking change was detected in template \"TypeNameImpl::Get()\". The format of the "
                      "predefined string __PRETTY_FUNCTION__ or (when _MSC_VER is #defined) __FUNCSIG__ was "
                      "likely changed by the compiler vendor (though would be very rare). \"TypeNameImpl::Get()\" "
                      "was (arbitrarily) tested with type float but the returned string isn't \"float\" and "
                      "normally should be. The format of __PRETTY_FUNCTION__ (or __FUNCSIG__) was therefore "
                      "(likely) changed since \"TypeNameImpl::Get()\"was written, so its implementation should "
                      "be reviewed and corrected.");
    } // namespace Private

    ////////////////////////////////////////////////////////////////////////
    // TypeName_v. Variable template that returns the literal name of the
    // given template arg T as a compile-time string, suitable for display
    // purposes (WYSIWYG). You can pass any type for T that is to return
    // its name as a "tstring_view". This is always a "std::string_view" on
    // non-MSFT platforms in this release ("tstring_view" always resolves
    // to "std::string_view"), and normally "std::wstring" on MSFT
    // platforms (or more accurately on any compiler that #defines _MSC_VER
    // when they're targeting Windows code). Note that MSFT platforms are
    // normally compiled for UTF-16 based on the #defined Microsoft
    // constants UNICODE and _UNICODE (see these if you're not already
    // familiar), but if your code is in fact compiled for ANSI instead (on
    // MSFT platforms), though it would be very rare these days, then
    // "tstring_view" will correctly resolve to "std::string_view" which we
    // still support. It *always* resolves to "std::string_view" on
    // non-MSFT platforms however (in this release as noted), since
    // supporting anything else means we'd have to deal with the overhead
    // of character conversions (if "wchar_t" or some other character type
    // had to be supported - easier on MSFT platforms only for now, at
    // least for Unicode vs non-Unicode builds, though in reality Unicode
    // builds are almost always the norm there). We can always review the
    // situation for other platforms in a future release if ever required.
    //
    //     EXAMPLE 1
    //     ---------
    //     ///////////////////////////////////////////////////////////
    //     // Returns "float" as a "tstring_view" (literally, quotes
    //     // not included).
    //     ///////////////////////////////////////////////////////////
    //     constexpr tstring_view typeName = TypeName_v<float>;
    //
    //     EXAMPLE 2
    //     ---------
    //     ////////////////////////////////////////////////////////////////
    //     // A type like "std::wstring" produces a far more complicated
    //     // name than the "float" example above of course. For example
    //     // (for all 4 compilers we currently support shown - note that
    //     // the surrounding quotes below are just for legibility and not
    //     // actually returned):
    //     //
    //     //   GCC
    //     //   ---
    //     //   "std::basic_string<wchar_t>" if "-D _GLIBCXX_USE_CXX11_ABI=0" macro is in effect (see https://gcc.gnu.org/onlinedocs/libstdc++/manual/using_dual_abi.html)
    //     //   "std::__cxx11::basic_string<wchar_t>" if above macro is set to 1 instead (again, see above link)
    //     //
    //     //   Microsoft
    //     //   ---------
    //     //   "class std::basic_string<wchar_t,struct std::char_traits<wchar_t>,class std::allocator<wchar_t> >"
    //     //
    //     //   Clang and Intel
    //     //   ---------------
    //     //   "std::basic_string<wchar_t>"
    //     ////////////////////////////////////////////////////////////////
    //     constexpr tstring_view typeName = TypeName_v<std::wstring>;
    //
    // Note that this template simply extracts the type name from the
    // predefined strings __PRETTY_FUNCTION__ (for non-MSFT compilers), or
    // __FUNCSIG__ (when _MSC_VER is #defined), noting that these may or
    // may not be defined as a macro or even a string literal depending on
    // the compiler. __FUNCSIG__ is always officially a string literal in
    // VC++ however or specifically treated like one by Clang and Intel,
    // even though it's not a string literal in those compilers (a special
    // fix to treat it like one was implemented in those compilers
    // however). __PRETTY_FUNCTION__ however is not a string literal on
    // all compilers we currently support. See here for GCC for instance
    // https://gcc.gnu.org/onlinedocs/gcc/Function-Names.html).
    //
    // Whether it's a string literal or not doesn't matter for our
    // purposes however since we correctly extract the type name portion
    // from the string regardless (corresponding to template arg "T"),
    // returning this as a "tstring_view". Note that there are no lifetime
    // worries with the returned "tstring_view" since by default we store
    // the name of the type in a static string just big enough to hold it
    // (so it remains alive for the life of the app). This behavior can
    // be disabled however by #defining
    // TYPENAME_V_DONT_MINIMIZE_REQD_SPACE but there's no normally no
    // reason to do so unless a compiler error prevents the current code
    // from compiling for some reason (which should never happen on
    // currently supported compilers normally but we support this constant
    // for future use). If #defined then the returned "tstring_view" will
    // point directly into __PRETTY_FUNCTION__ instead (or __FUNCSIG__
    // when _MSC_VER is #defined), but these strings are also statically
    // defined so there are still no lifetime issues. The "tstring_view"
    // returned is just a view into these static strings which remain
    // alive for the life of the app. Note TYPENAME_V_DONT_MINIMIZE_REQD_SPACE
    //
    // isn't #defined by default since it's more space efficient not to.
    // When #defined it means that all of __PRETTY_FUNCTION__ or
    // __FUNCSIG__ will be included in the compiled binary even though we
    // only access the name portion of it ("TypeName_v" just returns a
    // "tstring_view" pointing to the type name withing these strings).
    // Since TYPENAME_V_DONT_MINIMIZE_REQD_SPACE isn't #defined by default
    // it means that __PRETTY_FUNCTION__ or __FUNCSIG__ will no longer be
    // stored in the compiled binary, only a copy of the type name itself
    // in its own static buffer (normally reducing the space of your
    // compiled binary though unless you call "TypeName_v" with a lot of
    // different types, the savings in space is usually negligible).
    //
    // Lastly, note that the techniques used to extract the type are based
    // on the undocumented format of __PRETTY__FUNCTION and __FUNCSIG__ so
    // there's always the potential they may fail one day (if a compiler
    // vendor ever changes the format or we encounter some previously
    // unknown situation with the existing format, again, since they're
    // not officially documented). However, in practice they're stable,
    // and the techniques we rely on are generally used by many others as
    // well, using similar approaches (though unlike many other
    // implementations, our own implementation isn't vulnerable to changes
    // in the name of the implementation function the following variable
    // relies on - it can be changed to anything without breaking things
    // but read on). Moreover, a "static_assert" just after the
    // implementation class itself ("Private::TypeNameImpl") checks if it
    // actually works for type float as a quick and dirty (but normally
    // reliable) test, i.e., the function should return "float" if passed
    // float as its template arg (arbitrarily chosen for testing but any
    // type whose name is the same on all supported compilers will do -
    // see comments preceding "Private::TypeNameImplBase::GetPrettyFunction"
    // above for further details). If not then the "static_assert" kicks
    // in with an appropriate message (and likely other "static_asserts"
    // in the implementation code itself). Note that I've also coded
    // things not to rely on anything in the fully-qualified name (or
    // return type) of __PRETTY_FUNCTION__ and __FUNCSIG___ themselves
    // (beyond what's necessary), as many implementations of this
    // (general) technique do. Doing so is brittle should a user change
    // anything in the function's signature, such as its name and/or
    // return type (as some users might do). This would break many
    // implementations even though most will (hopefully) be easy to fix.
    // Ours isn't sensitive to the function's signature by design however,
    // again, beyond the minimal requirements necessary for the code to
    // work (given that we're dealing with undocumented strings). Changing
    // the function's fully-qualified name for instance (the actual
    // function name, its member class name, or namespace), and/or its
    // return type (to "tstring_view" or "std::basic_string<tchar>" for
    // instance) won't break the function. Some users may not like the
    // "auto" return type for example and may want to change it to
    // "tstring_view" (declared in "CompilerVersions.h" though this is a
    // more MSFT-centric technique due to their historical use of TCHAR
    // which you can read up on if you're not familiar), or
    // "std::basic_string_view<char>" (or when replacing "char" with TCHAR
    // on MSFT platforms, hence resolving to "wchar_t" usually), or
    // "std::string_view" or "std::wstring_view" (the latter only on MSFT
    // platforms at this writing)
    //
    // The bottom line is that you can change our implementation
    // function's signature if you wish and it will (normally) still work
    // correctly. Note that if a given compiler vendor ever changes the
    // format of __PRETTY_FUNCTION__ itself however (or __FUNCSIG__ when
    // _MSC_VER is #defined), then that could potentially break the
    // function as noted but again, the "static_asserts" in the
    // implementation will normally trigger if this occurs, and you'll
    // then have to fix the implementation function for this template to
    // deal with it ("Private::TypeNameImpl::Get()" as it's currently
    // called at this writing). It seems very unlikely a compiler vendor
    // will ever change the format however, let alone in a way that will
    // break this function, unless it's done to deal with some new C++
    // feature perhaps but who knows.
    //
    // Long story short, our implementation function should normally be
    // very stable but compilation will (normally) fail with an
    // appropriate error if something goes wrong (again, via the
    // "static_asserts" in the private implementation function called by
    // this template).
    ///////////////////////////////////////////////////////////////////////////
    template <typename T>
    inline constexpr tstring_view TypeName_v = Private::TypeNameImpl<T>::Get();
#endif // #if CPP20_OR_LATER && defined(STDEXT_USE_MODULES) && !defined(STDEXT_BUILDING_MODULE_TYPETRAITS)

    // See this #defined constant for details
    #if defined(USE_CONCEPTS)
        #if !defined(DECLARE_MACROS_ONLY)
            template <typename T>
            concept IsClass_c = std::is_class_v<T>;
        #endif

        #define IS_CLASS_C StdExt::IsClass_c
    #else
        #define STATIC_ASSERT_IS_CLASS(T) static_assert(std::is_class_v<T>, \
                                                        "\"" #T "\" must be a class or struct");
        #define IS_CLASS_C typename
    #endif

#if !defined(DECLARE_MACROS_ONLY)
    ////////////////////////////////////////////////////////////////////////////
    // IsConstOrVolatile_v. WYSIWYG. Note that the negation means "T" is *not*
    // "const" or "volatile" (neither qualifier present - again, WYSIWYG)
    ////////////////////////////////////////////////////////////////////////////
    template<typename T>
    inline constexpr bool IsConstOrVolatile_v = std::is_const_v<T> ||
                                                std::is_volatile_v<T>;

    /////////////////////////////////////////////////////////////////////////////
    // IsSpecialization. Primary template. See partial specialization just below
    // for details.
    /////////////////////////////////////////////////////////////////////////////
    template <typename,
              template<typename...> class>
    struct IsSpecialization : public std::false_type
    {
    };

    /////////////////////////////////////////////////////////////////////////////
    // Partial specialization of (primary) template just above. The following
    // kicks in when the 1st template arg in the primary template above (a type)
    // is a specialization of the 2nd template arg (a template). IOW, this partial
    // specialization kicks in if the 1st template arg (a type) is a type created
    // from a template given by the 2nd template arg (a template). If not then the
    // primary template kicks in above instead (i.e., when the 1st template arg (a
    // type) isn't a type created from the template given by the 2nd template arg
    // (a template), meaning it's not a specialization of that template).
    //
    //    Example
    //    -------
    //    template <class T>
    //    class Whatever
    //    {
    //       // Make sure type "T" is a "std::vector"
    //       static_assert(IsSpecialization<T, std::vector>::value,
    //                     "Invalid template arg T. Must be a \"std::vector\"");
    //    };
    //
    //    Whatever<std::vector<int>> whatever1; // "static_assert" above succeeds (2nd template arg is a "std::vector")
    //    Whatever<std::list<int>> whatever2; // "static_assert" above fails (2nd template arg is *not* a "std::vector")
    /////////////////////////////////////////////////////////////////////////////
    template <template<typename...> class Template,
              typename... TemplateArgs>
    struct IsSpecialization<Template<TemplateArgs...>, Template> : public std::true_type
    {
    };

    //////////////////////////////////////////////////////////
    // Helper variable template for "IsSpecialization" above
    // (with the usual "_v" suffix). Set to true if "Type"
    // is a specialization of the given "Template" or false
    // otherwise. See "IsSpecialization" above for details.
    //////////////////////////////////////////////////////////
    template <typename Type,
              template<typename...> class Template>
    inline constexpr bool IsSpecialization_v = IsSpecialization<Type, Template>::value;

    ///////////////////////////////////////////////////////
    // "std::remove_cvref_t" not available until C++20 so
    // we roll our own for C++17 or earlier (and name it
    // using our normal Pascal casing to prevent any
    // confusion and/or collision with the C++20 version,
    // even though we defer to it in C++20 and later)
    ///////////////////////////////////////////////////////
    #if CPP20_OR_LATER
        template <typename T>
        using RemoveCvRef_t = std::remove_cvref_t<T>;
    #else
        template <typename T>
        using RemoveCvRef_t = std::remove_cv_t<std::remove_reference_t<T>>;
    #endif

    template <typename T>
    using RemovePtrRef_t = std::remove_pointer_t<std::remove_reference_t<T>>;

    ///////////////////////////////////////////////////////////////////////////////
    // ReplaceNthType. Creates a type alias called "ReplaceNthType::Type" which is
    // a "std::tuple" of all types in the "Ts" template arg (parameter pack) but
    // where the "Nth" type in "Ts" has been replaced with "NewT". The purpose is
    // therefore to simply replace the "Nth" type in a parameter pack with a new
    // type and create an alias for the resulting parameter pack as a "std::tuple"
    // (specialized on the resulting types). Note that "N" must be less than the
    // number of types in "Ts" or a "static_assert" occurs (i.e., you must target
    // an existing type to replace in "Ts" so "N" must be within bounds). If "Ts"
    // itself is empty than the "static_assert" is guaranteed to trigger even if
    // "N" is zero (since zero targets the 1st type but since "Ts" itself is empty
    // it has no types that can be replaced whatsoever).
    //
    //     Example
    //     -------
    //     ////////////////////////////////////////////////////////////////////
    //     // Use the helper alias "ReplaceNthType_t" to replace the "char" in
    //     // the following parameter pack with an "int".
    //     //
    //     //    float
    //     //    double
    //     //    char
    //     //    std::string
    //     //
    //     // We pass the (zero-based) index of the type to be replaced via the
    //     // "N" template arg (so passing 2 here to target the "char"), the type
    //     // to replace it with via the "NewT" template arg (an "int"), and the
    //     // parameter pack itself via "Ts" (the remaining template args whose
    //     // "Nth" type will be replaced with "NewT"). The resulting alias is
    //     // therefore "std::tuple<float, double, int, std::string>"
    //     ////////////////////////////////////////////////////////////////////////
    //     using ReplaceCharWithIntTuple = ReplaceNthType_t<2, int, float, double, char, std::string>;
    //
    //     // Make sure 3rd element in above tuple is an "int" (previously a "char")
    //     static_assert(std::is_same_v<std::tuple_element_t<2, ReplaceCharWithIntTuple>, int>);
    /////////////////////////////////////////////////////////////////////////////
    template <std::size_t N,
              typename NewT,
              typename... Ts>
    struct ReplaceNthType
    {
    private:
        // See comments above
        static_assert(N < sizeof...(Ts), "Template arg \"N\" must be less than the number of types in template "
                                         "arg (parameter pack) \"Ts\" (i.e., \"N\" must target an existing type "
                                         "in \"Ts\" to be replaced - new types can't be added using this alias)");

        template <std::size_t... Ints>
        static auto ReplaceNth(std::index_sequence<Ints...>) -> std::tuple<std::conditional_t<Ints == N, NewT, Ts>...>;

    public:
        using Type = decltype(ReplaceNth(std::index_sequence_for<Ts...>()));
    };

    //////////////////////////////////////////////////////////////////////////////
    // ReplaceNthType_t. Helper alias for "ReplaceNthType::Type" just above. See
    // "ReplaceNthType" comments above.
    //////////////////////////////////////////////////////////////////////////////
    template <std::size_t N,
              typename NewT,
              typename... Ts>
    using ReplaceNthType_t = typename ReplaceNthType<N, NewT, Ts...>::Type;

    //////////////////////////////////////////////////////////////
    // "IsTuple" (primary template). Determines if "T" is a
    // "std::tuple" specialization. Note that the primary template
    // kicks in only if "T" is *not* a "std::tuple" in which case
    // the primary template inherits from "std::false_type".
    // Otherwise, if it is a "std::tuple" then the specialization
    // below kicks in instead, which inherits from "std::true_type".
    // The specialization does the actual work of checking if "T"
    // is a "std::tuple"
    //////////////////////////////////////////////////////////////
    template <typename T,
              typename = void>
    struct IsTuple : std::false_type
    {
    };

    ////////////////////////////////////////////////////////////////
    // Specialization of "IsTuple" (primary) template just above.
    // This specialization does all the work. See primary template
    // above for details.
    ////////////////////////////////////////////////////////////////
    template <typename T>
    struct IsTuple<T,
                   std::enable_if_t<IsSpecialization_v<T, std::tuple>>
                  > : std::true_type
    {
    };

    ///////////////////////////////////////////////////////
    // Helper variable template for "IsTuple" above (with
    // the usual "_v" suffix).
    ///////////////////////////////////////////////////////
    template <typename T>
    inline constexpr bool IsTuple_v = IsTuple<T>::value;
#endif // #if !defined(DECLARE_MACROS_ONLY)

    //////////////////////////////////////////////
    // Concept for above template (see following
    // #defined constant for details)
    //////////////////////////////////////////////
    #if defined(USE_CONCEPTS)
        #if !defined(DECLARE_MACROS_ONLY)
            template <typename T>
            concept Tuple_c = IsTuple_v<T>;
        #endif

        #define TUPLE_C StdExt::Tuple_c
    #else
        #define STATIC_ASSERT_IS_TUPLE(T) static_assert(StdExt::IsTuple_v<T>, \
                                                        "\"" #T "\" must be a \"std::tuple\"");
        #define TUPLE_C typename
    #endif

#if !defined(DECLARE_MACROS_ONLY)
    ///////////////////////////////////////////////////////////////////////////
    // Declarations mostly associated with struct "FunctionTraits" declared
    // later on (the latter struct itself plus its various base classes and
    // other support declarations including helper templates mostly associated
    // with "FunctionTraits"). "FunctionTraits" itself is used to retrieve
    // information about any function including (most notably) its return type
    // and the type of any of its arguments (write traits also exist in
    // addition to read traits). Less frequently used traits are also
    // available. Simply pass the function's type as the template's only
    // argument (see next paragraph), but see the helper alias templates
    // "ReturnType_t" and "ArgType_t" declared later on for an even easier
    // (less verbose) way to access the return type and arg types of any
    // function (a full complement of helper templates exist for every
    // possible trait). The helper templates take the same template arg and
    // simply wrap the appropriate aliases of "FunctionTraits" demo'd in the
    // example below. They're easier to use than "FunctionTraits" directly
    // however and should normally be relied on. The example below shows how
    // to do it using "FunctionTraits" directly but the helper aliases should
    // normally be used instead.
    //
    // Please note that template arg F can be any legal C++ syntax for a
    // function's type, so it can refer to free functions, class member
    // functions, pointers and references to functions (references legal in
    // C++ for free functions only, including static member functions),
    // functors (classes with "operator()" declared though overloads are
    // ambiguous so aren't supported - a compiler error will occur), functions
    // with different calling conventions (__cdecl, __stdcall, etc. but read
    // on), "noexcept", "const" and "volatile" for non-static member
    // functions, lvalue and rvalue references declared on non-static member
    // functions (though these are rarely used in practice), and variadic
    // functions (those whose last arg is "..."). A specialization therefore
    // exists for every permutation of the above, noting that if the primary
    // template itself kicks in then no specialization exists for template
    // argument "F". It means that "F" is not a function type so a compiler
    // error will occur (our own concept will trigger in C++20 or greater, or
    // our own "static_assert" for C++17 or earlier.
    // Lastly, please note that calling conventions like "stdcall", etc. may
    // be replaced by compilers with "cdecl" under certain circumstances, in
    // particular when compiling for 64 bit environments (opposed to 32 bit).
    // That is, if you declare a function with a particular calling convention
    // other than "cdecl" itself, your compiler might change it to "cdecl"
    // depending on your compile-time environment (though exceptions may exist
    // however, such as the "vectorcall" calling convention which remains
    // unchanged in some 64 bit environments). "FunctionTraits" will therefore
    // reflect this so its "CallingConvention" member will correctly indicate
    // "cdecl", not the calling convention your function is actually declared
    // with (since the compiler changed it). Note that a given compiler may or
    // may not produce warnings about this situation when it quietly replaces
    // your calling convention with "cdecl".
    //
    //     Example
    //     -------
    //     ////////////////////////////////////////////////////////////////
    //     // Free function (member functions and non-overloaded functors
    //     // also supported - see comments above)
    //     ////////////////////////////////////////////////////////////////
    //     float SomeFunction(const std::string &arg1, double arg2, int arg3);
    //
    //     /////////////////////////////////////////////////////////////
    //     // Apply "FunctionTraits" to above function (note: template
    //     // arg "SomeFunction" in the following can optionally be
    //     // "&SomeFunction", i.e., pointers to functions are also
    //     // supported as are references to functions though they're
    //     // not widely used in practice - note that references to
    //     // functions only apply to free functions however since
    //     // references to non-static member functions aren't supported
    //     // in C++ itself)
    //     ////////////////////////////////////////////////////////////
    //     using SomeFuncTraits = FunctionTraits<decltype(SomeFunction)>;
    //     // using SomeFuncTraits = FunctionTraits<decltype(&SomeFunction)>; Same as above but using a function pointer
    //     // using SomeFuncTraits = FunctionTraits<float (const std::string &, double, int)>; // Same as above but manually passing the
    //                                                                                         // signature (just to show we can)
    //
    //     using ReturnType_t = typename SomeFuncTraits::ReturnType; // Function's return type (float)
    //     using Arg3Type_t = typename SomeFuncTraits::template Args<2>::Type; // Type of "arg3" (int)
    //
    //     ////////////////////////////////////////////////////////////////
    //     // Type of "arg3" returned as a "tstring_view" (so returns
    //     // "int" (literally) as a "tstring_view", quotes not included)
    //     ////////////////////////////////////////////////////////////////
    //     constexpr tstring_view arg3TypeName = TypeName_v<Arg3Type_t>;
    //
    // The above example demonstrates how to retrieve the return type and
    // the type of any argument (3rd argument in this example). Overall
    // you can retrieve the following info:
    //
    //     1) Function's return type
    //     2) Function's argument types. Note that for formal parameters
    //        declared as pass by value (opposed to reference), the "const"
    //        keyword (if present) isn't included in the arguments's type
    //        (the compiler removes it which prevents us from returning it,
    //        but still respects it if you try to change the parameter in
    //        the function, though that has nothing to do with this template)
    //     3) Whether the function is variadic (i.e., its last arg is "...")
    //     4) Whether the function has been declared "noexcept"
    //     5) The function's calling convention (__cdecl, __stdcall, etc.)
    //     6) For non-static member functions only, whether the function is
    //        declared "const" (since latter doesn't apply to static member
    //        functions)
    //     7) For non-static member functions only, whether the function is
    //        declared "volatile" (since latter doesn't apply to static member
    //        functions)
    //     8) For non-static member functions only, whether the function is
    //        declared with a reference-qualifier ""& or "&&" (since latter
    //        doesn't apply to static member functions)
    //     9) For non-static member functions only, the class the function
    //        belongs to (not available for *static* member functions
    //        unfortunately since compilers don't provide this information in
    //        the partial specializations "FunctionTraits" rely on - the class
    //        for static member functions simply isn't available, unlike for
    //        non-static member functions)
    //
    // Note that the following function info *isn't* available however
    // because there are no C++ trait techniques that support it (that
    // I'm aware of at this writing). Looked into it but couldn't find
    // (ergonomically correct) any way to do it using either the language
    // itself or any compiler tricks (such as a predefined macro - none
    // provide the required info).
    //
    //     1) Whether a non-static member function is declared virtual
    //     2) Whether a non-static member function is declared with the "override"
    //        keyword
    //     3) The class (or struct) of a static member function
    //
    // IMPORTANT: The techniques below are almost entirely based on Raymond
    // Chen's work from MSFT, but I've modified them to deal with various
    // issues that he didn't (like handling member functions among other
    // things). Surprisingly his work only deals with free functions (again,
    // I added support for member functions), but there were also some other
    // issues. Note that Raymond wrote a series of 7 articles in his
    // (well-known) blog showing how he developed this code. The articles are
    // as follows (in the order he posted them):
    //
    //     Deconstructing function pointers in a C++ template
    //     https://devblogs.microsoft.com/oldnewthing/20200713-00/?p=103978
    //
    //     Deconstructing function pointers in a C++ template, the noexcept complication
    //     https://devblogs.microsoft.com/oldnewthing/20200714-00/?p=103981
    //
    //     Deconstructing function pointers in a C++ template, vexing variadics
    //     https://devblogs.microsoft.com/oldnewthing/20200715-00/?p=103984
    //
    //     Deconstructing function pointers in a C++ template, the calling convention conundrum
    //     https://devblogs.microsoft.com/oldnewthing/20200716-00/?p=103986
    //
    //     Deconstructing function pointers in a C++ template, trying to address the calling convention conundrum
    //     https://devblogs.microsoft.com/oldnewthing/20200717-00/?p=103989
    //
    //     Deconstructing function pointers in a C++ template, addressing the calling convention conundrum
    //     https://devblogs.microsoft.com/oldnewthing/20200720-00/?p=103993
    //
    //     Deconstructing function pointers in a C++ template, returning to enable_if
    //     https://devblogs.microsoft.com/oldnewthing/20200721-00/?p=103995
    //
    // Unfortunately he didn't consolidate all this into one long article to
    // make it easier to read (they're just individual blogs), nor post his
    // code in a downloadable file for immediate use. Anyone implementing his
    // code therefore has to copy and paste it but also has to read through
    // the articles to understand things (in order to make it production-ready).
    //
    // Note that he also doesn't #include the few standard C++ headers his
    // code relies on either but none of this matters to anyone using the code
    // below. All the work's been done so you don't need to understand the
    // details, just how to use the "FunctionTraits" template itself (see
    // example further above)
    ///////////////////////////////////////////////////////////////////////////

    // For internal use only ...
    namespace Private
    {
        /////////////////////////////////////////////////////////////////////
        // "IsFreeFunction" (primary template). Primary template inherits
        // from "std::false_type" if "T" is *not* a free function, which
        // also includes static member functions ("free" will refer to both
        // from here on). Otherwise, if it is a free function then the
        // specialization just below kicks in instead (inheriting from
        // "std::true_type"). In order to understand how "IsFreeFunction"
        // works (which isn't as simple as it sounds), you first need to
        // understand C++ function type syntax. Note that function types in
        // C++ are structured as follows (square brackets indicate optional
        // items - note that this syntax isn't 100% precise but close
        // enough for this discussion):
        //
        //   ReturnType [CallingConvention] ([Args]) [const] [volatile] [& | &&] [noexcept];
        //
        // All function types including non-static member functions are
        // typed this way. For non-static member functions however, when
        // dealing with situations involving their type, normally
        // developers target them using pointer-to-member syntax but the
        // function's underlying type is still structured as seen above (no
        // different than for free functions). This may surprise many not
        // used to seeing the actual type of a non-static member function
        // since it's rarely encountered this way in most real-world code.
        // If you have the following for instance:
        //
        //     class Whatever
        //     {
        //     public:
        //         int SomeFunc(float) const;
        //     };
        //
        // And you need to deal with "SomeFunc" based on its type, you
        // would normally do this or similar:
        //
        //     using SomeFuncType = decltype(&Whatever::SomeFunc);
        //
        // "SomeFuncType" therefore resolves to the following:
        //
        //     int (Whatever::*)(float) const;
        //
        // You can then invoke "SomeFunc" using this pointer, the usual
        // reason you need to work with its type. However, this is a
        // pointer type (to a member of "Whatever" that happens to be a
        // function), not the actual type of "SomeFunc" itself, which is
        // actually this:
        //
        //     int (float) const;
        //
        // The above syntax follows the general structure of all functions
        // described earlier. It's also what "std::is_function" takes as
        // its template arg and it will return true in this case, i.e., you
        // can do this:
        //
        //     using F = int (float) const;
        //     constexpr bool isFunction = std::is_function_v<F>; // true
        //
        // However, the above likely appears to many (maybe most) that it's
        // a free function but it can't be since a free function can't be
        // "const". However, if you remove the "const":
        //
        //     using F = int (float);
        //     constexpr bool isFunction = std::is_function_v<F>; // true
        //
        // Now it really appears to be a free function type even though it
        // was a non-static member function type before removing the
        // "const", so how can you tell the difference. In fact you can't
        // distinguish between this non-static member function type (which
        // it was just before removing the "const" so in theory still is),
        // and a free function that has the same type. They're both the
        // same, in this case a function taking a "float" and returning an
        // "int". Without knowing if it came from a class or not it's just a
        // function type and that's all. You can therefore have a free
        // function with this type and a non-static member function with the
        // same type. The (major) difference is that a non-static member
        // function must be wired to the class it belongs to before you can
        // invoke it, and the usual pointer-to-member syntax allows
        // developers to do so. Nevertheless, the function's actual type
        // (syntax) is still that of a normal (non-static) function, as
        // emphasized. In the wild it's rarely ever encountered this way
        // however as previously noted (for a non-static member function).
        // The context in which the actual (non-pointer) type of a
        // non-static member function needs to be dealt with rarely ever
        // comes up for most developers, though there are some rare
        // circumstances when it can (but I won't bother showing an example
        // here - few will ever come across it). In any case, for purposes
        // of "IsFreeFunction" we're creating here, we want to return "true"
        // when template arg "T" refers to a "free" function but not a
        // non-static member function. Since there's no way to distinguish
        // between the two however when targeting a function like so:
        //
        //     using F = int (float);
        //
        // Then for purposes of "IsFreeFunction" we simply always assume
        // that it is a free function (but read on). It normally will be
        // in the real world because this syntax is almost always used
        // exclusively to deal with free functions, opposed a non-static
        // member functions, where pointer-to-member syntax is normally
        // used instead. Nevertheless, there are ways of getting hold of a
        // non-static member function's type though it usually takes some
        // extra effort. If you have such a type however and pass it for
        // template arg "T", "IsFreeFunction" will appear to erroneously
        // return "true", at least for the case just above. It would
        // therefore (seemingly) misidentify it as a free function but it
        // should be understood that the intention of "IsFreeFunction"
        // isn't to identify whether "T" originated from a free function or
        // not. Its purpose is to determine if it *qualifies* as a "free"
        // function even if "T" originated from a non-static member
        // function. So long as it represents a valid free function type,
        // then "IsFreeFunction" will return true, even if "T" actually
        // came from a non-static member function (with the same type).
        // "IsFreeFunction" doesn't care where it originates from IOW, only
        // that it represents a valid free function. Whether it actually
        // came from one or not is immaterial since the origin of "T"
        // doesn't matter.
        //
        // One might therefore ask, why not just rely on "std::is_function"
        // instead. The difference is that "std::is_function" will return
        // true for *any* function type, even if the type you pass has a
        // cv-qualifier and/or reference-qualifier (again, see the general
        // syntax of functions at the outset of these comments). The
        // cv-qualifiers ("const" or "volatile") and reference-qualifiers
        // (& and &&) aren't legal qualifiers on a free function so if you
        // have any of these ...
        //
        //     using F1  = int (float) const [noexcept];
        //     using F2  = int (float) volatile [noexcept];
        //     using F3  = int (float) const volatile [noexcept];
        //     using F4  = int (float) & [noexcept];
        //     using F5  = int (float) const & [noexcept];
        //     using F6  = int (float) volatile & [noexcept];
        //     using F7  = int (float) const volatile & [noexcept];
        //     using F8  = int (float) && [noexcept];
        //     using F9  = int (float) const && [noexcept];
        //     using F10 = int (float) volatile && [noexcept];
        //     using F11 = int (float) const volatile && [noexcept];
        //
        // ... then none of them qualify as a free function.
        // "std::is_function" will still return true for these however since
        // they're still valid functions. "IsFreeFunction" won't however,
        // and that's the difference between the two. It will return "false"
        // instead since they don't qualify as free functions. It's illegal
        // in C++ to use them in any context where they might be identified
        // as free functions. That's the only difference between
        // "std::is_function" and "IsFreeFunction". For all other function
        // types, namely, those that don't include the above qualifiers (cv
        // and reference), "std::is_function" and "IsFreeFunction" will both
        // return the same true or false value. You should therefore call
        // "IsFreeFunction" when you need to filter out functions with the
        // above qualifiers. If "IsFreeFunction" returns true, it not only
        // means that "T" is a function type (passing a non-function type
        // like "int" for instance would obviously return false, as would
        // pointers/references to functions which are pointer/reference
        // types, not actual function types), but that it also qualifies as
        // a free function as well, meaning it has none of the above
        // qualifiers. If "IsFreeFunction" returns false however, it either
        // means "T" isn't a function whatsoever (again, if you pass a
        // non-function type like "int" or even a function pointer or
        // reference for instance), or "T" *is* a function but it has one of
        // the above qualifiers so therefore isn't a "free" function
        // (doesn't qualify as one). Note that function types with cv and
        // reference qualifiers are informally known as "abominable" function
        // types. They are described here at this writing (note that the
        // author, Alisdair Meredith, is the C++ Standard Committee Library
        // Working Group chair at this writing):
        //
        //     Abominable Function Types
        //     https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2015/p0172r0.html
        //
        // "IsFreeFunction" therefore simply filters out abominable function
        // types, inheriting from "std::false_type" if "T" is in fact an
        // abominable function (or a non-function type altogether, including
        // pointers and references to functions). For all other functions
        // (actual function types only, again, not pointer and references to
        // functions), it inherits from "std::true_type" which is the
        // definition of a "free" function in this context (a function type
        // that's not an abominable function).
        //
        // Lastly, see the specialization just below for details on how to
        // actually determine if the above qualifiers are present or not
        // (i.e., how we filter out abominable functions)
        /////////////////////////////////////////////////////////////////////
        template <typename T,
                  typename = void>
        struct IsFreeFunction : std::false_type
        {
        };

        //////////////////////////////////////////////////////////////////
        // Specialization of "IsFreeFunction" (primary) template just
        // above. This specialization does all the work. See primary
        // template above for details. Note that this specialization
        // kicks in if "T" is a valid function type but not an abominable
        // function (see primary template above for full details). IOW,
        // this specialization kicks in only if the following holds:
        //
        // 1) "T" is a function type (pointers or references to functions
        //    don't qualify - must be an actual function type described in
        //    the primary template above)
        // 2) The function "T" has no cv-qualifiers or reference-qualifiers
        //    meaning it qualifies as a free function (which includes
        //    static member functions). It's not an abominable function
        //    IOW. Again, see primary template above for details. As
        //    described there, the following specialization kicks in only
        //    if "T" is a function type AND does not have any cv-qualifiers
        //    or reference-qualifiers as per the following examples (every
        //    possible permutation of these qualifiers shown - these are
        //    the abominable function types by definition):
        //
        //    using T1  = int (float) const [noexcept];
        //    using T2  = int (float) volatile [noexcept];
        //    using T3  = int (float) const volatile [noexcept];
        //    using T4  = int (float) & [noexcept];
        //    using T5  = int (float) const & [noexcept];
        //    using T6  = int (float) volatile & [noexcept];
        //    using T7  = int (float) const volatile & [noexcept];
        //    using T8  = int (float) && [noexcept];
        //    using T9  = int (float) const && [noexcept];
        //    using T10 = int (float) volatile && [noexcept];
        //    using T11 = int (float) const volatile && [noexcept];
        //
        // If none of these qualifiers are present then this specialization
        // kicks in as described (inheriting from "std::true_type"). If
        // any of them are present however than the primary template kicks
        // in instead (inheriting from "std::false_type" type)
        //
        // Lastly, note that the code below works by simply attempting to
        // add a pointer to "T", resulting in "T *", and then immediately
        // removing the pointer and checking if the resulting type is a
        // function (by passing it to "std::is_function_v"). If "T" isn't a
        // function at all, such as an "int", or even a pointer or reference
        // to a function (or even a reference type like "int &" which will
        // fail when we attempt to convert it to "int & *" - pointers to
        // references are illegal in C++ so SFINAE will always fail), then
        // it's guaranteed that the primary template above will kick in
        // (i.e., "T" is not a function at all in this case). If "T" is a
        // function however, then the attempt to add a pointer to it, i.e.,
        // convert it to "T *", will only work if "T" does not have a
        // cv-qualifier or reference-qualifier (i.e. it's not an abominable
        // function). IOW, it's a function type but has none of the
        // qualifiers seen above. "T *" is legal in this case so we wind up
        // with "T *", then immediately remove the pointer using
        // "std::remove_pointer_t", resulting in "T" again, which we pass to
        // "std::is_function_v" and it's guaranteed to return true. "T" is
        // therefore a function type but has none of the above qualifiers so
        // it's a free function type. OTOH, if any of the above qualifiers
        // are present in "T", meaning that "T" must be a non-static member
        // function (since free functions can't have these qualifiers), then
        // the attempt to add a pointer to "T", resulting in "T *", will
        // always fail because it's illegal to do so. See section 2.2 here:
        //
        //     Abominable Function Types (by Alisdair Meredith)
        //     https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2015/p0172r0.html
        //
        // You can't create a pointer to a function that has any of these
        // qualifiers (we're not dealing with pointer-to-member syntax
        // here, but the actual raw function type), so this SFINAE context
        // fails and the primary template above therefore kicks in instead
        // (inheriting from "std::false_type" meaning "T" does not qualify
        // as a free function even though it's a valid function type,
        // because "T" contains one of the above qualifiers - it must be a
        // non-static member function type as well as an abominable
        // function).
        ///////////////////////////////////////////////////////////////////
        template <typename T>
        struct IsFreeFunction<T,
                              std::enable_if_t<std::is_function_v<std::remove_pointer_t<T *>>>
                             > : std::true_type
        {
        };

        // Usual "_v" helper variable for above template
        template <typename T>
        inline constexpr bool IsFreeFunction_v = IsFreeFunction<T>::value;

        //////////////////////////////////////////////
        // Concept for above template (see following
        // #defined constant for details)
        //////////////////////////////////////////////
        #if defined(USE_CONCEPTS)
            template <typename T>
            concept FreeFunction_c = IsFreeFunction_v<T>;

            // For internal use only (we #undef it later)
            #define FREE_FUNCTION_C StdExt::Private::FreeFunction_c
        #else
            // For internal use only (we #undef it later)
            #define FREE_FUNCTION_C typename
        #endif

        //////////////////////////////////////////////////
        // Helper variable returning "true" if "T" is a
        // non-static member function pointer with no
        // cv-qualifiers on the pointer itself (opposed
        // to the non-static member function it points
        // to), or false otherwise
        //////////////////////////////////////////////////
        template<typename T>
        inline constexpr bool IsMemberFunctionNonCvPointer_v = std::is_member_function_pointer_v<T> && // Note that "std::is_member_function_pointer_v"
                                                                                                       // returns true for cv-qualified pointers as well
                                                               !IsConstOrVolatile_v<T>;

        //////////////////////////////////////////////
        // Concept for above template (see following
        // #defined constant for details)
        //////////////////////////////////////////////
        #if defined(USE_CONCEPTS)
            template <typename T>
            concept MemberFunctionNonCvPointer_c = IsMemberFunctionNonCvPointer_v<T>;

            // For internal use only (we #undef it later)
            #define MEMBER_FUNCTION_NON_CV_POINTER_C StdExt::Private::MemberFunctionNonCvPointer_c
        #else
            // For internal use only (we #undef it later)
            #define MEMBER_FUNCTION_NON_CV_POINTER_C typename
        #endif

        ///////////////////////////////////////////////////////////////////
        // "IsFunctor" (primary template). Primary template inherits from
        // "std::false_type" if "T" is *not* a functor. Otherwise, if it
        // is a functor then the specialization just below kicks in
        // instead (inheriting from "std::true_type"). If true then by
        // definition "T" must be a class or struct with a member function
        // called "operator()". Note that if "T::operator()" is overloaded
        // then the primary template will kick in since which "operator()"
        // to use becomes ambiguous. In this case you'll have to target
        // the specific "operator()" overload you're interested by taking
        // its address and casting it to the exact signature you're
        // interested in (in order to disambiguate it for whatever purpose
        // is required). You can't rely on this template IOW (since it
        // will fail to identify "T" as a functor due to the ambiguity).
        ///////////////////////////////////////////////////////////////////
        template <typename T,
                  typename = void>
        struct IsFunctor : std::false_type
        {
        };

        /////////////////////////////////////////////////////////////////
        // Specialization of "IsFunctor" (primary) template just above.
        // This specialization does all the work. See primary template
        // above for details.
        /////////////////////////////////////////////////////////////////
        template <typename T>
        struct IsFunctor<T,
                         std::void_t<decltype(&T::operator())>
                        > : std::true_type
        {
        };

        // Usual "_v" helper variable for above template
        template <typename T>
        inline constexpr bool IsFunctor_v = IsFunctor<T>::value;

        /////////////////////////////////////////////////////////////////
        // IsTraitsFreeFunction_v. Variable template set to true if "T"
        // is any of the following or false otherwise:
        //
        //   1) A free function including static member functions (but
        //      abominable function types always return false by design -
        //      see https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2015/p0172r0.html
        //   2) An (optionally cv-qualified) pointer to a free function
        //   3) A reference to a free function
        //   4) A reference to an (optionally cv-qualified) pointer to a
        //      free function
        /////////////////////////////////////////////////////////////////
        template <typename T>
        inline constexpr bool IsTraitsFreeFunction_v = IsFreeFunction_v<RemovePtrRef_t<T>>;

        //////////////////////////////////////////////
        // Concept for above template (see following
        // #defined constant for details)
        //////////////////////////////////////////////
        #if defined(USE_CONCEPTS)
            template <typename T>
            concept TraitsFreeFunction_c = IsTraitsFreeFunction_v<T>;

            // For internal use only (we #undef it later)
            #define TRAITS_FREE_FUNCTION_C StdExt::Private::TraitsFreeFunction_c
        #else
            // For internal use only (we #undef it later)
            #define TRAITS_FREE_FUNCTION_C typename
        #endif

        ///////////////////////////////////////////////////////////////////
        // IsTraitsMemberFunction_v. Variable template set to true if "T"
        // is either of the following or false otherwise:
        //
        //   1) An (optionally cv-qualified) pointer to a non-static member
        //      function
        //   2) An reference to an (optionally cv-qualified) pointer to a
        //      non-static member function
        ///////////////////////////////////////////////////////////////////
        template <typename T>
        inline constexpr bool IsTraitsMemberFunction_v = std::is_member_function_pointer_v<std::remove_reference_t<T>>; // Note that "std::is_member_function_pointer_v"
                                                                                                                        // returns true for cv-qualified pointers as well

        //////////////////////////////////////////////
        // Concept for above template (see following
        // #defined constant for details)
        //////////////////////////////////////////////
        #if defined(USE_CONCEPTS)
            template <typename T>
            concept IsTraitsMemberFunction_c = IsTraitsMemberFunction_v<T>;

            // For internal use only (we #undef it later)
            #define TRAITS_MEMBER_FUNCTION_C StdExt::Private::IsTraitsMemberFunction_c
        #else
            // For internal use only (we #undef it later)
            #define TRAITS_MEMBER_FUNCTION_C typename
        #endif

        ///////////////////////////////////////////////////////////////////
        // IsTraitsFreeOrMemberFunction_v. Variable template set to "true"
        // if "T" is any of the following or false otherwise:
        //
        //   1) A free function including static member functions (but
        //      abominable function types always return false by design -
        //      see https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2015/p0172r0.html
        //   2) An (optionally cv-qualified) pointer to a free function
        //   3) A reference to a free function
        //   4) A reference to an (optionally cv-qualified) pointer to a free
        //      function
        //   5) An (optionally cv-qualified) pointer to a non-static member
        //      function
        //   6) A reference to an (optionally cv-qualified) pointer to a
        //      non-static member function
        ///////////////////////////////////////////////////////////////////
        template <typename T>
        inline constexpr bool IsTraitsFreeOrMemberFunction_v = IsTraitsFreeFunction_v<T> ||
                                                               IsTraitsMemberFunction_v<T>;

        //////////////////////////////////////////////
        // Concept for above template (see following
        // #defined constant for details)
        //////////////////////////////////////////////
        #if defined(USE_CONCEPTS)
            template <typename T>
            concept IsTraitsFreeOrMemberFunction_c = IsTraitsFreeOrMemberFunction_v<T>;

            // For internal use only (we #undef it later)
            #define TRAITS_FREE_OR_MEMBER_FUNCTION_C StdExt::Private::IsTraitsFreeOrMemberFunction_c
        #else
            // For internal use only (we #undef it later)
            #define TRAITS_FREE_OR_MEMBER_FUNCTION_C typename
        #endif

        ///////////////////////////////////////////////////////////////////
        // IsTraitsFunctor_v. Variable template set to "true" if "T" is
        // either of the following or false otherwise:
        //
        //   1) A functor
        //   2) A reference to a functor
        //
        // See "struct IsFunctor" primary template earlier for details on
        // what constitutes a functor (the usual and expected C++
        // definition of a "functor" meaning "T" must be a class or struct
        // with a member function called "operator()" but for our purposes
        // "T::operator()" can't be overloaded or the following is
        // guaranteed to return false - we don't support overloaded functors
        // by design but again, see "struct IsFunctor" primary template
        // for details).
        ///////////////////////////////////////////////////////////////////
        template <typename T>
        inline constexpr bool IsTraitsFunctor_v = IsFunctor_v<std::remove_reference_t<T>>;

        //////////////////////////////////////////////
        // Concept for above template (see following
        // #defined constant for details)
        //////////////////////////////////////////////
        #if defined(USE_CONCEPTS)
            template <typename T>
            concept TraitsFunctor_c = IsTraitsFunctor_v<T>;

            // For internal use only (we #undef it later)
            #define TRAITS_FUNCTOR_C StdExt::Private::TraitsFunctor_c
        #else
            // For internal use only (we #undef it later)
            #define TRAITS_FUNCTOR_C typename
        #endif
    } // namespace Private

    /////////////////////////////////////////////////////////////////////////////
    // "IsTraitsFunction_v". Variable_template set to true if "T" is a function
    // type suitable for passing as the "F" template arg to struct "FunctionTraits"
    // or any of its helper templates, or "false" otherwise. Note that for use
    // with "FunctionTraits" or any of its helper templates, a suitable function
    // refers to any of the following types (in each case referring to the type,
    // not an actual instance of the type - note that pointer types may be
    // cv-qualified):
    //
    // 1) Free functions (which includes static member functions) but not
    //    including abominable functions (more on this shortly). See comments
    //    preceding "Private::IsFreeFunction" for complete details.
    // 2) Pointers to free functions
    // 3) References to free functions
    // 4) References to pointers to free functions
    // 5) Pointers to non-static member functions
    // 6) References to pointers to non-static member functions
    // 7) Functors or references to functors. In either case refers to a class
    //    with a single non-static "operator()" member so long as it's not
    //    overloaded. Doing so would create an ambiguous situation so if you
    //    need to deal with this you'll normally need to target the specific
    //    "operator()" overload you're interested in by taking its address and
    //    casting it to the exact signature you need to target. Item 5 or 6
    //    above will then handle it. Note that non-generic lambdas are also
    //    supported since lambdas are just syntactic sugar for creating functors
    //    on-the-fly. Generic lambdas however are not supported using a
    //    functor's type because they are not functors in the conventional
    //    sense. They can still be accessed via 5 and 6 above however as
    //    described in "Example 3" of the comments preceding the
    //    "FunctionTraits" specialization for functors. Search this file for
    //    "Example 3 (generic lambda)" (quotes not included).
    //
    // Note that function types (1 above) containing cv-qualifiers and/or
    // reference-qualifiers (AKA "abominable" functions) are NOT supported, so
    // "IsTraitsFunction_v" will return false for them (see details in comments
    // preceding "Private::IsFreeFunction" for further details). These are very
    // rare in reality though. They apply to non-static member functions only
    // since the presence of these qualifiers in free functions isn't legal in
    // C++. However, non-static member functions are only supported using
    // pointers to these functions (or references to such pointers), as seen in
    // items 5 and 6 above, not the actual non-static member function type
    // itself. Again, for complete details see the comments preceding
    // "Private::IsFreeFunction". The upshot is that if "T" is a function type
    // (item 1 above), and it has cv-qualifiers and/or reference-qualifiers as
    // per the following examples, then "IsTraitsFunction_v" will return false
    // by design. Note that all permutations of these qualifiers are accounted
    // for in the following examples. So long as those qualifiers aren't present
    // then "T" constitutes a legal free function and is interpreted that way
    // (not as a non-static member function which "FunctionTraits" only supports
    // via normal non-static member function pointer syntax or references to
    // such pointers - the actual member function's non-pointer type isn't
    // supported even though the following types are legal in C++):
    //
    //     using T1  = int (float) const [noexcept];
    //     using T2  = int (float) volatile [noexcept];
    //     using T3  = int (float) const volatile [noexcept];
    //     using T4  = int (float) & [noexcept];
    //     using T5  = int (float) const & [noexcept];
    //     using T6  = int (float) volatile & [noexcept];
    //     using T7  = int (float) const volatile & [noexcept];
    //     using T8  = int (float) && [noexcept];
    //     using T9  = int (float) const && [noexcept];
    //     using T10 = int (float) volatile && [noexcept];
    //     using T11 = int (float) const volatile && [noexcept];
    /////////////////////////////////////////////////////////////////////////////
    template <typename T>
    inline constexpr bool IsTraitsFunction_v = Private::IsTraitsFreeFunction_v<T> || // 1) Is "T" a free function (includes static member functions).
                                                                                     //    See (long) comments just above "Private::IsFreeFunction"
                                                                                     //    for details. Note that this item (1) excludes pointers and
                                                                                     //    references to free functions which are handled by items 2-4
                                                                                     //    below. This item (1) only targets the function's actual C++
                                                                                     //    type which is never a pointer or a reference. The actual C++
                                                                                     //    type for functions isn't used that often in practice however
                                                                                     //    (typically), since free function names decay to a pointer to
                                                                                     //    the function in most expressions so item 2 below is usually
                                                                                     //    encountered more frequently (though item 1 does occur sometimes
                                                                                     //    so it's correctly handled). In any case, if "T" is *not* an
                                                                                     //    actual function type then ...
                                                                                     // 2) Is "T" an (optionally cv-qualified) pointer to a free function
                                                                                     //    (the most common case in practice since free function names
                                                                                     //    decay to a pointer to the function in most expressions) or ...
                                                                                     // 3) Is "T" a reference to a free function (rare in practice) or ...
                                                                                     // 4) Is "T" a reference to an (optionally cv-qualified) pointer to
                                                                                     //    a free function (rare in practice)
                                                                                     // X) None of the above hold so "T" isn't a free function (or
                                                                                     //    a pointer to one or a reference to one or a reference to a
                                                                                     //    pointer to one) so we go on to check if it's a pointer to a
                                                                                     //    non-static member function ...
                                               Private::IsTraitsMemberFunction_v<T> || // 1) Is T an (optionally cv-qualified) pointer to a non-static
                                                                                       //    member function (the most common case for non-static
                                                                                       //    member functions in practice) or ...
                                                                                       // 2) Is T a reference to an (optionally cv-qualified) pointer
                                                                                       //    to a non-static member function (rare in practice)
                                                                                       // X) Neither of the above hold noting that "T" *cannot*
                                                                                       //    be a reference to a member function (not supported
                                                                                       //    in C++ itself so we don't check for it) though 2 just
                                                                                       //    above does support references to pointers to non-static
                                                                                       //    member functions
                                               Private::IsTraitsFunctor_v<T>; // Is "T" a functor or a reference to one (in either case the
                                                                              // functor has a non-static member function named "operator()"
                                                                              // by definition and it's not overloaded since overloads would
                                                                              // be ambiguous).
#endif // #if !defined(DECLARE_MACROS_ONLY)

    //////////////////////////////////////////////
    // Concept for above template (see following
    // #defined constant for details)
    //////////////////////////////////////////////
    #if defined(USE_CONCEPTS)
        #if !defined(DECLARE_MACROS_ONLY)
            template <typename T>
            concept TraitsFunction_c = IsTraitsFunction_v<T>;
        #endif

        #define TRAITS_FUNCTION_C StdExt::TraitsFunction_c
    #else
        #define STATIC_ASSERT_IS_TRAITS_FUNCTION(T) static_assert(StdExt::IsTraitsFunction_v<T>, \
                                                                  "\"T\" isn't a function type suitable for passing to \"FunctionTraits\" or any of " \
                                                                  "its helper templates. See comments preceding \"StdExt::IsTraitsFunction_v\" for " \
                                                                  "details but for all intents and purposes any legal type identifying a function will " \
                                                                  "normally do (i.e., free functions which include static members functions, pointers " \
                                                                  "and references to free functions, references to pointers to free functions, pointers " \
                                                                  "to non-static member functions, references to pointers to non-static member functions, " \
                                                                  "and (non-overloaded) functors or references to functors). \"T\" doesn't qualify as any " \
                                                                  "of these.");
        #define TRAITS_FUNCTION_C typename
    #endif

    ////////////////////////////////////////////////////////
    // MSFT? (or any other compiler we support but running
    // in Microsoft VC++ compatibility mode)
    ////////////////////////////////////////////////////////
    #if defined(_MSC_VER)
        ///////////////////////////////////////////////////////////////////
        // See https://learn.microsoft.com/en-us/cpp/cpp/argument-passing-and-naming-conventions?view=msvc-170
        ///////////////////////////////////////////////////////////////////
        #define STDEXT_CC_CDECL __cdecl
        #define STDEXT_CC_STDCALL __stdcall
        #define STDEXT_CC_FASTCALL __fastcall
        #define STDEXT_CC_VECTORCALL __vectorcall
        #define STDEXT_CC_THISCALL __thiscall

        /////////////////////////////////////////////////////////////
        // "regcall" calling convention only available on Clang and
        // Intel compilers (among the ones we currently support)
        /////////////////////////////////////////////////////////////
        #if defined(CLANG_COMPILER) || defined(INTEL_COMPILER)
            ////////////////////////////////////////////////////////////////////
            // See https://clang.llvm.org/docs/AttributeReference.html#regcall
            // See https://www.intel.com/content/www/us/en/docs/cpp-compiler/developer-guide-reference/2021-10/c-c-calling-conventions.html
            ////////////////////////////////////////////////////////////////////
            #define STDEXT_CC_REGCALL __regcall
        #endif
    #elif defined(GCC_COMPILER) || \
          defined(CLANG_COMPILER) || \
          defined(INTEL_COMPILER)

        /////////////////////////////////////////////////////////////////////////////
        // For GCC: https://gcc.gnu.org/onlinedocs/gcc/x86-Function-Attributes.html
        // For Clang: https://clang.llvm.org/docs/AttributeReference.html#calling-conventions
        // For Intel: https://www.intel.com/content/www/us/en/docs/cpp-compiler/developer-guide-reference/2021-10/c-c-calling-conventions.html
        /////////////////////////////////////////////////////////////////////////////
        #define STDEXT_CC_CDECL __attribute__((cdecl))
        #define STDEXT_CC_STDCALL __attribute__((stdcall))
        #define STDEXT_CC_FASTCALL __attribute__((fastcall))
        #define STDEXT_CC_THISCALL __attribute__((thiscall))

        /////////////////////////////////////////////////////////////
        // "regcall" calling convention only available on Clang and
        // Intel compilers (among the ones we currently support).
        // See here regarding GCC's support for vectorcall (still
        // not supported at this writing):
        //
        //     Bug 89485 - Support vectorcall calling convention on windows
        //     https://gcc.gnu.org/bugzilla/show_bug.cgi?id=89485
        /////////////////////////////////////////////////////////////
        #if defined(CLANG_COMPILER) || defined(INTEL_COMPILER)
            // See https://clang.llvm.org/docs/AttributeReference.html#regcall
            // See https://www.intel.com/content/www/us/en/docs/cpp-compiler/developer-guide-reference/2021-10/c-c-calling-conventions.html
            #define STDEXT_CC_VECTORCALL __attribute__((vectorcall))
            #define STDEXT_CC_REGCALL __attribute__((regcall))
        #endif
    #else
        /////////////////////////////////////////////////////
        // Note: Shouldn't be possible to come through here
        // at this writing since the same #error message
        // would have already been displayed earlier (when
        // the compiler constants just above were #defined
        // in "CompilerVersions.h")
        /////////////////////////////////////////////////////
        #error "Unsupported compiler (GCC, Microsoft, Clang and Intel are the only ones supported at this writing)"
    #endif

    ///////////////////////////////////////////////////////////////////
    // Always the case at this writing (variadic functions are always
    // STDEXT_CC_CDECL on the platforms we currently support), though
    // in a future release we may want to handle this better should we
    // ever support a platform where other calling conventions that
    // support variadic functions are available. For now we only
    // support variadic functions whose calling convention is
    // STDEXT_CC_CDECL and in practice this calling convention is
    // almost always universally used for variadic functions (even if
    // some platforms support other calling conventions that can be
    // used). Note that when using our "FunctionTraits" class or any
    // of its helper templates, if a variadic function is passed to
    // any of these templates with a calling convention other than
    // STDEXT_CC_CDECL (though it should never happen at this writing
    // on the platforms we support), then our TRAITS_FUNCTION_C
    // concept will usually kick in for C++20 and later, or a
    // "static_assert" for C++17 ("FunctionTraits" isn't available in
    // earlier versions - it's preprocessed out). On MSFT platforms
    // however variadic functions with calling conventions other than
    // "__cdecl" are automatically changed to "__cdecl" so no compiler
    // error occurs. See the following by Raymond Chen from MSFT:
    //
    //    "If you try to declare a variadic function with an incompatible
    //    calling convention, the compiler secretly converts it to cdecl"
    //    https://devblogs.microsoft.com/oldnewthing/20131128-00/?p=2543
    //
    // If we ever do handle other calling conventions for variadic
    // functions then additional specializations for "FunctionTraits"
    // will have to be created to handle them (plus some other
    // adjustments to the "FunctionTraits" code).
    ///////////////////////////////////////////////////////////////////
    #define STDEXT_CC_VARIADIC STDEXT_CC_CDECL

#if !defined(DECLARE_MACROS_ONLY)
    #if defined(GCC_COMPILER)
        ///////////////////////////////////////////////////////////////////
        // When targeting 64 bit builds the above STDEXT_CC_? calling
        // conventions are usually ignored (automatically replaced by the
        // compiler with STDEXT_CC_CDECL), except for STDEXT_CC_CDECL
        // itself (always supported), and STDEXT_CC_VECTORCALL (also
        // normally supported in a 64 bit build for all compilers we
        // support though GCC doesn't support this particular calling
        // convention). It's also possible that some calling conventions
        // may not be supported depending on other compiler options though
        // I haven't looked into this. The bottom line is that this can
        // result in many annoying warnings about unsupported calling
        // convention attributes so we'll temporarily turn these warnings
        // off here and back on later (only if they were already on).
        ///////////////////////////////////////////////////////////////////
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wattributes" // E.g., "warning: 'stdcall' attribute ignored [-Wattributes]"
                                                      // See https://gcc.gnu.org/onlinedocs/gcc-12.2.0/gcc/Diagnostic-Pragmas.html#Diagnostic-Pragmas

    // For Clang see https://clang.llvm.org/docs/UsersManual.html#controlling-diagnostics-via-pragmas
    // Note that Intel also relies on Clang as described in the comments just below
    #elif defined(CLANG_COMPILER) || \
          defined(INTEL_COMPILER)

        /////////////////////////////////////////////////
        // According to the Intel DPC++/C++ docs here:
        //
        //   Error Handling
        //   https://www.intel.com/content/www/us/en/docs/dpcpp-cpp-compiler/developer-guide-reference/2023-0/error-handling.html
        //
        // It says:
        //
        //    For a summary of warning and error options,
        //    see the Clang documentation.
        //
        // Where "Clang documentation" is this link:
        //
        //   https://clang.llvm.org/docs/UsersManual.html#options-to-control-error-and-warning-messages
        //
        // Hence the following works for Intel as well
        // (for the DPC++/C++ compiler which is what we
        // support, not the "Classic" Intel compiler
        // which they've deprecated and replaced with
        // the DPC++/C++ compiler - we don't support
        // the "Classic" compiler)
        /////////////////////////////////////////////////

        //////////////////////////////////////////////////////
        // See GCC_COMPILER call to its own "diagnostic push"
        // above (comments there apply here as well )
        //////////////////////////////////////////////////////
        #pragma clang diagnostic push
        #pragma clang diagnostic ignored "-Wignored-attributes" // E.g., "warning: 'stdcall' calling convention is not
                                                                // supported for this target [-Wignored-attributes]"
                                                                // See https://clang.llvm.org/docs/DiagnosticsReference.html#wignored-attributes
        #pragma clang diagnostic ignored "-Wunknown-attributes" // E.g., "warning: unknown attribute 'stdcall' ignored
                                                                // [-Wunknown-attributes]". Not normally encountered
                                                                // however. All calling conventions above are legal in
                                                                // Clang but ignoring this warning just in case. See
                                                                // https://clang.llvm.org/docs/DiagnosticsReference.html#wunknown-attributes
    #endif

    enum class CallingConvention
    {
        /////////////////////////////////////////////////////////
        // *** IMPORTANT ***
        // -----------------
        // Don't change the order OR value of these, our design
        // depends on it. Dirty but we'll live with it (makes
        // life easier for our purposes). Note that when adding
        // new values, make sure "Last" is updated below.
        /////////////////////////////////////////////////////////
        Cdecl,
        Stdcall,
        Fastcall,
        Vectorcall,
        Thiscall,
        Regcall,
        Last = Regcall, // IMPORTANT: Make sure this is always set to the last value just above (for internal use only)
        Variadic = Cdecl,
    };

    // Number of calling conventions we support
    inline constexpr std::size_t CallingConventionCount_v = static_cast<std::size_t>(CallingConvention::Last) + 1;

    ///////////////////////////////////////////////////////////////////////////
    // CallingConventionToString(). WYSIWYG
    ///////////////////////////////////////////////////////////////////////////
    inline constexpr tstring_view CallingConventionToString(const CallingConvention callingConvention) noexcept
    {
        constexpr std::array callingConventions = { _T("cdecl"),
                                                    _T("stdcall"),
                                                    _T("fastcall"),
                                                    _T("vectorcall"),
                                                    _T("thiscall"),
                                                    _T("regcall")
                                                  };

        ////////////////////////////////////////////
        // Triggers if new calling convention is
        // added to "CallingConvention" enum (very
        // rare), but not yet added to above array
        // (so add it)
        ////////////////////////////////////////////
        static_assert(callingConventions.size() == CallingConventionCount_v);

        /////////////////////////////////////////////////////
        // Returned as a "tstring_view" (constructor taking
        // a null-terminated string implicitly called)
        /////////////////////////////////////////////////////
        return callingConventions[static_cast<std::size_t>(callingConvention)];
    }

    ////////////////////////////////////////////////////////////////////////////
    // CallingConventionReplacedWithCdecl(). For the given "CallingConventionT"
    // template arg, returns true if the compiler will change a function with
    // that calling convention to the "cdecl" calling convention or false
    // otherwise (for a free function if "IsFreeFuncT" is "true" or a
    // non-static member function if "false"). Note that compilers will
    // sometimes change a function's (implicitly or explicitly) declared
    // calling convention to cdecl depending on your compiler options, in
    // particular when compiling for 64 bit versus 32 bit. The following tests
    // for this by simply comparing an (arbitrary) function with the calling
    // convention you pass ("CallingConventionT"), to the same (arbitrary)
    // function but with the cdecl calling convention. If they're equal then
    // the compiler isn't respecting the given "CallingConventionT" since it
    // changed it to cdecl, so the function returns true (meaning functions
    // declared with the calling convention you pass are replaced with the
    // cdecl calling convention by the compiler). Note however that false is
    // always returned if you pass "CallingConvention::Cdecl" itself for the
    // "CallingConventionT" arg, since for the intended purpose of this
    // function, the compiler never replaces cdecl with itself (it makes no
    // sense since it's already cdecl and AFAIK cdecl is always supported no
    // matter what compiler options are in effect). Note that the "vectorcall"
    // calling convention is also supported in 64 bit environments, at least
    // on the platforms we currently support and probably all others as well)
    /////////////////////////////////////////////////////////////////////////////
    template <CallingConvention CallingConventionT,
              bool IsFreeFuncT,
              typename StaticAssertT = void // Only required by calls to "static_assert" below
                                            // (to make them dependent on a template arg or
                                            // they'd always trigger otherwise )
             >
    inline constexpr bool CallingConventionReplacedWithCdecl() noexcept
    {
        // Free or static member function ...
        if constexpr (IsFreeFuncT)
        {
            if constexpr (CallingConventionT == CallingConvention::Cdecl)
            {
                /////////////////////////////////////////////////////////////
                // STDEXT_CC_CDECL never replaced with itself (for purposes
                // of this function), and it's always supported regardless
                // (so it never gets replaced, let alone with itself)
                /////////////////////////////////////////////////////////////
                return false;
            }
            // For internal use only (#undefined below)
            #define IS_REPLACED_WITH_CDECL(CC) std::is_same_v<void CC (), void STDEXT_CC_CDECL ()>
            else if constexpr (CallingConventionT == CallingConvention::Stdcall)
            {
                return IS_REPLACED_WITH_CDECL(STDEXT_CC_STDCALL);
            }
            else if constexpr (CallingConventionT == CallingConvention::Fastcall)
            {
                return IS_REPLACED_WITH_CDECL(STDEXT_CC_FASTCALL);
            }
            else if constexpr (CallingConventionT == CallingConvention::Vectorcall)
            {
                #if defined(STDEXT_CC_VECTORCALL)
                    return IS_REPLACED_WITH_CDECL(STDEXT_CC_VECTORCALL);
                #else
                    // Always triggers by design!
                    static_assert(AlwaysFalse_v<StaticAssertT>,
                                  "\"CallingConvention::Vectorcall\" not supported for this compiler " \
                                  "(the compiler itself doesn't support this calling convention)");
                    return true;
                #endif
            }
            else if constexpr (CallingConventionT == CallingConvention::Thiscall)
            {
                // Always triggers by design!
                static_assert(AlwaysFalse_v<StaticAssertT>,
                              "\"CallingConvention::Thiscall\" not supported for free functions " \
                              "(applicable to non-static member functions only)");
                return true;
            }
            else if constexpr (CallingConventionT == CallingConvention::Regcall)
            {
                #if defined(STDEXT_CC_REGCALL)
                    return IS_REPLACED_WITH_CDECL(STDEXT_CC_REGCALL);
                #else
                    // Always triggers by design!
                    static_assert(AlwaysFalse_v<StaticAssertT>,
                                  "\"CallingConvention::Regcall\" not supported for this compiler " \
                                  "(the compiler itself doesn't support this calling convention)");
                    return true;
                #endif
            }
            #undef IS_REPLACED_WITH_CDECL // Done with this
            else
            {
                // Always triggers by design!
                static_assert(AlwaysFalse_v<StaticAssertT>, "Unknown \"CallingConventionT\"");
                return true;
            }
        }
        // Non-static member function ...
        else
        {
            if constexpr (CallingConventionT == CallingConvention::Cdecl)
            {
                /////////////////////////////////////////////////////////////
                // STDEXT_CC_CDECL never replaced with itself (for purposes
                // of this function), and it's always supported regardless
                // (so it never gets replaced, let alone with itself)
                /////////////////////////////////////////////////////////////
                return false;
            }
            else
            {
                /////////////////////////////////////////////////////////
                // For use by macro just below. Note that testing shows
                // the calling convention isn't impacted by the scope
                // of this class, at least in the compilers we support.
                // The same calling convention results whether the
                // class is declared at function scope or outside the
                // function (at namespace scope), though it's a fuzzy
                // area so always possible a given compiler or compiler
                // option could affect it (though it seems unlikely at
                // this point - compilers sometimes change the calling
                // convention to Cdecl for instance, even when declared
                // with another calling convention, though it's normally
                // based on whether the declared calling convention is
                // supported depending on the compiler and compiler
                // options in effect, *not* the scope of the function
                // itself - we're likely safe IOW).
                /////////////////////////////////////////////////////////
                class AnyClass
                {
                };

                // For internal use only (#undefined below)
                #define IS_REPLACED_WITH_CDECL(CC) std::is_same_v<void (CC AnyClass::*)(), void (STDEXT_CC_CDECL AnyClass::*)()>

                if constexpr (CallingConventionT == CallingConvention::Stdcall)
                {
                    return IS_REPLACED_WITH_CDECL(STDEXT_CC_STDCALL);
                }
                else if constexpr (CallingConventionT == CallingConvention::Fastcall)
                {
                    return IS_REPLACED_WITH_CDECL(STDEXT_CC_FASTCALL);
                }
                else if constexpr (CallingConventionT == CallingConvention::Vectorcall)
                {
                    #if defined(STDEXT_CC_VECTORCALL)
                        return IS_REPLACED_WITH_CDECL(STDEXT_CC_VECTORCALL);
                    #else
                        // Always triggers by design!
                        static_assert(AlwaysFalse_v<StaticAssertT>,
                                      "\"CallingConvention::Vectorcall\" not supported for this compiler " \
                                      "(the compiler itself doesn't support this calling convention)");
                        return true;
                    #endif
                }
                else if constexpr (CallingConventionT == CallingConvention::Thiscall)
                {
                    return IS_REPLACED_WITH_CDECL(STDEXT_CC_THISCALL);
                }
                else if constexpr (CallingConventionT == CallingConvention::Regcall)
                {
                    #if defined(STDEXT_CC_REGCALL)
                        return IS_REPLACED_WITH_CDECL(STDEXT_CC_REGCALL);
                    #else
                        // Always triggers by design!
                        static_assert(AlwaysFalse_v<StaticAssertT>,
                                      "\"CallingConvention::Regcall\" not supported for this compiler " \
                                      "(the compiler itself doesn't support this calling convention)");
                        return true;
                    #endif
                }
                #undef IS_REPLACED_WITH_CDECL// Done with this
                else
                {
                    // Always triggers by design!
                    static_assert(AlwaysFalse_v<StaticAssertT>, "Unknown \"CallingConventionT\"");
                    return true;
                }
            }
        }
    }

    /////////////////////////////////////////////////////////////
    // Refers to the optional "&" or "&&" ref-qualifiers that
    // can be added just after the argument list of non-static
    // member functions (similar to adding optional
    // cv-qualifiers though the purpose is different of course).
    // These are rarely used in practice though so the "None"
    // enum is usually in effect for most member functions. Read
    // up on "ref-qualifiers" for further details (in the
    // context of non-static member functions)
    /////////////////////////////////////////////////////////////
    enum class RefQualifier
    {
        None,
        LValue,
        RValue
    };

    ///////////////////////////////////////////////////////////////////////////
    // RefQualifierToString(). WYSIWYG
    ///////////////////////////////////////////////////////////////////////////
    inline constexpr tstring_view RefQualifierToString(const RefQualifier refQualifier,
                                                       const bool useAmpersands = true) noexcept
    {
        tstring_view str;

        switch (refQualifier)
        {
            case RefQualifier::None:
                str = _T("None");
                break;
            case RefQualifier::LValue:
                str = useAmpersands ? _T("&") : _T("LValue");
                break;
            case RefQualifier::RValue:
                str = useAmpersands ? _T("&&") : _T("RValue");
                break;
        }

        return str;
    }

    ///////////////////////////////////////////////////////////////////////////
    // For internal use only (implementation details for "FunctionTraits" and
    // its helper templates, both declared after this namespace)
    ///////////////////////////////////////////////////////////////////////////
    namespace Private
    {
#endif // #if !defined(DECLARE_MACROS_ONLY)

        ////////////////////////////////////////////////
        // Always true (undefined) by default unless an
        // end-user explicitly #defines this for some
        // reason (we never do). If so then function
        // write traits won't be available in
        // "FunctionTraits" (we won't declare them).
        // Only read traits will be available. Note
        // that there's usually little reason for users
        // to #define the following constant however,
        // unless they never use function write traits
        // (usually the case for most - read traits is
        // far more common), and just want to eliminate
        // the overhead of function write traits (fair
        // enough though the overhead isn't that
        // significant). Note that most users will
        // likely never #define the following so write
        // traits will always be available by default
        // (unless they're compiling in VC++ from
        // Microsoft Visual Studio 2017 where write
        // traits aren't supported due to bugs in that
        // compiler - see test for #defined constant
        // MSVC_FROM_VISUAL_STUDIO_2017 just below)
        ////////////////////////////////////////////////
        #if !defined(REMOVE_FUNCTION_WRITE_TRAITS)
            /////////////////////////////////////////////////
            // Any compiler other than VC++ from Microsoft
            // Visual Studio 2017? (usually the case in most
            // environments so the following will usually
            // test true). Function write traits are
            // therefore normally supported. See comments
            // below.
            /////////////////////////////////////////////////
            #if !defined(MSVC_FROM_VISUAL_STUDIO_2017)
                //////////////////////////////////////////////////////////
                // Function write traits are supported for all compilers
                // other than VC++ from Visual Studio 2017 (as checked
                // just above). Two unrelated compiler bugs exist in that
                // version which cause compilation of our function write
                // traits to fail (details not worth getting into here
                // since use of that version is likely in significant
                // decline by now). The following will therefore be
                // #undefined for that compiler only meaning that
                // function write traits will be available in all
                // compilers we support except VC++ from Visual Studio
                // 2017. In that version they'll be preprocessed out
                // based on the following constant being #undefined so
                // they won't exist at all (and therefore won't be
                // available to end users). Only function read traits
                // will therefore be available in versions of VC++ from
                // Visual Studio 2017. Function write traits will only be
                // available in versions of VC++ from Visual Studio 2019
                // or later (as well as all non-Microsoft compilers).
                // Note that this constant is for internal use only (we
                // #undef it later).
                //////////////////////////////////////////////////////////
                #define FUNCTION_WRITE_TRAITS_SUPPORTED
            #endif
        #endif

#if !defined(DECLARE_MACROS_ONLY)
        ///////////////////////////////////////////////////////////////////////
        // FunctionTraitsHelper. Base class of "FunctionTraitsBase" which all
        // "FunctionTraits" derivatives (specializations) ultimately inherit
        // from. The following class is for internal use only so it contains
        // no public members. Contains helper aliases that "FunctionTraits"
        // ultimately relies on for its implementation (specifically for
        // handling function write traits - function read traits never rely
        // on this class).
        ///////////////////////////////////////////////////////////////////////
        class FunctionTraitsHelper
        {
            //////////////////////////////////////////////////
            // See this constant for details. Following code
            // is preprocessed out if function write traits
            // not supported (since code is only required to
            // support function write traits at this writing,
            // never function read traits). Function write
            // traits are normally supported however so the
            // following normally tests true (the only
            // exception at this writing is Microsoft Visual
            // C++ from Visual Studio 2017, which has a bug
            // that prevents us from including this code)
            //////////////////////////////////////////////////
            #if defined(FUNCTION_WRITE_TRAITS_SUPPORTED)
                template <typename F1, typename F2>
                using MigrateConst = std::conditional_t<std::is_const_v<std::remove_reference_t<F1>>,
                                                        std::add_const_t<F2>,
                                                        F2>;

                template <typename F1, typename F2>
                using MigrateVolatile = std::conditional_t<std::is_volatile_v<std::remove_reference_t<F1>>,
                                                           std::add_volatile_t<F2>,
                                                           F2>;

                template <typename F1, typename F2>
                using MigrateCv = MigrateVolatile<F1, MigrateConst<F1, F2>>;

                template <typename F1, typename F2>
                using MigrateRef = std::conditional_t<std::is_lvalue_reference_v<F1>,
                                                      std::add_lvalue_reference_t<F2>,
                                                      std::conditional_t<std::is_rvalue_reference_v<F1>,
                                                                         std::add_rvalue_reference_t<F2>,
                                                                         F2>>;

                //////////////////////////////////////////////////////////////////////////////
                // ReplaceArgsTupleImpl (primary template). Implements the "ReplaceArgsTuple"
                // alias seen in all "FunctionTraits" derivatives (specializations). That is,
                // "ReplaceArgsTuple" which is publicly declared in both
                // "StdExt::Private::FreeFunctionTraits" and
                // "StdExt::Private::FreeFunctionTraits" ("FunctionTraits" always derives
                // from either), ultimately defers to the struct you're now reading to carry
                // out its work. The following primary template never kicks in however, only
                // the partial specialization just below whenever template arg "T" is a
                // "std::tuple" and it always is by design (or the "static_assert" seen below
                // kicks in).
                //
                // Given the following (contrived) example for instance (end-user code):
                //
                //     using F = int (Whatever::* const volatile &&)(float, double)>;
                //     using NewArgsTupleT = std::tuple<char>;
                //     using FuncWithNewArgs = ReplaceArgsTuple_t<F, NewArgsTupleT>;
                //
                // The call to "ReplaceArgsTuple_t" just above (the helper template most
                // users will rely on), ultimately defers to the following struct with the
                // following template args (and because template arg "T" is always a
                // "std::tuple", the partial specialization just below will always kick in
                // to handle it):
                //
                //     ReplaceArgsT = StdExt::Private::MemberFunctionTraits<int (Whatever::* const volatile &&)(float, double),
                //                                                          int (Whatever::*)(float, double),
                //                                                          void
                //                                                         >::ReplaceArgs,
                //     T = std::tuple<char>
                //
                // The partial specialization below simply invokes "ReplaceArgsT" seen just
                // above, passing it the types that tuple "T" is specialized on.
                //////////////////////////////////////////////////////////////////////////////
                template <template<typename... NewArgsT> class ReplaceArgsT,
                          typename T>
                struct ReplaceArgsTupleImpl
                {
                    static_assert(AlwaysFalse_v<T>, "Invalid template arg \"T\". Must be a \"std::tuple\" containing the arg types you wish to apply");
                };

                ///////////////////////////////////////////////////////////////////
                // ReplaceArgsTupleImpl (partial specialization when 2nd arg is a
                // "std::tuple" - it always is by design - see primary template
                // just above for details)
                ///////////////////////////////////////////////////////////////////
                template <template<typename...> class ReplaceArgsT,
                          typename... NewArgsT>
                struct ReplaceArgsTupleImpl<ReplaceArgsT, std::tuple<NewArgsT...>>
                {
                    using Type = ReplaceArgsT<NewArgsT...>;
                };

            protected:
                // Called for free functions only in this release (including static member functions)
                template <typename F, typename NewF>
                using MigratePointerAndRef = MigrateRef<F,
                                                        std::conditional_t<std::is_pointer_v<std::remove_reference_t<F>>,
                                                                           MigrateCv<F, std::add_pointer_t<NewF>>,
                                                                           NewF>>;

                // Called for non-static member functions only in this release
                template <typename F, typename NewF>
                using MigrateCvAndRef = MigrateRef<F, MigrateCv<F, NewF>>;

                ///////////////////////////////////////////////////////////
                // Helper alias for "ReplaceArgsTupleImpl" template above.
                // Implements the public "ReplaceArgsTuple" alias seen in
                // all "FunctionTraits" derivatives (specializations).
                // Latter alias just defers to this one which defers to
                // "ReplaceArgsTupleImpl" as seen. See latter template
                // above for details.
                ///////////////////////////////////////////////////////////
                template <template<typename... NewArgsT> class ReplaceArgsT,
                          TUPLE_C NewArgsTupleT>
                using ReplaceArgsTupleImpl_t = typename ReplaceArgsTupleImpl<ReplaceArgsT, NewArgsTupleT>::Type;
            #endif // #if defined(FUNCTION_WRITE_TRAITS_SUPPORTED)
        };

        ////////////////////////////////////////////////////////////////////////////
        // struct FunctionTraitsBase. Base class that all "FunctionTraits"
        // specializations declared further below ultimately inherit from (the
        // latter template is what you'll actually use in your code, not
        // "FunctionTraitsBase" directly). Note that "FunctionTraits" has a primary
        // template that's empty (no members), and one partial specialization for
        // each permutation of function types that can be handled. However, since
        // there would normally be so many specializations given how many
        // permutations of function types we support, from free functions, to
        // pointers to free functions, references to free functions, references to
        // pointers to free functions, pointers to non-static member functions,
        // references to pointers to non-static member functions, etc., and coupled
        // with the fact that pointers to functions can also have cv-qualifiers
        // (i.e., you can have "const" pointers and "volatile" pointers and "const
        // volatile" pointers), and then when you factor in "noexcept", calling
        // conventions, and "cv-qualifiers" and "ref-qualifiers" on non-static
        // member functions, there would normally be 1878 total specializations to
        // handle on most platforms we currently support (and more than 2000 on
        // Clang and Intel which support an extra calling convention). While this
        // many specializations can easily be handled by a relatively small number
        // of macros which stitches together every permutation (and our original
        // version did in fact handle things this way - compile time was still very
        // quick though), in this release I've streamlined things to reduce the
        // number of specializations from 1878 on most platforms to 154 (again, a
        // bit more on Clang and Intel). We still handle all 1878 permutations that
        // normally exist, but through judicious use of "std::remove_reference_t",
        // "std::remove_pointer_t" and "std::remove_cv_t", we're now able to
        // capture all 1878 permutations using only 154 specializations (and it may
        // be possible to reduce this even further which I might attempt in a
        // future release - for now 154 is a big improvement over the original
        // release).
        //
        // The bottom line is that no matter how a function is declared, a partial
        // specialization of "FunctionTraits" exists below to handle it (even though
        // the actual number of specializations is far fewer now), and each
        // "FunctionTraits" specialization ultimately inherits from the following
        // struct (FunctionTraitsBase). "FunctionTraitsBase" contains all the
        // function's actual read traits (write traits in its "FreeFunctionTraits"
        // or "MemberFunctionTraits" derivatives), but users will normally access
        // these traits through either the "FunctionTraits" class itself (again,
        // which ultimately inherits from "FunctionTraitsBase"), or more commonly
        // the helper templates for "FunctionTraits" declared later on (which rely
        // on "FunctionTraits").
        ////////////////////////////////////////////////////////////////////////////
        template <TRAITS_FREE_OR_MEMBER_FUNCTION_C F, // Function's full type
                  #undef TRAITS_FREE_OR_MEMBER_FUNCTION_C // Done with this
                  typename ReturnTypeT, // Function's return type
                  CallingConvention CallingConventionT, // Function's calling convention (implicitly or explicitly declared in the function)
                  bool IsVariadicT, // "true" if function is variadic (last arg of function is "...") or "false" otherwise
                  typename ClassT, // Applicable to non-static member functions only (use the "IsMemberFunction" constant below to check this).
                                   // Stores the class (type) this non-static member function belongs to. Always "void" if not a member
                                   // function.
                  bool IsConstT, // Applicable to non-static member functions only (use the "IsMemberFunction" constant below to check this).
                                 // Set to "true" if a "const" member function or "false" otherwise. Always false if not a member function.
                  bool IsVolatileT, // Applicable to non-static member functions only (use the "IsMemberFunction" constant below to check this).
                                    // Set to "true" if a "volatile" member function or "false" otherwise. Always false if not a member function.
                  RefQualifier RefQualifierT, // Applicable to non-static member functions only (use the "IsMemberFunction" constant below to check
                                              // this). Set to the given "RefQualifier" enumerator if one is present in the function (& or &&).
                                              // Always "RefQualifier::None" otherwise (therefore always the case if not a member function)
                  bool IsNoexceptT, // "true" if the function is declared "noexcept" or "false" otherwise
                  typename... ArgsT> // Function's arguments (types) in left-to-right order of declaration (as would be expected)
        struct FunctionTraitsBase : FunctionTraitsHelper
        {
            // See this constant for details
            #if !defined(USE_CONCEPTS)
                //////////////////////////////////////////////////
                // Kicks in if concepts not supported, otherwise
                // TRAITS_FREE_OR_MEMBER_FUNCTION_C concept kicks
                // in in the template declaration above instead
                // (latter simply resolves to the "typename"
                // keyword when concepts aren't supported)
                //////////////////////////////////////////////////
                static_assert(IsTraitsFreeOrMemberFunction_v<F>,
                              "\"F\" must be a free function which includes static" \
                              "member functions, or a pointer or a reference to a " \
                              "free function, or a reference to a pointer to a free " \
                              "function, or a pointer to a non-static member function, " \
                              "or a reference to a pointer to a non-static member " \
                              "function. In all cases pointers may be cv-qualified " \
                              "(optional)");
            #endif

            ///////////////////////////////////////////////////////////////
            // Template args "ClassT", "IsConstT", "IsVolatileT" and
            // "RefQualifierT" only apply when "F" is a non-static member
            // function pointer (or a reference to one). Otherwise we're
            // dealing with a free function so they're not applicable
            // (make sure they're off) ...
            ///////////////////////////////////////////////////////////////
            static_assert(IsTraitsMemberFunction_v<F> || // Always true when "F" is a non-static member function
                                                         // pointer or a reference to one (pointer may be cv-qualified).
                                                         // Otherwise "F" must be a free function, or a pointer to one,
                                                         // or a reference to one, or a reference to a pointer to one.
                                                         // In all free function cases the 4 conditions just below must
                                                         // hold (since they don't apply to free functions) ...
                          (std::is_void_v<ClassT> &&
                           !IsConstT &&
                           !IsVolatileT &&
                           RefQualifierT == StdExt::RefQualifier::None));

            ///////////////////////////////////////////////////////////////////////
            // Function's full type. Same as template arg "F" used to instantiate
            // each "FunctionTraits" specialization (which all derive from
            // "FunctionTraitsBase"). If the template arg "F" passed to
            // "FunctionTraits" is a functor however (a class/struct with an
            // "operator()" member), then the "F" we receive here isn't that
            // functor (its class/struct), but the type of "operator()" in the
            // functor. The member "IsFunctor" below will return true in this case.
            // For a detailed explanation of acceptable values of "F" (that can be
            // passed to "FunctionTraits"), see "IsTraitsFunction_v".
            ///////////////////////////////////////////////////////////////////////
            using Type = F;

            ////////////////////////////////////////////////////////////////////////
            // Function's return type. The syntax for accessing this directly is a
            // bit verbose however since you have to apply the "typename" keyword.
            // You should therefore normally rely on the helper alias templates
            // "ReturnType" or "FunctionTraitsReturnType_t" instead (declared later
            // in this file). The former just defers to the latter and should
            // usually be used (it's easier). See these for details (plus examples).
            ////////////////////////////////////////////////////////////////////////
            using ReturnType = ReturnTypeT;

            /////////////////////////////////////////////////////////////////////////
            // Function's calling convention (implicitly or explicitly declared in
            // the function, usually the former but it doesn't matter). Note that if
            // a calling convention isn't supported, usually based on your compiler
            // options (target environment, etc.), then the compiler will replace it
            // with the "cdecl" calling convention normally (so the following will
            // reflect that). If you declare a function as "stdcall" for instance
            // (or any other besides "cdecl"), and it's not supported based on your
            // compiling options, then the compiler will replace it with "cdecl"
            // instead (and you may or may not receive a compiler warning that this
            // occurred depending on your platform and compiler options). Note that
            // "cdecl" itself is always (realistically) supported by all compilers
            // AFAIK (those we support at least but very likely all others as well)
            /////////////////////////////////////////////////////////////////////////
            static constexpr enum CallingConvention CallingConvention = CallingConventionT; // Note: Leave the "enum" in place since Clang flags it as an error otherwise
                                                                                            // (since we've named the variable "CallingConvention" which is the same as
                                                                                            // the enumerator's name)

            ///////////////////////////////////////////////////////////////////////
            // "true" if the function is variadic (last arg of function is "...")
            // or "false" otherwise. Note that "..." refers to the old-school
            // definition in C, so nothing to do with variadic template arguments
            // in C++.
            ///////////////////////////////////////////////////////////////////////
            static constexpr bool IsVariadic = IsVariadicT;

            ////////////////////////////////////////////////////////////////////
            // Applicable to non-static member functions only. Stores the class
            // (type) this non-static member function belongs to. Always "void"
            // otherwise. Use the "IsMemberFunction" member below to check
            // this. Note that at this writing, the following isn't supported
            // for static member functions (it's always void for them) since
            // C++ doesn't provide any simple or clean way to detect them in
            // the context that "FunctionTraits" is used.
            ////////////////////////////////////////////////////////////////////
            using Class = ClassT;

            /////////////////////////////////////////////////////////////////////
            // Is non-static member function declared with the "const" keyword.
            // Applicable to non-static member functions only (use the
            // "IsMemberFunction" member below to check this). Always false
            // otherwise (N/A in this case)
            /////////////////////////////////////////////////////////////////////
            static constexpr bool IsConst = IsConstT;

            ////////////////////////////////////////////////////////////////////////
            // Is non-static member function declared with the "volatile" keyword.
            // Applicable to non-static member functions only (use the
            // "IsMemberFunction" member below to check this). Always false
            // otherwise (N/A in this case)
            ////////////////////////////////////////////////////////////////////////
            static constexpr bool IsVolatile = IsVolatileT;

            //////////////////////////////////////////////////////////////////////
            // Is non-static member function declared with a reference-qualifier
            // (& or &&). Applicable to non-static member functions only (use the
            // "IsMemberFunction" member below to check this). Always
            // "RefQualifier::None" otherwise (N/A in this case).
            //////////////////////////////////////////////////////////////////////
            static constexpr enum RefQualifier RefQualifier = RefQualifierT; // Note: Leave the "enum" in place since Clang flags it as an error otherwise
                                                                             // (since we've named the variable "RefQualifier" which is the same as the
                                                                             // enumerator's name)

            // "true" if the function is declared "noexcept" or "false" otherwise
            static constexpr bool IsNoexcept = IsNoexceptT;

            ///////////////////////////////////////////////////////////////////////
            // Function's arguments (types) in left-to-right order of declaration
            // (as would be expected). The syntax for accessing this directly is
            // verbose however (using the "Args" struct below), so you should
            // normally rely on the helper alias templates "ArgType_t" (usually)
            // or less frequently "FunctionTraitsArgType_t" instead (both declared
            // later in this file). The former just defers to the latter. See
            // these for details (plus examples).
            ///////////////////////////////////////////////////////////////////////
            using ArgTypes = std::tuple<ArgsT...>;

            ///////////////////////////////////////////////////////////////////
            // Number of arguments in the function. This is officially called
            // "arity" but the term is obscure so we'll stick with a name
            // everyone can relate to.
            ///////////////////////////////////////////////////////////////////
            static constexpr std::size_t ArgCount = sizeof...(ArgsT);

            ////////////////////////////////////////////////////////////////////////
            // Function's arguments (types). Called as per the following example
            // (index "I" is zero-based) but see IMPORTANT section below for an
            // easier way (and also see helper function template "ForEachArg()"
            // if you need to iterate all arg types instead of just accessing
            // an individual arg type as the following does):
            //
            //  Example
            //  -------
            //       // Function (we'll detect the type of its 3rd arg just below)
            //       int MyFunction(int, double, float *, const std::string &)
            //       {
            //       }
            //
            //       ///////////////////////////////////////////////////////////////////
            //       // Type of the 3rd arg of above function ("float *"). Passing 2
            //       // here since template arg I is zero-based. Note that the syntax
            //       // is (obviously) verbose however so see IMPORTANT section just
            //       // below for an easier way (that wraps the following so you should
            //       // normally rely on it instead).
            //       ///////////////////////////////////////////////////////////////////
            //       using TypeOf3rdArg = typename FunctionTraits<decltype(MyFunction)>::template Args<2>::Type;
            //
            // IMPORTANT:
            // ---------
            //
            // Note that the syntax above is verbose so you should normally rely on
            // the helper alias templates "FunctionTraitsArgType_t" or "ArgType_t"
            // instead (usually the latter, and both declared later in this file).
            // The latter just defers to the former but in either case the syntax
            // is much easier than using the above technique directly (since the
            // aliases wrap it all up for you). See these templates for details. In
            // the above example for instance, you can replace it with this (using
            // the helper aliases):
            //
            //       //////////////////////////////////////////////////////////
            //       // Still verbose but better than the original call above
            //       // (but read on - it gets even easier)
            //       //////////////////////////////////////////////////////////
            //       using TypeOf3rdArg = FunctionTraitsArgType_t<FunctionTraits<decltype(MyFunction)>, 2>;
            //
            // or simplifying the above a bit (by breaking it into 2 lines, making
            // it longer but still arguably more digestible):
            //
            //       using MyFunctionTraits = FunctionTraits<decltype(MyFunction)>;
            //       using TypeOf3rdArg = FunctionTraitsArgType_t<MyFunctionTraits, 2>;
            //
            // or better yet, you can rely on the following helper instead (so the
            // syntax is now about as clean as we can get it):
            //
            //       using TypeOf3rdArg = ArgType_t<decltype(MyFunction), 2>;
            //
            // or lastly, you can break the line above into the following two lines
            // instead if you wish (though the above line isn't that verbose IMHO so
            // still arguably better):
            //
            //       using MyFunctionType = decltype(MyFunction);
            //       using TypeOf3rdArg = ArgType_t<MyFunctionType, 2>;
            ////////////////////////////////////////////////////////////////////////
            template <std::size_t I> // I is 0-based
            struct Args
            {
                static_assert(I < ArgCount, "Invalid index I. Must be less than the number of types in (variadic) template arg \"ArgsT\" "
                                            "(i.e., the number of arguments in the function this struct is being specialized on). Note "
                                            "that the number of function arguments can be retrieved by member \"ArgCount\" or its more "
                                            "user friendly helper template \"ArgCount_v\" (should you require this).");
                using Type = std::tuple_element_t<I, ArgTypes>;
            };

            //////////////////////////////////////////////////////////////
            // Returns true if the function represented by this traits
            // struct is a non-static member function (including
            // functors), or false otherwise (in the latter case it's
            // either a free function or a static member function). If
            // true then the "Class", "IsConst", "IsVolatile" and
            // "RefQualifier" members above can be inspected. They're
            // always "void", "false", "false" and "RefQualifier::None"
            // respectively otherwise. Please note that at this writing,
            // the following isn't supported for *static* member
            // functions (it's always false for them), since C++ doesn't
            // provide any simple or clean (ergonomically satisfying) way
            // to detect them that I could find (in the context of using
            // a traits class like "FunctionTraits"). Therefore, even
            // though a static member function is a member function, the
            // following always returns false for it (hopefully a future
            // C++ release will allow us to revisit the situation)
            //////////////////////////////////////////////////////////////
            static constexpr bool IsMemberFunction = !std::is_void_v<Class>;

            //////////////////////////////////////////////////////////////
            // Returns true if the function represented by this traits
            // struct is a free function (including static member
            // functions) or false otherwise (in the latter case it must
            // be a member function including functors).
            //////////////////////////////////////////////////////////////
            static constexpr bool IsFreeFunction = !IsMemberFunction;

            //////////////////////////////////////////////////////////////////////////
            // Was this class instantiated from a functor? Always "false" here as
            // seen but overridden in the "FunctorTraits" partial specialization for
            // functors later on, which ultimately inherits from the class you're now
            // reading. In that derived class we redefine the following and set it to
            // true, which then "overrides" (hides) the following when accessing it
            // from that particular specialization only (and no other specialization
            // - hiding base class members admittedly not the cleanest design but
            // we'll live with it for now). Note that when true (in that partial
            // specialization only), then it means "FunctionTraits" was instantiated
            // on the functor given by the "Class" alias member seen further above.
            // The "Type" alias further above therefore stores the type of
            // "Class::operator()". Note that when true, member "IsMemberFunction"
            // is also guaranteed to be true.
            //
            // IMPORTANT: Note that if someone instantiates "FunctionTraits" using
            // the actual type of "operator()" for its template arg, opposed to the
            // functor itself (the class containing "operator()"), then "IsFunctor"
            // will always be false, even though "FunctionTraits" targets the same
            // "operator()" (again, because someone passed its type directly, opposed
            // to the functor class it belongs to). This is because the partial
            // specialization taking a non-static member function pointer kicks in
            // opposed to the specialization taking the (functor) class type. It's by
            // design IOW but given a non-static member function's type only, there's
            // no way to determine if it represents "operator()" anyway (since
            // another non-static member function with the exact same type might
            // exist in a class, so we wouldn't know if the function we're working
            // with is "operator()" or some other member that happens to have the
            // exact same type).
            //////////////////////////////////////////////////////////////////////////
            static constexpr bool IsFunctor = false;

        protected:
            /////////////////////////////////////////////////////////
            // Default constructor. Made protected to prevent class
            // from being publicly constructed (since it ultimately
            // serves as a base class for "FunctionTraits" - no
            // instance is normally ever required since it's a
            // traits class).
            /////////////////////////////////////////////////////////
            FunctionTraitsBase() = default;
        }; // struct FunctionTraitsBase

        template <TRAITS_FREE_FUNCTION_C F,
                  FREE_FUNCTION_C FreeFunctionT = RemovePtrRef_t<F>, // For "std::enable_if_t" purposes in our specializations. Just "F" above after
                                                                     // removing any reference from "F" if present and then any pointer if present.
                                                                     // "F" itself may be a raw function or a pointer to a function or a reference
                                                                     // to a function or a reference to a pointer to a function. "FreeFunctionT" is
                                                                     // therefore always just the resulting raw function type (i.e., not a pointer
                                                                     // to a function or a reference to a function or a reference to a pointer to a
                                                                     // function but the actual raw function type). Never an "abominable" function
                                                                     // type however since "F" is never passed one by design (note that the
                                                                     // TRAITS_FUNCTION_C and FREE_FUNCTION_C concepts also reject them). We don't
                                                                     // support them since abominable functions (those with cv-qualifiers and/or
                                                                     // ref-qualifiers) are always processed as member function pointers by design
                                                                     // so they never come through here.
                  #undef FREE_FUNCTION_C // Done with this
                  typename = void> // For "std::enable_if_t" purposes in our specializations
        struct FreeFunctionTraits
        {
            // See this constant for details
            #if !defined(USE_CONCEPTS)
                //////////////////////////////////////////////////
                // Kicks in if concepts not supported, otherwise
                // TRAITS_FREE_FUNCTION_C and FREE_FUNCTION_C
                // concepts kick in in the template declaration
                // above instead (both simply resolve to the
                // "typename" keyword when concepts aren't
                // supported)
                //////////////////////////////////////////////////
                static_assert(IsTraitsFreeFunction_v<F>);
                static_assert(IsFreeFunction_v<FreeFunctionT>);
            #endif

            /////////////////////////////////////////////////////////////
            // By design (mandatory and always the case when relying on
            // the default arg for "FreeFunctionT" which we always do)
            /////////////////////////////////////////////////////////////
            static_assert(std::is_same_v<RemovePtrRef_t<F>, FreeFunctionT>);

            static_assert(AlwaysFalse_v<F>, "No expected partial specialization exists for \"FreeFunctionTraits\", "
                                            "usually because its \"FreeFunctionT\" template arg has an unsupported "
                                            "calling convention (though other rare reasons may be possible). Note "
                                            "that \"FreeFunctionT\" is just template arg \"F\" stripped down to the "
                                            "free function type it refers to (the result of removing any reference "
                                            "from \"F\" if present, and then any function pointer if present). \"F\" "
                                            "itself is therefore an unsupported type for use with this template, and "
                                            "more generally for \"FunctionTraits\" or any of its helper templates.");
        }; // struct FreeFunctionTraits

        /////////////////////////////////////////////////////////
        // For internal use only (we #undef it later). Macro
        // to check if a function is variadic. ARGS arg will
        // always be either of the following at this writing
        // (formatted exactly as seen and nothing else ever
        // passed):        
        //
        // (ArgsT...)
        // (ArgsT..., ...)
        //
        // Macro simply converts the above to a string (literal)
        // and checks offset 10 for a NULL-terminator. If present
        // then we're dealing with the first string above (a
        // non-variadic function), or the second one otherwise (a
        // variadic function). Check for hardcoded 10 is brittle
        // and ugly but should normally be stable so long as
        // nobody changes the above (so we'll live with it for
        // now).
        //////////////////////////////////////////////////////////
        #define IS_VARIADIC(ARGS) #ARGS[10] != '\0'

        ////////////////////////////////////////////////////////////////
        // For internal use only (we #undef it later). Base class for
        // "FreeFunctionTraits" specialization further below (macro
        // for internal use only). All "FunctionTraits" specializations
        // for free functions ultimately derive from this.
        ////////////////////////////////////////////////////////////////
        #define FUNCTION_TRAITS_BASE_CLASS(CALLING_CONVENTION, ARGS, IS_NOEXCEPT) \
            FunctionTraitsBase<F, \
                               R, \
                               CALLING_CONVENTION, \
                               IS_VARIADIC(ARGS), \
                               void, \
                               false, \
                               false, \
                               RefQualifier::None, \
                               IS_NOEXCEPT, \
                               ArgsT...>

        ///////////////////////////////////////////////////////
        // See this constant for details. Following code is
        // added to "FreeFunctionTraits" specialization
        // further below if function write traits supported
        // (normally the case), or preprocessed out otherwise.
        // If #defined then the latter struct will contain
        // our implementation of function write traits.
        ///////////////////////////////////////////////////////
        #if defined(FUNCTION_WRITE_TRAITS_SUPPORTED)
            /////////////////////////////////////////////////
            // Is "vectorcall" calling convention supported
            // by this compiler?
            /////////////////////////////////////////////////
            #if defined(STDEXT_CC_VECTORCALL)
                // For internal use only (we #undef it later). 
                #define STDEXT_CC_VECTORCALL_REPLACE(CC) STDEXT_CC_VECTORCALL
            #else
                // For internal use only (we #undef it later). 
                #define STDEXT_CC_VECTORCALL_REPLACE(CC) CC
            #endif

            //////////////////////////////////////////////
            // Is "regcall" calling convention supported
            // by this compiler?
            //////////////////////////////////////////////
            #if defined(STDEXT_CC_REGCALL)
                // For internal use only (we #undef it later). 
                #define STDEXT_CC_REGCALL_REPLACE(CC) STDEXT_CC_REGCALL
            #else
                // For internal use only (we #undef it later). 
                #define STDEXT_CC_REGCALL_REPLACE(CC) CC
            #endif

            ////////////////////////////////////////////////////////////////////////////
            // REPLACE_CALLING_CONVENTION (for internal use only - we #undef it later).
            // Used to implement "FreeFunctionTraits::ReplaceCallingConvention" for
            // non-variadic free functions. Note that the (zero-based) 4th entry in
            // the tuple we're creating below is for handling STDEXT_CC_THISCALL. It's
            // not supported for free functions so if someone calls
            // "FreeFunctionTraits::ReplaceCallingConvention" and passes
            // "CallingConvention::Thiscall" for its template arg, the (zero-based)
            // 4th entry below will kick in to handle it, thus ensuring the calling
            // convention remains unchanged ("CC" in the (zero-based) 4th entry below
            // will be the calling convention of the function the user is targeting so
            // it remains unchanged).
            ////////////////////////////////////////////////////////////////////////////
            #define REPLACE_CALLING_CONVENTION(CC, ARGS, IS_NOEXCEPT) \
                std::tuple_element_t<static_cast<std::size_t>(NewCallingConventionT), \
                                     std::tuple<R STDEXT_CC_CDECL ARGS noexcept(IS_NOEXCEPT), \
                                                R STDEXT_CC_STDCALL ARGS noexcept(IS_NOEXCEPT), \
                                                R STDEXT_CC_FASTCALL ARGS noexcept(IS_NOEXCEPT), \
                                                R STDEXT_CC_VECTORCALL_REPLACE(CC) ARGS noexcept(IS_NOEXCEPT), \
                                                R CC ARGS noexcept(IS_NOEXCEPT), \
                                                R STDEXT_CC_REGCALL_REPLACE(CC) ARGS noexcept(IS_NOEXCEPT)> \
                                    >
            ////////////////////////////////////////////////////////
            // For internal use only (we #undef it later). Adds
            // function write traits to "struct FreeFunctionTraits"
            // below.
            ////////////////////////////////////////////////////////
            #define DECLARE_FUNCTION_WRITE_TRAITS(CC, CALLING_CONVENTION, ARGS, IS_NOEXCEPT) \
                private: \
                    using BaseClass = FUNCTION_TRAITS_BASE_CLASS(CALLING_CONVENTION, ARGS, IS_NOEXCEPT); \
                    template <typename NewF> \
                    using MigratePointerAndRef_t = typename BaseClass::template MigratePointerAndRef<F, NewF>; \
                public: \
                    using AddVariadicArgs = MigratePointerAndRef_t<R STDEXT_CC_VARIADIC (ArgsT..., ...) noexcept(IS_NOEXCEPT)>; \
                    using RemoveVariadicArgs = MigratePointerAndRef_t<R CC (ArgsT...) noexcept(IS_NOEXCEPT)>; \
                    using AddConst = F; \
                    using RemoveConst = F; \
                    using AddVolatile = F; \
                    using RemoveVolatile = F; \
                    using AddCV = F; \
                    using RemoveCV = F; \
                    using AddLValueReference = F; \
                    using AddRValueReference = F; \
                    using RemoveReference = F; \
                    using AddNoexcept = MigratePointerAndRef_t<R CC ARGS noexcept>; \
                    using RemoveNoexcept = MigratePointerAndRef_t<R CC ARGS>; \
                    template <StdExt::CallingConvention NewCallingConventionT> \
                    using ReplaceCallingConvention = MigratePointerAndRef_t<REPLACE_CALLING_CONVENTION(CC, ARGS, IS_NOEXCEPT)>; \
                    template <IS_CLASS_C> \
                    using ReplaceClass = F; \
                    template <typename NewReturnTypeT> \
                    using ReplaceReturnType = MigratePointerAndRef_t<NewReturnTypeT CC ARGS noexcept(IS_NOEXCEPT)>; \
                    template <typename... NewArgsT> \
                    using ReplaceArgs = MigratePointerAndRef_t<std::conditional_t<BaseClass::IsVariadic, \
                                                                                  R STDEXT_CC_VARIADIC (NewArgsT..., ...) noexcept(IS_NOEXCEPT), \
                                                                                  R CC (NewArgsT...) noexcept(IS_NOEXCEPT)>>; \
                    template <TUPLE_C NewArgsTupleT> \
                    using ReplaceArgsTuple = typename BaseClass::template ReplaceArgsTupleImpl_t<ReplaceArgs, NewArgsTupleT>; \
                    template <std::size_t N, typename NewArgT> \
                    using ReplaceNthArg = ReplaceArgsTuple<ReplaceNthType_t<N, NewArgT, ArgsT...>>;
        #else
            ////////////////////////////////////////////////
            // For internal use only (we #undef it later).
            // Does nothing however. Function write traits
            // simply won't be added to "struct
            // FreeFunctionTraits" just below.
            ////////////////////////////////////////////////
            #define DECLARE_FUNCTION_WRITE_TRAITS(CC, CALLING_CONVENTION, ARGS, IS_NOEXCEPT)
        #endif // #if defined(FUNCTION_WRITE_TRAITS_SUPPORTED)

        //////////////////////////////////////////////////////////////////////////////
        // For internal use only (we #undef it later). Creates a "FreeFunctionTraits"
        // partial specialization to handle both free (non-member) functions and
        // static member functions (non-static member functions are handled later).
        // Called multiple times just below for each unique combination of macro
        // parameters (in succession by the numeric suffix at the end of each macro,
        // starting at "1" and ending with this one, "2"). A partial specialization
        // will therefore exist for all permutations of free functions and static
        // member functions (i.e., those with different calling conventions, those
        // with and without "noexcept", and variadic functions). Anytime someone uses
        // "FunctionTraits" to retrieve the traits for either a free (non-member)
        // function or static member function, the appropriate partial specialization
        // will therefore kick in (since "FunctionTraits" derives from
        // "FreeFunctionTraits" in this case). See Raymond Chen's 7 articles (links)
        // posted in the large header block further above (the following is based on
        // his work though I've modified his code for our needs including adding
        // support for non-static member functions further below)
        //////////////////////////////////////////////////////////////////////////////
        #define MAKE_FREE_FUNC_TRAITS_2(CC, CALLING_CONVENTION, ARGS, IS_NOEXCEPT) \
            template <TRAITS_FREE_FUNCTION_C F, \
                      typename R, \
                      typename... ArgsT> \
            struct FreeFunctionTraits<F, \
                                      R CC ARGS noexcept(IS_NOEXCEPT), \
                                      std::enable_if_t<!CallingConventionReplacedWithCdecl<CALLING_CONVENTION, true>() && \
                                                       AlwaysTrue_v<F>>> \
                : FUNCTION_TRAITS_BASE_CLASS(CALLING_CONVENTION, ARGS, IS_NOEXCEPT) \
            { \
                static_assert(std::is_same_v<RemovePtrRef_t<F>, R CC ARGS noexcept(IS_NOEXCEPT)>); \
                DECLARE_FUNCTION_WRITE_TRAITS(CC, CALLING_CONVENTION, ARGS, IS_NOEXCEPT) \
            };

        // noexcept (for internal use only, we #undef it later - invokes macro just above)
        #define MAKE_FREE_FUNC_TRAITS_1(CC, CALLING_CONVENTION, ARGS) \
            MAKE_FREE_FUNC_TRAITS_2(CC, CALLING_CONVENTION, ARGS, false) \
            MAKE_FREE_FUNC_TRAITS_2(CC, CALLING_CONVENTION, ARGS, true)

        ////////////////////////////////////////////////////////////////////
        // For internal use only (we #undef it later). Invokes macro just
        // above. Launching point (first macro to be called) to create
        // partial specializations of "FreeFunctionTraits" for handling
        // every permutation of non-variadic functions for the passed
        // calling convention (variadic functions are handled just after).
        ////////////////////////////////////////////////////////////////////
        #define MAKE_FREE_FUNC_TRAITS_NON_VARIADIC(CC, CALLING_CONVENTION) \
            MAKE_FREE_FUNC_TRAITS_1(CC, CALLING_CONVENTION, (ArgsT...))

        /////////////////////////////////////////////////////////////
        // Call above macro once for each calling convention (for
        // internal use only - invokes macro just above).
        /////////////////////////////////////////////////////////////
        MAKE_FREE_FUNC_TRAITS_NON_VARIADIC(STDEXT_CC_CDECL,    CallingConvention::Cdecl)
        MAKE_FREE_FUNC_TRAITS_NON_VARIADIC(STDEXT_CC_STDCALL,  CallingConvention::Stdcall)
        MAKE_FREE_FUNC_TRAITS_NON_VARIADIC(STDEXT_CC_FASTCALL, CallingConvention::Fastcall)
        #if defined(STDEXT_CC_VECTORCALL)
            MAKE_FREE_FUNC_TRAITS_NON_VARIADIC(STDEXT_CC_VECTORCALL, CallingConvention::Vectorcall)
        #endif
        #if defined(STDEXT_CC_REGCALL)
            MAKE_FREE_FUNC_TRAITS_NON_VARIADIC(STDEXT_CC_REGCALL, CallingConvention::Regcall)
        #endif

        // Done with this
        #undef MAKE_FREE_FUNC_TRAITS_NON_VARIADIC

        ////////////////////////////////////////////////
        // Constants just below not #defined otherwise
        // (applicable only when function write traits
        // supported - usually the case)
        ////////////////////////////////////////////////
        #if defined(FUNCTION_WRITE_TRAITS_SUPPORTED)
            // Done with this
            #undef REPLACE_CALLING_CONVENTION

            ////////////////////////////////////////////////////////////
            // For internal use only (we #undef it later). Used to
            // implement "FreeFunctionTraits::ReplaceCallingConvention"
            // for variadic free functions.
            ////////////////////////////////////////////////////////////
            #define REPLACE_CALLING_CONVENTION(CC, ARGS, IS_NOEXCEPT) \
                R STDEXT_CC_VARIADIC ARGS noexcept(IS_NOEXCEPT)
        #endif

        ////////////////////////////////////////////////////////////////////////
        // Macro call for internal use only. Creates partial specializations of
        // "FreeFunctionTraits" to handle variadic functions (those whose last
        // arg is "..."). Simply launches MAKE_MEMBER_FUNC_TRAITS_1 as seen
        // which creates partial specializations for handling every permutation
        // of variadic function. These are always assumed to be STDEXT_CC_CDECL
        // in this release since it's the only calling convention that supports
        // variadic functions among all the platforms we support at this
        // writing (but in the general case it's usually the calling convention
        // used for variadic functions on most platforms anyway, though some
        // platforms may have other calling conventions that support variadic
        // functions but it would be rare and we don't currently support them
        // anyway).
        ////////////////////////////////////////////////////////////////////////
        MAKE_FREE_FUNC_TRAITS_1(STDEXT_CC_VARIADIC, CallingConvention::Variadic, (ArgsT..., ...))

        // Done with these
        #undef REPLACE_CALLING_CONVENTION // Only #defined if FUNCTION_WRITE_TRAITS_SUPPORTED is #defined (usually the case but harmless to always #undef it here regardless)
        #undef MAKE_FREE_FUNC_TRAITS_1
        #undef MAKE_FREE_FUNC_TRAITS_2
        #undef DECLARE_FUNCTION_WRITE_TRAITS
        #undef FUNCTION_TRAITS_BASE_CLASS
        #undef TRAITS_FREE_FUNCTION_C

        template <TRAITS_MEMBER_FUNCTION_C F,
                  MEMBER_FUNCTION_NON_CV_POINTER_C MemberFunctionNonCvPtrT = RemoveCvRef_t<F>, // For "std::enable_if_t" purposes in our specializations. Just
                                                                                               // "F" above after removing any reference from "F" if present,
                                                                                               // and then any cv-qualifiers from the resulting member function
                                                                                               // pointer. "F" itself may be a cv-qualified pointer to a function
                                                                                               // or a reference to such a pointer so "MemberFunctionNonCvPtrT"
                                                                                               // is therefore always a non-cv-qualified pointer to a non-static
                                                                                               // member function.
                  #undef MEMBER_FUNCTION_NON_CV_POINTER_C // Done with this
                  typename = void> // For "std::enable_if_t" purposes in our specializations
        struct MemberFunctionTraits
        {
            // See this constant for details
            #if !defined(USE_CONCEPTS)
                ///////////////////////////////////////////////////
                // Kicks in if concepts not supported, otherwise
                // TRAITS_MEMBER_FUNCTION_C and
                // MEMBER_FUNCTION_NON_CV_POINTER_C concepts kick
                // in in the template declaration above instead
                // (both simply resolve to the "typename" keyword
                // when concepts aren't supported)
                ///////////////////////////////////////////////////
                static_assert(IsTraitsMemberFunction_v<F>);
                static_assert(IsMemberFunctionNonCvPointer_v<MemberFunctionNonCvPtrT>);
            #endif

            /////////////////////////////////////////////////////////////
            // By design (mandatory and always the case when relying on
            // the default arg for "MemberFunctionNonCvPtrT" which we
            // always do)
            /////////////////////////////////////////////////////////////
            static_assert(std::is_same_v<RemoveCvRef_t<F>, MemberFunctionNonCvPtrT>);

            static_assert(AlwaysFalse_v<F>, "No expected partial specialization exists for \"MemberFunctionTraits\", "
                                            "usually because its \"MemberFunctionNonCvPtrT\" template arg has an "
                                            "unsupported calling convention (though other rare reasons may be possible). "
                                            "Note that \"MemberFunctionNonCvPtrT\" is just template arg \"F\" stripped "
                                            "down to the non-cv-qualified, non-static member function pointer it refers "
                                            "to (the result of removing any reference from \"F\" if present, and then "
                                            "any cv-qualifiers from the resulting non-static member function pointer). "
                                            "\"F\" itself is therefore an unsupported type for use with this template, "
                                            "and more generally for \"FunctionTraits\" or any of its helper templates.");
        }; // struct MemberFunctionTraits

        //////////////////////////////////////////////////////////////
        // For internal use only (we #undef it later). Base class for
        // "MemberFunctionTraits" specialization further below. All
        // "FunctionTraits" specializations for non-static member
        // functions ultimately derive from this
        //////////////////////////////////////////////////////////////
        #define FUNCTION_TRAITS_BASE_CLASS(CALLING_CONVENTION, ARGS, CONST, VOLATILE, REF, IS_NOEXCEPT) \
            FunctionTraitsBase<F, \
                               R, \
                               CALLING_CONVENTION, \
                               IS_VARIADIC(ARGS), \
                               C, \
                               #CONST[0] != '\0', \
                               #VOLATILE[0] != '\0', \
                               #REF[0] == '\0' ? RefQualifier::None \
                                               : (#REF[1] == '\0' ? RefQualifier::LValue \
                                                                  : RefQualifier::RValue), \
                               IS_NOEXCEPT, \
                               ArgsT...>

        //////////////////////////////////////////////////////
        // See this constant for details. Following code is
        // added to "MemberFunctionTraits" specialization
        // further below if function write traits supported
        // (normally the case), or preprocessed out otherwise.
        // If #defined then the latter struct will contain
        // our implementation of function write traits.
        //////////////////////////////////////////////////////
        #if defined(FUNCTION_WRITE_TRAITS_SUPPORTED)
            ///////////////////////////////////////////////////////////////
            // For internal use only (we #undef it later). Used to
            // implement "MemberFunctionTraits::ReplaceCallingConvention"
            // for non-variadic, non-static member functions.
            ///////////////////////////////////////////////////////////////
            #define REPLACE_CALLING_CONVENTION(CC, ARGS, CONST, VOLATILE, REF, IS_NOEXCEPT) \
                std::tuple_element_t<static_cast<std::size_t>(NewCallingConventionT), \
                                     std::tuple<R (STDEXT_CC_CDECL C::*)ARGS CONST VOLATILE REF noexcept(IS_NOEXCEPT), \
                                                R (STDEXT_CC_STDCALL C::*)ARGS CONST VOLATILE REF noexcept(IS_NOEXCEPT), \
                                                R (STDEXT_CC_FASTCALL C::*)ARGS CONST VOLATILE REF noexcept(IS_NOEXCEPT), \
                                                R (STDEXT_CC_VECTORCALL_REPLACE(CC) C::*)ARGS CONST VOLATILE REF noexcept(IS_NOEXCEPT), \
                                                R (STDEXT_CC_THISCALL C::*)ARGS CONST VOLATILE REF noexcept(IS_NOEXCEPT), \
                                                R (STDEXT_CC_REGCALL_REPLACE(CC) C::*)ARGS CONST VOLATILE REF noexcept(IS_NOEXCEPT)> \
                                    >

            //////////////////////////////////////////////////////////
            // For internal use only (we #undef it later). Adds
            // function write traits to "struct MemberFunctionTraits"
            // below.
            //////////////////////////////////////////////////////////
            #define DECLARE_FUNCTION_WRITE_TRAITS(CC, CALLING_CONVENTION, ARGS, CONST, VOLATILE, REF, IS_NOEXCEPT) \
                private: \
                    using BaseClass = FUNCTION_TRAITS_BASE_CLASS(CALLING_CONVENTION, ARGS, CONST, VOLATILE, REF, IS_NOEXCEPT); \
                    template <typename NewF> \
                    using MigrateCvAndRef_t = typename BaseClass::template MigrateCvAndRef<F, NewF>; \
                public: \
                    using AddVariadicArgs = MigrateCvAndRef_t<R (STDEXT_CC_VARIADIC C::*)(ArgsT..., ...) CONST VOLATILE REF noexcept(IS_NOEXCEPT)>; \
                    using RemoveVariadicArgs = MigrateCvAndRef_t<R (CC C::*)(ArgsT...) CONST VOLATILE REF noexcept(IS_NOEXCEPT)>; \
                    using AddConst = MigrateCvAndRef_t<R (CC C::*)ARGS const VOLATILE REF noexcept(IS_NOEXCEPT)>; \
                    using RemoveConst = MigrateCvAndRef_t<R (CC C::*)ARGS VOLATILE REF noexcept(IS_NOEXCEPT)>; \
                    using AddVolatile = MigrateCvAndRef_t<R (CC C::*)ARGS CONST volatile REF noexcept(IS_NOEXCEPT)>; \
                    using RemoveVolatile = MigrateCvAndRef_t<R (CC C::*)ARGS CONST REF noexcept(IS_NOEXCEPT)>; \
                    using AddCV = MigrateCvAndRef_t<R (CC C::*)ARGS const volatile REF noexcept(IS_NOEXCEPT)>; \
                    using RemoveCV = MigrateCvAndRef_t<R (CC C::*)ARGS REF noexcept(IS_NOEXCEPT)>; \
                    using AddLValueReference = MigrateCvAndRef_t<R (CC C::*)ARGS CONST VOLATILE & noexcept(IS_NOEXCEPT)>; \
                    using AddRValueReference = MigrateCvAndRef_t<R (CC C::*)ARGS CONST VOLATILE && noexcept(IS_NOEXCEPT)>; \
                    using RemoveReference = MigrateCvAndRef_t<R (CC C::*)ARGS CONST VOLATILE noexcept(IS_NOEXCEPT)>; \
                    using AddNoexcept = MigrateCvAndRef_t<R (CC C::*)ARGS CONST VOLATILE REF noexcept>; \
                    using RemoveNoexcept = MigrateCvAndRef_t<R (CC C::*)ARGS CONST VOLATILE REF>; \
                    template <StdExt::CallingConvention NewCallingConventionT> \
                    using ReplaceCallingConvention = MigrateCvAndRef_t<REPLACE_CALLING_CONVENTION(CC, ARGS, CONST, VOLATILE, REF, IS_NOEXCEPT)>; \
                    template <IS_CLASS_C NewClassT> \
                    using ReplaceClass = MigrateCvAndRef_t<R (CC NewClassT::*)ARGS CONST VOLATILE REF noexcept(IS_NOEXCEPT)>; \
                    template <typename NewReturnTypeT> \
                    using ReplaceReturnType = MigrateCvAndRef_t<NewReturnTypeT (CC C::*)ARGS CONST VOLATILE REF noexcept(IS_NOEXCEPT)>; \
                    template <typename... NewArgsT> \
                    using ReplaceArgs = MigrateCvAndRef_t<std::conditional_t<BaseClass::IsVariadic, \
                                                                             R (STDEXT_CC_VARIADIC C::*)(NewArgsT..., ...) CONST VOLATILE REF noexcept(IS_NOEXCEPT), \
                                                                             R (CC C::*)(NewArgsT...) CONST VOLATILE REF noexcept(IS_NOEXCEPT)>>; \
                    template <TUPLE_C NewArgsTupleT> \
                    using ReplaceArgsTuple = typename BaseClass::template ReplaceArgsTupleImpl_t<ReplaceArgs, NewArgsTupleT>; \
                    template <std::size_t N, typename NewArgT> \
                    using ReplaceNthArg = ReplaceArgsTuple<ReplaceNthType_t<N, NewArgT, ArgsT...>>;
        #else
            ///////////////////////////////////////////////////////////
            // For internal use only (we #undef it later). Does
            // nothing however. Function write traits simply won't be
            // added to "struct MemberFunctionTraits" just below.
            ///////////////////////////////////////////////////////////
            #define DECLARE_FUNCTION_WRITE_TRAITS(CC, CALLING_CONVENTION, ARGS, CONST, VOLATILE, REF, IS_NOEXCEPT)
        #endif

        ////////////////////////////////////////////////////////////////////////////////
        // For internal use only (we #undef it later). Creates a "MemberFunctionTraits"
        // partial specialization to handle non-static member functions. Called
        // multiple times from the other macros just below for each unique combination
        // of macro parameters (in succession by the numeric suffix at the end of each
        // macro, starting at "1" and ending with this one, "5"). A partial
        // specialization will therefore exist for all permutations of non-static
        // member functions, e.g., those with different calling conventions, "const" vs
        // non-const member functions, member functions with and without "noexcept",
        // variadic functions, etc. Anytime someone uses this class to retrieve the
        // function traits for a non-static member function, the appropriate partial
        // specialization will therefore kick in (since "FunctionTraits" derives from
        // "MemberFunctionTraits" in this case)
        //
        // IMPORTANT: Note that these specializations handle pointers to non-member
        // functions only, and these pointers never have cv-qualifiers since we remove
        // them if present before arriving here (referring to cv-qualifiers on the
        // pointer itself, not cv-qualifiers on the actual function itself, which may
        // be present). References to pointers are also removed so the bottom line is
        // that these specializations handle pointers to non-static member functions
        // only (not references to such pointers), and only pointers without
        // cv-qualifiers as noted. To this end, it's the 2nd template arg that we're
        // effectively specializing on here as seen, and it handles non-cv-qualified
        // pointers only. Note that the 2nd template arg is always just template arg
        // "F" itself after removing any reference from "F" (if present), and then any
        // cv-qualifiers from what remains (so if what remains is a pointer to a
        // non-static member function, its cv-qualifiers are removed from the pointer
        // if any and the specialization then kicks in to handle that non-cv-qualified
        // pointer). Note that template arg "F" is just the original type passed to
        // "FunctionTraits" before removing the reference if present and/or any
        // cv-qualifiers on the resulting pointer (as just described), but it's not
        // involved in the specializations. We just pass it as-is to the
        // "FunctionTraitsBase" class where its stored in that class' "Type" alias so
        // users (or us) can access it if required (i.e., the original "F" they passed
        // to this class or any of its helper template declared later on).
        ///////////////////////////////////////////////////////////////////////////
        #define MAKE_MEMBER_FUNC_TRAITS_5(CC, CALLING_CONVENTION, ARGS, CONST, VOLATILE, REF, IS_NOEXCEPT) \
            template <TRAITS_MEMBER_FUNCTION_C F, \
                      typename R, \
                      IS_CLASS_C C, \
                      typename... ArgsT> \
            struct MemberFunctionTraits<F, \
                                        R (CC C::*)ARGS CONST VOLATILE REF noexcept(IS_NOEXCEPT), \
                                        std::enable_if_t<!CallingConventionReplacedWithCdecl<CALLING_CONVENTION, false>() && \
                                                         AlwaysTrue_v<F>>> \
                : FUNCTION_TRAITS_BASE_CLASS(CALLING_CONVENTION, ARGS, CONST, VOLATILE, REF, IS_NOEXCEPT) \
            { \
                static_assert(std::is_same_v<RemoveCvRef_t<F>, R (CC C::*)ARGS CONST VOLATILE REF noexcept(IS_NOEXCEPT)>); \
                DECLARE_FUNCTION_WRITE_TRAITS(CC, CALLING_CONVENTION, ARGS, CONST, VOLATILE, REF, IS_NOEXCEPT) \
            };

        //////////////////////////////////////////////////////////////////////////////////////////
        // IMPORTANT: The following macros declare all specializations of non-static member
        // function pointers. As an optimization however (premature or not), though it's unclear
        // if it's actually of any benefit (very likely negligible), the specializations are
        // declared in the order that they're mostly likely to be encountered when applied by
        // end-users, with calling convention preference first given to STDEXT_CC_CDECL and then
        // STDEXT_CC_THISCALL (read on). The first 4 specializations in particular are as
        // follows:
        //
        //    R (STDEXT_CC_CDECL C::*)(ArgsT...) const noexcept
        //    R (STDEXT_CC_CDECL C::*)(ArgsT...) const
        //    R (STDEXT_CC_CDECL C::*)(ArgsT...) noexcept
        //    R (STDEXT_CC_CDECL C::*)(ArgsT...)
        //
        // In practice these are by far the most likely functions users will be passing on
        // non-Microsoft platforms (read on for Microsoft). There are just 4 of them as seen so
        // their own order won't make much of a difference but I've put them first in the order
        // (before all others) in case a given compiler searches for matching specializations in
        // the order of declaration (so it will find a matching declaration for any of the above
        // reasonably quickly - no need to search through less frequently used specializations
        // first, i.e., those with less frequently used calling conventions, and those with the
        // "volatile" and/or "&" or "&&" qualifiers). I don't know how a given compiler might
        // actually search through the specializations however (whether declaration order
        // necessarily matters), and there's also the issue of the calling convention itself,
        // since STDEXT_CC_THISCALL is normally preferable over STDEXT_CC_CDECL when compiling
        // for 32 bits on Microsoft platforms (i.e., when compiling for 32 bits in Microsoft
        // Visual C++, non-static member functions normally default to STDEXT_CC_THISCALL instead
        // of STDEXT_CC_CDECL so ideally we'd want to check STDEXT_CC_THISCALL first instead).
        // However, when compiling for 64 bits on Microsoft platforms, STDEXT_CC_CDECL will kick
        // in instead (even on non-Microsoft platforms for most calling conventions that aren't
        // already STDEXT_CC_CDECL).
        //
        // The upshot is that we should ideally check the calling convention actually in effect
        // first if possible, regardless of what it is, so that the order of specialization
        // reflects the calling conventions most likely to be in effect depending on the platform
        // and target build (32 vs 64 bit normally). For now however we only (very) crudely order
        // the specializations to try and put some of the most common specializations first, and
        // STDEXT_CC_CDECL is given priority over STDEXT_CC_THISCALL (so things will be a tiny
        // bit slower on Microsoft platforms, since all 24 STDEXT_CC_CDECL specializations occur
        // before the first STDEXT_CC_THISCALL specialization is encountered). A future release
        // may try to do better assuming it's even doable (and can be done cleanly), but any
        // performance boost will likely (usually) be negligible anyway, no matter what the order.
        // With 144 specializations of non-static member functions we're creating at this writing
        // however (168 on Clang and Intel due to an extra calling convention they support,
        // "regcall"), for now I'm ordering things to potentially improve the performance even if
        // just a bit (again, the attempt is crude), assuming it has any appreciable effect at
        // all (but harmless if not).
        //////////////////////////////////////////////////////////////////////////////////////////

        // noexcept (for internal use only, we #undef it later - invokes macro just above)
        #define MAKE_MEMBER_FUNC_TRAITS_4(CC, CALLING_CONVENTION, ARGS, REF, VOLATILE, CONST) \
            MAKE_MEMBER_FUNC_TRAITS_5(CC, CALLING_CONVENTION, ARGS, CONST, VOLATILE, REF, true) \
            MAKE_MEMBER_FUNC_TRAITS_5(CC, CALLING_CONVENTION, ARGS, CONST, VOLATILE, REF, false)

        // const (for internal use only, we #undef it later - invokes macro just above)
        #define MAKE_MEMBER_FUNC_TRAITS_3(CC, CALLING_CONVENTION, ARGS, REF, VOLATILE) \
            MAKE_MEMBER_FUNC_TRAITS_4(CC, CALLING_CONVENTION, ARGS, REF, VOLATILE, const) \
            MAKE_MEMBER_FUNC_TRAITS_4(CC, CALLING_CONVENTION, ARGS, REF, VOLATILE, )

        // volatile (for internal use only, we #undef it later - invokes macro just above)
        #define MAKE_MEMBER_FUNC_TRAITS_2(CC, CALLING_CONVENTION, ARGS, REF) \
            MAKE_MEMBER_FUNC_TRAITS_3(CC, CALLING_CONVENTION, ARGS, REF, ) \
            MAKE_MEMBER_FUNC_TRAITS_3(CC, CALLING_CONVENTION, ARGS, REF, volatile)

        // && and && (for internal use only, we #undef it later - invokes macro just above)
        #define MAKE_MEMBER_FUNC_TRAITS_1(CC, CALLING_CONVENTION, ARGS) \
            MAKE_MEMBER_FUNC_TRAITS_2(CC, CALLING_CONVENTION, ARGS, ) \
            MAKE_MEMBER_FUNC_TRAITS_2(CC, CALLING_CONVENTION, ARGS, &) \
            MAKE_MEMBER_FUNC_TRAITS_2(CC, CALLING_CONVENTION, ARGS, &&)

        ////////////////////////////////////////////////////////////////////
        // For internal use only (we #undef it later). Invokes macro just
        // above. Launching point (first macro to be called) to create
        // partial specializations of "MemberFunctionTraits" for handling
        // every permutation of non-variadic functions for the passed
        // calling convention (variadic functions are handled just after).
        ////////////////////////////////////////////////////////////////////
        #define MAKE_MEMBER_FUNC_TRAITS_NON_VARIADIC(CC, CALLING_CONVENTION) \
            MAKE_MEMBER_FUNC_TRAITS_1(CC, CALLING_CONVENTION, (ArgsT...))

        ////////////////////////////////////////////////////////////
        // Call above macro once for each calling convention (for
        // internal use only - invokes macro just above). Note that
        // am ordering these calls by their (likely) frequency of
        // occurrence in order to potentially speed things up a bit
        // (based on partial guesswork though but see IMPORTANT
        // comments preceding MAKE_MEMBER_FUNC_TRAITS_4 above for
        // further details)
        ////////////////////////////////////////////////////////////
        MAKE_MEMBER_FUNC_TRAITS_NON_VARIADIC(STDEXT_CC_CDECL,    CallingConvention::Cdecl)
        MAKE_MEMBER_FUNC_TRAITS_NON_VARIADIC(STDEXT_CC_THISCALL, CallingConvention::Thiscall)
        MAKE_MEMBER_FUNC_TRAITS_NON_VARIADIC(STDEXT_CC_STDCALL,  CallingConvention::Stdcall)
        MAKE_MEMBER_FUNC_TRAITS_NON_VARIADIC(STDEXT_CC_FASTCALL, CallingConvention::Fastcall)
        #if defined(STDEXT_CC_VECTORCALL)
            MAKE_MEMBER_FUNC_TRAITS_NON_VARIADIC(STDEXT_CC_VECTORCALL, CallingConvention::Vectorcall)
        #endif
        #if defined(STDEXT_CC_REGCALL)
            MAKE_MEMBER_FUNC_TRAITS_NON_VARIADIC(STDEXT_CC_REGCALL, CallingConvention::Regcall)
        #endif

        // Done with this
        #undef MAKE_MEMBER_FUNC_TRAITS_NON_VARIADIC

        ////////////////////////////////////////////////
        // Constants just below not #defined otherwise
        // (applicable only when function write traits
        // supported - usually the case)
        ////////////////////////////////////////////////
        #if defined(FUNCTION_WRITE_TRAITS_SUPPORTED)
            // Done with these
            #undef REPLACE_CALLING_CONVENTION
            #undef STDEXT_CC_REGCALL_REPLACE
            #undef STDEXT_CC_VECTORCALL_REPLACE

            //////////////////////////////////////////////////////////////
            // For internal use only (we #undef it later). Used to
            // implement "MemberFunctionTraits::ReplaceCallingConvention"
            // for variadic, non-static member functions.
            //////////////////////////////////////////////////////////////
            #define REPLACE_CALLING_CONVENTION(CC, ARGS, CONST, VOLATILE, REF, IS_NOEXCEPT) \
                R (STDEXT_CC_VARIADIC C::*)ARGS CONST VOLATILE REF noexcept(IS_NOEXCEPT)
        #endif

        ////////////////////////////////////////////////////////////////////////
        // For internal use only (we #undef it later). Creates partial
        // specializations of "MemberFunctionTraits" to handle variadic
        // functions (those whose last arg is "..."). Simply launches
        // MAKE_MEMBER_FUNC_TRAITS_1 as seen which creates partial
        // specializations for handling every permutation of variadic function.
        // These are always assumed to be STDEXT_CC_CDECL in this release since
        // it's the only calling convention that supports variadic functions
        // among all the platforms we support at this writing (but in the
        // general case it's usually the calling convention used for variadic
        // functions on most platforms anyway, though some platforms may have
        // other calling conventions that support variadic functions but it
        // would be rare and we don't currently support them anyway).
        ////////////////////////////////////////////////////////////////////////
        MAKE_MEMBER_FUNC_TRAITS_1(STDEXT_CC_VARIADIC, CallingConvention::Variadic, (ArgsT..., ...))

        // Done with these
        #undef REPLACE_CALLING_CONVENTION // Only #defined if FUNCTION_WRITE_TRAITS_SUPPORTED is #defined (usually the case but harmless to always #undef it here regardless)
        #undef MAKE_MEMBER_FUNC_TRAITS_1
        #undef MAKE_MEMBER_FUNC_TRAITS_2
        #undef MAKE_MEMBER_FUNC_TRAITS_3
        #undef MAKE_MEMBER_FUNC_TRAITS_4
        #undef MAKE_MEMBER_FUNC_TRAITS_5
        #undef DECLARE_FUNCTION_WRITE_TRAITS
        #undef FUNCTION_TRAITS_BASE_CLASS
        #undef IS_VARIADIC
        #undef TRAITS_MEMBER_FUNCTION_C

        template <TRAITS_FUNCTOR_C F>
        #undef TRAITS_FUNCTOR_C // Done with this
        struct FunctorTraits : MemberFunctionTraits<decltype(&std::remove_reference_t<F>::operator())>
        {
            // See this constant for details
            #if !defined(USE_CONCEPTS)
                /////////////////////////////////////////////////////
                // Kicks in if concepts not supported, otherwise
                // TRAITS_FUNCTOR_C concept kicks in in the template
                // declaration above instead (latter simply resolves
                // to the "typename" keyword when concepts aren't
                // supported)
                /////////////////////////////////////////////////////
                static_assert(IsTraitsFunctor_v<F>);
            #endif

            //////////////////////////////////////////////////////////
            // Overrides the same member in the "FunctionTraitsBase"
            // class (which is always false). See this for details.
            //////////////////////////////////////////////////////////
            static constexpr bool IsFunctor = true;
        }; // struct FunctorTraits
    } // namespace Private

    //////////////////////////////////////////////////////
    // Restore warnings we disabled earlier (for GCC see
    // earlier call to "#pragma GCC diagnostic push")
    //////////////////////////////////////////////////////
    #if defined(GCC_COMPILER)
        #pragma GCC diagnostic pop
    //////////////////////////////////////////////////////////
    // Again, restore warnings we disabled earlier (for both
    // Clang and Intel see earlier call to "#pragma clang
    // diagnostic push", noting that Intel is based on Clang
    // so same calls handle both)
    //////////////////////////////////////////////////////////
    #elif defined(CLANG_COMPILER) || \
          defined(INTEL_COMPILER)
        #pragma clang diagnostic pop
    #endif

    //////////////////////////////////////////////////////////////////////////
    // FunctionTraits (primary template). Template that all users rely on to
    // retrieve the traits of any function, though users will normally rely
    // on the (easier) helper templates for "FunctionTraits" instead
    // (declared later on), which ultimately rely on "FunctionTraits". Note
    // that the following declaration is the primary template for
    // "FunctionTraits" but all uses of this template always rely on a
    // particular partial specialization declared just below, one for each
    // permutation of function type we may encounter (but read on). If
    // template arg "F" isn't a function type suitable for passing to this
    // template however (see comments preceding "IsTraitsFunction_v" for
    // details), then the following primary template kicks in. A compiler
    // error therefore always occurs by design (either the TRAITS_FUNCTION_C
    // concept kicks in for C++20 or later, or the "static_assert" seen in
    // the following for C++17 or earlier - in either case "F" must be
    // illegal by definition since no specialization kicked in to handle it).
    // Normally "F" will be valid though (why would anyone pass something
    // invalid unless it's for SFINAE purposes) so "F" can be any of the
    // following types (again, see comments preceding "IsTraitsFunction_v"
    // for details). Note that all pointer types in this list may contain
    // optional cv-qualifiers:
    //
    //    1) Free function (i.e., raw C++ function type excluding abominable
    //       functions - see "IsFreeFunction()" for details)
    //    2) Pointer to free function
    //    3) Reference to free function
    //    4) Reference to pointer to free function
    //    5) Pointer to non-static member function
    //    6) Reference to pointer to non-static member function (references
    //       to non-static member functions not supported by C++ itself,
    //       only references to pointer are).
    //    7) Functor (or reference to functor) including lambdas
    //
    // IMPORTANT:
    // ---------
    // Given the types we support as seen above, including optional
    // cv-qualifiers on any of the pointer types, and then factoring in
    // calling conventions on the target function, as well as "noexcept" and
    // for non-static member functions or functors, cv and ref qualifiers,
    // we're dealing with 1878 permutations (required specializations) on all
    // platforms we currently support (a bit more for Clang and Intel
    // compilers which have an extra calling convention, "regcall"). Although
    // this can easily be handled in a relatively small amount of code using
    // the macros seen further above (used to create these specializations -
    // they handled all 1878 specializations in earlier releases), we've
    // since adjusted these macros to handle a much smaller number of
    // specializations (154). We still target all 1878 permutations of
    // function types however by specializing "F" *after* removing the
    // optional reference and/or pointer to "F" if it targets a free function
    // (resulting in the actual free function type itself) , or the optional
    // reference and cv-qualifiers if "F" targets a non-static member
    // function (resulting in a non-cv-qualified, non-static member function
    // pointer - functors are routed through here as well). We rely on these
    // removals to (effectively) specialize "FunctionTraits" on "F" *after*
    // these removals, not on "F" itself, which reduces the number of
    // specializations to 154. After these removals on "F" that is, we only
    // require specializations for either raw free function types (never
    // pointers to free functions or references to free functions or
    // references to pointers to free functions), or (non-cv-qualified)
    // pointers to non-static member functions (and never references to such
    // pointers). This greatly reduces the number of specializations we need
    // to handle as noted, again, from 1878 to 154, but we still capture the
    // same information for all 1878 permutations because these removals
    // don't impact our ability to collect this information. We can capture
    // the same info as before using only 154 specializations.
    //
    // The upshot is that these removals are designed solely to reduce the
    // large number of specializations of "FunctionTraits" that would
    // otherwise be required (1878), without affecting our ability to capture
    // the exact same info in all these specializations (which we now do
    // using only 154 specializations).
    //
    // Finally, please be aware that when "F" is in fact a supported function
    // type, its calling convention may be changed by the compiler to the
    // "cdecl" calling convention. This normally occurs when compiling for 64
    // bits opposed to 32 bits, or possibly other scenarios depending on the
    // compiler options in effect (platform specific), in which case the
    // compiler ignores the calling convention on "F" and automatically
    // switches to the "cdecl" calling convention. Note that our
    // "FreeFunctionTraits" and "MemberFunctionTraits" specializations
    // further above which handle calling conventions are designed to detect
    // this situation and when it occurs, the specializations for all calling
    // conventions besides "cdecl" itself therefore won't kick in (such as
    // when compiling for 64 bits as noted - the compiler always switches to
    // the "cdecl" calling convention in this case unless a given calling
    // convention is supported in that environment, normally the "vectorcall"
    // calling convention only at this writing). The primary template for
    // "FreeFunctionTraits" and "MemberFunctionTraits" themselves would
    // therefore normally kick in instead in this situation (since no
    // specializations now exist for these calling conventions - we removed
    // them), but it will never happen because these calling conventions will
    // never be encountered in "F". The "cdecl" calling convention always
    // will be and a specialization always exists for it. For instance, if
    // function "F" is declared with the "stdcall" calling convention
    // (whatever its syntax is for a given compiler), and you compile for 64
    // bits opposed to 32 bits, the compiler will ignore the "stdcall"
    // calling convention on "F" and use "cdecl" instead (for all compilers
    // we currently support). No specialization is therefore required for
    // "stdcall" so the specialization that handles "stdcall" won't kick in
    // (based on our design), since the "cdecl" specialization will handle
    // it. IOW, even though there's no specialization for functions with the
    // "stdcall" calling convention, it doesn't matter since the "stdcall" in
    // function "F" has been replaced with "cdecl" by the compiler. The
    // primary template therefore won't kick in for functions declared with
    // "stdcall", the specialization that handles "cdecl" will. The primary
    // template only kicks in if "F" isn't a supported function type (someone
    // passes an "int" for instance which isn't even a function), in which
    // case either the TRAITS_FUNCTION_C concept (C++20 or later) or
    // "static_assert" (C++17 or earlier) in the following struct will
    // trigger (see below). Specializations whose calling convention is
    // replaced by "cdecl" therefore never wind up here as described, so no
    // error will ever be flagged for them (they're still supported functions
    // that is, only that the "cdecl" specialization will now kick in to
    // handle them instead of the usual specialization that normally targets
    // that specific calling convention).
    ////////////////////////////////////////////////////////////////////////////
    template <TRAITS_FUNCTION_C F,
              typename = void> // For "std::enable_if_t" purposes in our specializations
    struct FunctionTraits
    {
        // See this constant for details
        #if !defined(USE_CONCEPTS)
            /////////////////////////////////////////////////////
            // Kicks in if concepts not supported, otherwise
            // TRAITS_FUNCTION_C concept kicks in in the
            // template declaration above instead (latter simply
            // resolves to the "typename" keyword when concepts
            // aren't supported)
            /////////////////////////////////////////////////////
            static_assert(AlwaysFalse_v<F>, "\"F\" isn't a function type suitable for passing to \"FunctionTraits\" or any of its " \
                                            "helper templates. See comments preceding \"StdExt::IsTraitsFunction_v\" for details " \
                                            "but for all intents and purposes any legal type identifying a function will normally " \
                                            "do (i.e., free functions which include static members functions, pointers and references " \
                                            "to free functions, references to pointers to free functions, pointers to non-static " \
                                            "member functions, references to pointers to non-static member functions, and " \
                                            "(non-overloaded) functors or references to functors). \"F\" doesn't qualify as any of " \
                                            "these.");
        #endif
    }; // struct FunctionTraits

    ////////////////////////////////////////////////////////////////////////
    // "FunctionTraits" partial specialization for handling free functions
    // including static member functions. "F" can be any of the following:
    //
    //   1) A free function including static member functions
    //   2) An (optionally cv-qualified) pointer to a free function
    //   3) A reference to a free function
    //   4) A reference to an (optionally cv-qualified) pointer to a free
    //      function
    ////////////////////////////////////////////////////////////////////////
    template <typename F>
    struct FunctionTraits<F,
                          std::enable_if_t<Private::IsTraitsFreeFunction_v<F>>> : // True if "F" is a free function or a
                                                                                  // pointer to one or a reference to one
                                                                                  // or a reference to a pointer to one
        Private::FreeFunctionTraits<F>
    {
    };

    /////////////////////////////////////////////////////////////////////////////
    // "FunctionTraits" partial specialization for handling non-static member
    // functions. "F" can be either of the following:
    //
    //   1) An (optionally cv-qualified) pointer to a non-static member function
    //   2) A reference to an (optionally cv-qualified) pointer to a non-static
    //      member function
    /////////////////////////////////////////////////////////////////////////////
    template <typename F>
    struct FunctionTraits<F,
                          std::enable_if_t<Private::IsTraitsMemberFunction_v<F>>> : // True if "F" is a pointer to a
                                                                                    // non-static member function or
                                                                                    // a reference to such a pointer
        Private::MemberFunctionTraits<F>
    {
    };

    //////////////////////////////////////////////////////////////////////////
    // "FunctionTraits" partial specialization for handling function objects
    // (AKA functors). "F" can be either a functor or a reference to one. In
    // either case this includes lambdas which are just syntactic sugar for
    // creating function objects on the fly (the compiler creates a hidden
    // functor behind the scenes - note that generic lambdas can't be
    // targeted using functor syntax but can be targeted using
    // pointer-to-member syntax as seen in example 3 below). Template arg "F"
    // therefore isn't a function type in this specialization (unlike all
    // other specializations which handle function types and pointers and
    // references to such types), but a class or struct type with a single
    // "F::operator()" member function (the class or struct type itself is
    // known as the lambda's "closure type"). The traits created by this
    // specialization are therefore for that function. Note that overloads of
    // "F::operator()" aren't supported however since it would be ambiguous
    // (which "F::operator()" to use). In this case the following template
    // will fail SFINAE (due to the ambiguity) so the primary template will
    // kick in instead (which results in a "static_assert", i.e., the alias
    // template can't be specialized).
    //
    //     Example 1 (functor)
    //     -------------------
    //     class MyFunctor
    //     {
    //     public:
    //          int operator()(float, const std::string &);
    //     };
    //
    //     ////////////////////////////////////////////////////////////
    //     // Resolves to "int". Following helper template relies on
    //     // "FunctionTraits" to determine this (passing a functor
    //     // here). See it for details.
    //     ////////////////////////////////////////////////////////////
    //     using MyClassReturnType_t = ReturnType_t<MyFunctor>
    //
    //     ////////////////////////////////////////////////////////////
    //     // Resolves to "const std::string &" (passing 1 here which
    //     // is the zero-based index of the arg we want). Following
    //     // helper template relies on "FunctionTraits" to determine
    //     // this. See it for details.
    //     ////////////////////////////////////////////////////////////
    //     using Arg2Type_t = ArgType_t<MyFunctor, 1>
    //
    //     Example 2 (lambda - note that lambdas are just functors)
    //     --------------------------------------------------------
    //     const auto myLambda = [](float val, const std::string &str)
    //                             {
    //                                 int rc;
    //                                 // ...
    //                                 return rc;
    //                             };
    //
    //     ///////////////////////////////////////////////////////////////
    //     // Resolves to "int" (lambda's return type). Following helper
    //     // template relies on "FunctionTraits" to determine this. See
    //     // it for details.
    //     ///////////////////////////////////////////////////////////////
    //     using MyLambdaReturnType_t = ReturnType_t<decltype(myLambda)>;
    //
    //     ////////////////////////////////////////////////////////////
    //     // Resolves to "const std::string &" (passing 1 here which
    //     // is the zero-based index of the arg we want). Following
    //     // helper template relies on "FunctionTraits" to determine
    //     // this. See it for details.
    //     ////////////////////////////////////////////////////////////
    //     using Arg2Type_t = ArgType_t<decltype(myLambda), 1>;
    //
    //     Example 3 (generic lambda). See comments below.
    //     -----------------------------------------------
    //     const auto myLambda = [](auto val, const std::string &str)
    //                             {
    //                                 int rc;
    //                                 // ...
    //                                 return rc;
    //                             };
    //
    //     //////////////////////////////////////////////////////////////
    //     // Generic lambdas, similar to lambda templates in C++20,
    //     // can't be handled using functor syntax, i.e., by passing
    //     // the lambda's type like in "Example 2" above (to
    //     // "FunctionTraits" or any of its helper templates). That's
    //     // because for all lambdas, generic or not, C++ creates a
    //     // hidden functor behind the scenes whose "operator()" member
    //     // has the same args as the lambda itself, but when a generic
    //     // lambda is created, such as the one in "Example 3", a
    //     // template version of "operator()" is created instead (to
    //     // handle the lambda's "auto" parameters). The functor's
    //     // class (i.e., it's "closure type") is therefore not really
    //     // a functor in the conventional sense since "operator()" is
    //     // actually a function template (unlike for non-generic
    //     // lambdas where "operator()" isn't a template). In "Example
    //     // 3" above for instance, C++ will create a class behind the
    //     // scenes that looks something like the following (which
    //     // we're calling "Lambda" here but whatever internal name the
    //     // compiler actually assigns it - it's an implementation
    //     // detail we don't care about):
    //     //
    //     //     class Lambda
    //     //     {
    //     //     public:
    //     //         template<class T>
    //     //         inline auto operator()(T val, const std::string &str) const
    //     //         {
    //     //            int rc;
    //     //            // ...
    //     //            return rc;
    //     //         }
    //     //     };
    //     //
    //     // You therefore can't simply pass "decltype(myLambda)" to
    //     // "FunctionTraits" or any of its helper templates like you
    //     // could in "Example 2" because "operator()" is now a
    //     // template. It therefore needs to be specialized on template
    //     // arg "T" first, unlike "Example 2" whose own "operator()"
    //     // member isn't a template (it's a non-template function).
    //     // Dealing with "myLambda" in "Example 3" is therefore
    //     // different than for "Example 2" because you can't pass
    //     // "decltype(myLambda)" to "FunctionTraits" or any of its
    //     // helper templates (since the compiler doesn't know the
    //     // function template arguments for "operator()" until you
    //     // explicitly tell it). To do so you need to instantiate the
    //     // underlying "Lambda::operator()" template in "Example 3"
    //     // and access it using pointer-to-member function syntax
    //     // instead of functor syntax. The syntax for doing so is
    //     // ugly compared to normal functor syntax but you can always
    //     // create a helper alias to clean it up a bit if you wish
    //     // (though we don't do that here). The following example
    //     // demonstrates this by creating alias "F" which specializes
    //     // "operator()" on type "float" and then takes its address
    //     // as seen (details to follow). "F" in the following call
    //     // therefore effectively resolves to the following type:
    //     //
    //     //     int (Lambda::*)(float, const std::string &) const
    //     //
    //     // We therefore have an ordinary member function pointer type
    //     // that we can pass to "FunctionTraits" or any of its helper
    //     // templates. From this point on everything works the same as
    //     // "Example 2" above except "F" is the functor's class type
    //     // in that example and a pointer to a non-static member
    //     // function in the following example (but the 1st arg in each
    //     // example is still a "float" - the only difference for our
    //     // purposes is how we pass the lambda to "FunctionTraits" or
    //     // any of its helper functions, by using functor syntax in
    //     // "Example 2" and pointer-to-member function syntax in
    //     // "Example 3")
    //     //
    //     // Lastly, note that in the following (cryptic) call, the
    //     // call to "decltype(myLambda)" simply returns the type of
    //     // "myLambda" (i.e., the compiler-generated class for this
    //     // which is just "class Lambda" seen above), and we then
    //     // specialize its "operator()" member on type "float". By
    //     // applying the "&" operator to the result, we therefore
    //     // get back a pointer to "operator()" which is now just an
    //     // ordinary member function specialized on "float" (so we
    //     // have a pointer to this function). The outer "decltype"
    //     // then returns the type of this pointer as seen above.
    //     // Again, the syntax is nasty but a helper alias can always
    //     // be written to clean it up a bit (and may be made available
    //     // in a future release of this header). Note that creating a
    //     // specialization of "FunctionTraits" itself to handle this
    //     // syntax instead isn't viable for reasons beyond the scope
    //     // of this documentation (such a specialization would require
    //     // the caller to pass the type(s) that "operator()" needs to
    //     // be specialized on which introduces unwieldy complications).
    //     //////////////////////////////////////////////////////////////
    //     using F = decltype(&decltype(myLambda)::template operator()<float>); // See above for details. A helper alias
    //                                                                          // to make the syntax cleaner will likely
    //                                                                          // be made available in a future release
    //                                                                         //  of this header.
    //
    //     ///////////////////////////////////////////////////////
    //     // Resolves to "int" (lambda's return type) just like
    //     // "Example 2".
    //     ///////////////////////////////////////////////////////
    //     using MyLambdaReturnType_t = ReturnType_t<F>;
    //
    //     ////////////////////////////////////////////////////////
    //     // Resolves to "const std::string &", again, just like
    //     // "Example 2" (and again, passing 1 here which is the
    //     // zero-based index of the arg we want).
    //     ////////////////////////////////////////////////////////
    //     using Arg2Type_t = ArgType_t<F, 1>;
    //////////////////////////////////////////////////////////////////////////
    template <typename F>
    struct FunctionTraits<F,
                          std::enable_if_t<Private::IsTraitsFunctor_v<F>>> : // True if "F" is a functor
                                                                             // or a reference to one
        Private::FunctorTraits<F>
    {
    };

    //////////////////////////////////////////////////////////////////////////
    // "IsFunctionTraits" (primary template). Primary template inherits from
    // "std::false_type" if the template arg is *not* a specialization of
    // struct "FunctionTraits". Otherwise, if it is a specialization of
    // "FunctionTraits" then the specialization just below kicks in instead
    // (inheriting from "std::true_type")
    //
    //    Example (see FUNCTION_TRAITS_C declaration further below)
    //    ---------------------------------------------------------
    //    template <FUNCTION_TRAITS_C F>
    //    class Whatever
    //    {
    //       // See this constant for details
    //       #if !defined(USE_CONCEPTS)
    //            STATIC_ASSERT_IS_FUNCTION_TRAITS(F);
    //       #endif
    //     };
    //
    //    ///////////////////////////////////////////////////////////////
    //    // "static_assert" (C++17 or earlier) or concept (C++20 or
    //    // later) succeeds above (template arg is a "FunctionTraits")
    //    ///////////////////////////////////////////////////////////////
    //    Whatever<FunctionTraits<int ()> whatever1;
    //
    //    //////////////////////////////////////////////////////////////////
    //    // "static_assert" (C++17 or earlier) or concept (C++20 or later)
    //    //  fails above (template arg is *not* a "FunctionTraits")
    //    //////////////////////////////////////////////////////////////////
    //    Whatever<int> whatever2;
    //////////////////////////////////////////////////////////////////////////
    template <typename>
    struct IsFunctionTraits : public std::false_type
    {
    };

    //////////////////////////////////////////////////////////////////
    // Specialization of "IsFunctionTraits" (primary) template just
    // above. This specialization does all the work. See primary
    // template above for details.
    //////////////////////////////////////////////////////////////////
    template <typename F, typename EnableIf>
    struct IsFunctionTraits<FunctionTraits<F, EnableIf>> : public std::true_type
    {
    };

    //////////////////////////////////////////////////////////////////
    // Helper variable template for "IsFunctionTraits" above (with
    // the usual "_v" suffix). Set to true if "T" is a specialization
    // of "FunctionTraits" or false otherwise.
    //////////////////////////////////////////////////////////////////
    template <typename T>
    inline constexpr bool IsFunctionTraits_v = IsFunctionTraits<T>::value;
#endif // #if !defined(DECLARE_MACROS_ONLY)

    //////////////////////////////////////////////
    // Concept for above template (see following
    // #defined constant for details)
    //////////////////////////////////////////////
    #if defined(USE_CONCEPTS)
        #if !defined(DECLARE_MACROS_ONLY)
            template <typename T>
            concept FunctionTraits_c = IsFunctionTraits_v<T>;
        #endif

        #define FUNCTION_TRAITS_C StdExt::FunctionTraits_c
    #else
        #define STATIC_ASSERT_IS_FUNCTION_TRAITS(T) static_assert(StdExt::IsFunctionTraits_v<T>, \
                                                                  "\"" #T "\" must be a \"FunctionTraits\" specialization");
        #define FUNCTION_TRAITS_C typename
    #endif

#if !defined(DECLARE_MACROS_ONLY)
    /////////////////////////////////////////////////////////////////////////
    // FunctionTraitsArgCount_v. Helper template yielding
    // "FunctionTraitsT::ArgCount" (number of args in the function
    // represented by "FunctionTraits" not including variadic args if any),
    // where "FunctionTraitsT" is a "FunctionTraits" specialization. Note
    // that there's usually no benefit to this particular template however
    // compared to calling "FunctionTraitsT::ArgCount" directly (there's no
    // reduction in verbosity), but it's provided for consistency anyway (to
    // ensure a helper template is available for all members
    // of"FunctionTraits"). In most cases you should rely on the
    // "ArgCount_v" helper template instead however, which just defers to
    // "FunctionTraitsArgCount_v" and is easier to use.
    //
    // IMPORTANT:
    // ---------
    // Please note that if you wish to check if a function's argument list
    // is completely empty, then inspecting this helper template for zero
    // (0) is not sufficient, since it may return zero but still contain
    // variadic args. To check for a completely empty argument list, call
    // "FunctionTraitsIsEmptyArgList_v" instead. See this for further
    // details.
    //
    // Lastly, note that the "FunctionTraitsT" template arg must be a
    // "FunctionTraits" specialization or compilation will normally fail
    // (concept kicks in in C++20 or later so failure is guaranteed - in
    // C++17 or earlier however there is no guarantee compilation will fail
    // but usually will for other reasons - longer story but since C++20 or
    // later is fast becoming the norm we won't worry about it - pass the
    // expected "FunctionTraits" template arg and everything will be fine).
    /////////////////////////////////////////////////////////////////////////
    template <FUNCTION_TRAITS_C FunctionTraitsT>
    inline constexpr std::size_t FunctionTraitsArgCount_v = FunctionTraitsT::ArgCount;

    /////////////////////////////////////////////////////////////////////////
    // FunctionTraitsArgType_t. Helper alias template yielding
    // "FunctionTraitsT::Args" (the type of the zero-based "Ith" arg of the
    // function represented by "FunctionTraitsT"), where "FunctionTraitsT"
    // is a "FunctionTraits" specialization. Note that it's less verbose
    // than accessing the "Args" alias in "FunctionTraits" directly so you
    // should normally rely on the following instead (or preferably the
    // "ArgType_t" helper alias instead, which just defers to
    // "FunctionTraitsArgType_t" and is even easier to use).  Note that "I"
    // must be less than the number args in the function represented by
    // "FunctionTraits" or a compiler error occurs. If this function has
    // no args or variadic args only (if it's variadic, i.e., the last arg
    // is "..."), then a compiler error will always occur so this function
    // should never be called in that case (since no args exist). You can
    // check "FunctionTraitsArgCount_v" for this and if zero then it's not
    // legal to use "FunctionTraitsArgType_t". Note that variadic args are
    // never included so "I" can't be used to target them (they're
    // effectively ignored as far as "I" is concerned). Lastly, note that
    // the "FunctionTraitsT" template arg must be a "FunctionTraits"
    // specialization or compilation will normally fail (concept kicks in
    // in C++20 or later so failure is guaranteed - in C++17 or earlier
    // however there is no guarantee compilation will fail but usually will
    // for other reasons - longer story but since C++20 or later is fast
    // becoming the norm we won't worry about it - pass the expected
    // "FunctionTraits" template arg and everything will be fine).
    //
    // E.g.,
    //
    //     void MyFunction(const std::string &str, int val)
    //     {
    //     }
    //
    //     // "FunctionTraits" for above function
    //     using MyFunctionTraits = FunctionTraits<decltype(MyFunction)>;
    //     // using MyFunctionTraits = FunctionTraits<void (const std::string &, int)>; // This will also work (same as above but passing function's type on-the-fly)
    //     // using MyFunctionTraits = FunctionTraits<void (*)(const std::string &, int)>; // ... and this too (but now using a pointer to the function - references
    //                                                                                     // to the function will also work, and references to pointers to the
    //                                                                                     // function as well)
    //
    //     ////////////////////////////////////////////////////////
    //     // Type of second arg of "MyFunction" above (an "int").
    //     // Passing 1 here since index is zero-based.
    //     ////////////////////////////////////////////////////////
    //     using MyFunctionArg2Type = FunctionTraitsArgType_t<MyFunctionTraits, 1>;
    /////////////////////////////////////////////////////////////////////////
    template <FUNCTION_TRAITS_C FunctionTraitsT, std::size_t I /* Zero-based */>
    using FunctionTraitsArgType_t = typename FunctionTraitsT::template Args<I>::Type;

    //////////////////////////////////////////////////////////////////////////
    // FunctionTraitsArgTypeName(). See "FunctionTraitsArgType_t" just above.
    // Simply converts that to a string suitable for display purposes and
    // returns it as a "tstring_view"
    //////////////////////////////////////////////////////////////////////////
    template <FUNCTION_TRAITS_C FunctionTraitsT, std::size_t I /* Zero-based */>
    inline constexpr tstring_view FunctionTraitsArgTypeName_v = TypeName_v<FunctionTraitsArgType_t<FunctionTraitsT, I>>;

    ////////////////////////////////////////////////////////////////////////////
    // FunctionTraitsArgTypes_t. Helper alias yielding
    // "FunctionTraitsT::ArgTypes" (a "std::tuple" storing all arg types for
    // the function represented by "FunctionTraits"), where "FunctionTraitsT"
    // is a "FunctionTraits" specialization. Note that it's less verbose than
    // accessing "FunctionTraits::ArgTypes" directly so you should normally
    // rely on the following instead (or preferably the "ArgTypes_t" helper
    // alias instead, which just defers to "FunctionTraitsArgTypes_t" and is
    // even easier to use). Note that using this alias is rarely ever required
    // in practice however since arg types can be individually accessed using
    // the "FunctionTraitsArgType_t" helper instead (see this for details).
    // Normally "FunctionTraitsArgTypes_t" only needs to be accessed if you
    // need to iterate all the arg types for some reason but if so then you can
    // just invoke helper function (template) "ForEachArg()" which exists for
    // this purpose. See this for details. Lastly, note that the
    // "FunctionTraitsT" template arg must be a "FunctionTraits" specialization
    // or compilation will normally fail (concept kicks in C++20 or later so
    // failure is guaranteed - in C++17 or earlier however there is no
    // guarantee compilation will fail but usually will for other reasons -
    // longer story but since C++20 or later is fast becoming the norm we won't
    // worry about it - pass the expected "FunctionTraits" template arg and
    // everything will be fine).
    //
    // E.g.,
    //
    //       void MyFunction(const std::string &str, int val)
    //       {
    //       }
    //
    //       using MyFunctionTraits = FunctionTraits<decltype(MyFunction)>;
    //       // using MyFunctionTraits = FunctionTraits<void (const std::string &, int)>; // This will also work (same as above but passing function's type on-the-fly)
    //       // using MyFunctionTraits = FunctionTraits<void (*)(const std::string &, int)>; // ... and this too (but now using a pointer to the function so the type
    //                                                                                       // will now reflect that - references to the function will also work, and
    //                                                                                       // references to pointers to the function as well)
    //
    //       ////////////////////////////////////////////////////////////////
    //       // "ArgTypes" for "MyFunction" above. A std::tuple" containing
    //       // all args in the function so its size will be 2 in this case
    //       // (first type a "const std::string &" and 2nd type an "int",
    //       // as seen in the function above). Note that you can use
    //       // "std::is_same" to compare this with the "ArgTypes" in another
    //       // "FunctionTraits" to determine if they contain the exact same
    //       // argument types.
    //       ////////////////////////////////////////////////////////////////
    //       using MyFunctionArgTypes = FunctionTraitsArgTypes_t<MyFunctionTraits>;
    /////////////////////////////////////////////////////////////////////////
    template <FUNCTION_TRAITS_C FunctionTraitsT>
    using FunctionTraitsArgTypes_t = typename FunctionTraitsT::ArgTypes;

    //////////////////////////////////////////////////////////////////////
    // FunctionTraitsCallingConvention_v. Helper template yielding
    // "FunctionTraitsT::CallingConvention" (the calling convention of
    // the function represented by "FunctionTraitsT"), where
    // "FunctionTraitsT" is a "FunctionTraits" specialization. Note that
    // there's usually no benefit to this particular helper template
    // however compared to calling "FunctionTraitsT::CallingConvention"
    // directly (there's no reduction in verbosity) but it's provided for
    // consistency anyway (to ensure a helper template is available for
    // all members of "FunctionTraits"). In most cases you should rely on
    // "CallingConvention_v" helper template instead however, which just
    // defers to "FunctionTraitsCallingConvention_v" and is easier to
    // use.
    //
    // Please note that compilers will sometimes change the explicitly
    // declared calling convention of a function to "cdecl" (whatever the
    // syntax is for this on the given platform) depending on the
    // compiler options in effect, in particular when compiling for 64
    // bits. In these cases the following will usually return
    // "CallingConvention::Cdecl" regardless of what the calling
    // convention is explicitly set to on the function (since "cdecl" is
    // the actual calling convention applied by the compiler - some
    // exceptions may exist however, such as the "vectorcall" calling
    // convention which remains unchanged in 64 bit environments, at
    // least on the platforms we currently support and probably all
    // others as well). Lastly, note that the "FunctionTraitsT" template
    // arg must be a "FunctionTraits" specialization or compilation will
    // normally fail (concept kicks in in C++20 or later so failure is
    // guaranteed - in C++17 or earlier however there is no guarantee
    // compilation will fail but usually will for other reasons - longer
    // story but since C++20 or later is fast becoming the norm we won't
    // worry about it - pass the expected "FunctionTraits" template arg
    // and everything will be fine).
    //////////////////////////////////////////////////////////////////////
    template <FUNCTION_TRAITS_C FunctionTraitsT>
    inline constexpr CallingConvention FunctionTraitsCallingConvention_v = FunctionTraitsT::CallingConvention;

    ///////////////////////////////////////////////////////////////////////////////////
    // FunctionTraitsCallingConventionName_v. See "FunctionTraitsCallingConvention_v"
    // just above. Simply converts that to a string suitable for display purposes and
    // returns it as a "tstring_view"
    ///////////////////////////////////////////////////////////////////////////////////
    template <FUNCTION_TRAITS_C FunctionTraitsT>
    inline constexpr tstring_view FunctionTraitsCallingConventionName_v = CallingConventionToString(FunctionTraitsCallingConvention_v<FunctionTraitsT>);

    /////////////////////////////////////////////////////////////////////////
    // FunctionTraitsFunctionType_t. Helper alias yielding "FunctionTraitsT::Type"
    // (the type of the function represented by "FunctionTraitsT"), where
    // "FunctionTraitsT" is a "FunctionTraits" specialization. Note that
    // it's less verbose than accessing "FunctionTraits::Type" directly so
    // you should normally rely on the following instead (or preferably the
    // "FunctionType_t" helper alias instead, which just defers to
    // "FunctionTraitsFunctionType_t" and is even easier to use). Note that the
    // "FunctionTraitsT" template arg must be a "FunctionTraits"
    // specialization or compilation will normally fail (concept kicks in
    // in C++20 or later so failure is guaranteed - in C++17 or earlier
    // however there is no guarantee compilation will fail but usually will
    // for other reasons - longer story but since C++20 or later is fast
    // becoming the norm we won't worry about it - pass the expected
    // "FunctionTraits" template arg and everything will be fine).
    //
    // E.g.,
    //
    //       void MyFunction(const std::string &str, int val)
    //       {
    //       }
    //
    //       // "FunctionTraits" for above function
    //       using MyFunctionTraits = FunctionTraits<decltype(MyFunction)>;
    //       // using MyFunctionTraits = FunctionTraits<void (const std::string &, int)>; // This will also work (same as above but passing function's type on-the-fly)
    //       // using MyFunctionTraits = FunctionTraits<void (*)(const std::string &, int)>; // ... and this too (but now using a pointer to the function so the type
    //                                                                                       // will now reflect that - references to the function will also work, and
    //                                                                                       // references to pointers to the function as well)
    //
    //       //////////////////////////////////////////////////////////////
    //       // Type of "MyFunction" above. Yields the following (though
    //       // it might include the calling convention as well depending
    //       // on the platform):
    //       //
    //       //     void (const std::string &, int)
    //       //////////////////////////////////////////////////////////////
    //       using MyFunctionType = FunctionTraitsFunctionType_t<MyFunctionTraits>;
    /////////////////////////////////////////////////////////////////////////
    template <FUNCTION_TRAITS_C FunctionTraitsT>
    using FunctionTraitsFunctionType_t = typename FunctionTraitsT::Type;

    ////////////////////////////////////////////////////////////////////
    // FunctionTraitsTypeName. See "FunctionTraitsFunctionType_t" just
    // above. Simply converts that to a string suitable for display
    // purposes and returns it as a "tstring_view"
    ////////////////////////////////////////////////////////////////////
    template <FUNCTION_TRAITS_C FunctionTraitsT>
    inline constexpr tstring_view FunctionTraitsTypeName_v = TypeName_v<FunctionTraitsFunctionType_t<FunctionTraitsT>>;

    ///////////////////////////////////////////////////////////////////////////
    // FunctionTraitsIsArgTypeSame_v. Helper template yielding "true" if the
    // (zero-based) "Ith" arg of the function represented by "FunctionTraitsT"
    // is the same as the given type "T" or false otherwise. The function is
    // just a thin wrapper around "std::is_same_v", where the latter template
    // is passed the type of the "Ith" arg in function "FunctionTraitsT" as
    // its 1st template arg, and "T" as its 2nd template arg. IOW, it simply
    // compares the type of the "Ith" arg in function "FunctionTraitsT" with
    // type "T". This is a common requirement for many users so this template
    // provides a convenient wrapper.
    // 
    //    using F = int SomeFunc(int, float, double);
    //    using SomeFuncTraits = FunctionTraits<F>;
    // 
    //    //////////////////////////////////////////////////////////////////
    //    // Identical to the following but more convenient (and less
    //    // verbose):
    //    //
    //    // using Arg2_t = FunctionTraitsArgType_t<SomeFuncTraits, 1>;
    //    // constexpr bool Is2ndArgAFloat = std::is_same_v<Arg2_t, float // true
    //    //////////////////////////////////////////////////////////////////
    //    constexpr bool Is2ndArgAFloat = FunctionTraitsIsArgTypeSame_v<SomeFuncTraits, 1, float>;
    //    
    ///////////////////////////////////////////////////////////////////////////
    template <FUNCTION_TRAITS_C FunctionTraitsT, std::size_t I /* Zero-based */, typename T>
    inline constexpr bool FunctionTraitsIsArgTypeSame_v = std::is_same_v<FunctionTraitsArgType_t<FunctionTraitsT, I>, T>;

    ///////////////////////////////////////////////////////////////////////////
    // FunctionTraitsIsFreeFunction_v. Helper template yielding
    // "FunctionTraitsT::IsFreeFunction" (true or false to indicate if the
    // function represented by "FunctionTraitsT" is a free function including
    // static member functions), where "FunctionTraitsT" is a "FunctionTraits"
    // specialization. Note that there's usually no benefit to this particular
    // template however compared to calling "FunctionTraitsT::IsFreeFunction"
    // directly (there's no reduction in verbosity), but it's provided for
    // consistency anyway (to ensure a template is available for all members
    // of "FunctionTraits"). In most cases you should rely on the
    // "IsFreeFunction_v" helper template instead however, which just defers
    // to "FunctionTraitsIsFreeFunction_v" and is easier to use.
    //
    // Lastly, note that the "FunctionTraitsT" template arg must be a
    // "FunctionTraits" specialization or compilation will normally fail
    // (concept kicks in in C++20 or later so failure is guaranteed - in C++17
    // or earlier however there is no guarantee compilation will fail but
    // usually will for other reasons - longer story but since C++20 or later
    // is fast becoming the norm we won't worry about it - pass the expected
    // "FunctionTraits" template arg and everything will be fine).
    ///////////////////////////////////////////////////////////////////////////
    template <FUNCTION_TRAITS_C FunctionTraitsT>
    inline constexpr bool FunctionTraitsIsFreeFunction_v = FunctionTraitsT::IsFreeFunction;

    /////////////////////////////////////////////////////////////////////////
    // FunctionTraitsIsFunctor_v. Helper template yielding
    // "FunctionTraitsT::IsFunctor" (true or false to indicate if the
    // function represented by "FunctionTraitsT" is a functor), where
    // "FunctionTraitsT" is a "FunctionTraits" specialization. If "true"
    // then note that "FunctionTraitsIsMemberFunction_v" is also guaranteed
    // to return true. Please note that there's usually no benefit to this
    // particular helper template however compared to calling
    // "FunctionTraitsT::IsFunctor" directly (there's no reduction in
    // verbosity), but it's provided for consistency anyway (to ensure a
    // helper template is available for all members of "FunctionTraits"). In
    // most cases you should rely on the "IsFunctor_v" helper template
    // instead however, which just defers to "FunctionTraitsIsFunctor_v" and
    // is easier to use.
    //
    // Lastly, note that the "FunctionTraitsT" template arg must be a
    // "FunctionTraits" specialization or compilation will normally fail
    // (concept kicks in in C++20 or later so failure is guaranteed - in
    // C++17 or earlier however there is no guarantee compilation will fail
    // but usually will for other reasons - longer story but since C++20 or
    // later is fast becoming the norm we won't worry about it - pass the
    // expected "FunctionTraits" template arg and everything will be fine).
    /////////////////////////////////////////////////////////////////////////
    template <FUNCTION_TRAITS_C FunctionTraitsT>
    inline constexpr bool FunctionTraitsIsFunctor_v = FunctionTraitsT::IsFunctor;

    /////////////////////////////////////////////////////////////////////////
    // FunctionTraitsIsMemberFunction_v. Helper template yielding
    // "FunctionTraitsT::IsMemberFunction" (which stores "true" if
    // "FunctionTraitsT" represents a non-static member function including
    // functors), where "FunctionTraitsT" is a "FunctionTraits"
    // specialization. Note that there's usually no benefit to this
    // particular helper template however compared to calling
    // "FunctionTraitsT::IsMemberFunction" directly (there's no reduction in
    // verbosity), but it's provided for consistency anyway (to ensure a
    // helper template is available for all members of"FunctionTraits"). In
    // most cases you should rely on the "IsMemberFunction_v" helper
    // template instead however, which just defers to
    // "FunctionTraitsIsMemberFunction_v" and is easier to use.
    //
    // Note that you might want to invoke the following to determine if
    // "FunctionTraits" does in fact target a non-static member function
    // before invoking any other helper template specific to non-static member
    // functions only (if you don't know this ahead of time). The following
    // templates are affected:
    //
    //     1) FunctionTraitsMemberFunctionClass_t
    //     2) MemberFunctionTraitsClassName
    //     3) FunctionTraitsIsMemberFunctionConst_v
    //     4) FunctionTraitsIsMemberFunctionVolatile_v
    //     4) FunctionTraitsMemberFunctionRefQualifier_v
    //     6) FunctionTraitsMemberFunctionRefQualifierName_v
    //
    // Lastly, note that the "FunctionTraitsT" template arg must be a
    // "FunctionTraits" specialization or compilation will normally fail
    // (concept kicks in in C++20 or later so failure is guaranteed - in
    // C++17 or earlier however there is no guarantee compilation will fail
    // but usually will for other reasons - longer story but since C++20 or
    // later is fast becoming the norm we won't worry about it - pass the
    // expected "FunctionTraits" template arg and everything will be fine).
    /////////////////////////////////////////////////////////////////////////
    template <FUNCTION_TRAITS_C FunctionTraitsT>
    inline constexpr bool FunctionTraitsIsMemberFunction_v = FunctionTraitsT::IsMemberFunction;

    /////////////////////////////////////////////////////////////////////////
    // FunctionTraitsIsMemberFunctionConst_v. Helper template yielding
    // "FunctionTraitsT::IsConst" (storing true or false to indicate if the
    // non-static function represented by "FunctionTraitsT" is declared with
    // the "const" qualifier), where "FunctionTraitsT" is a "FunctionTraits"
    // specialization. Note that there's usually no benefit to this
    // particular helper template however compared to calling
    // "FunctionTraitsT::IsConst" directly (there's no reduction in
    // verbosity), but it's provided for consistency anyway (to ensure a
    // helper template is available for all members of "FunctionTraits"). In
    // most cases you should rely on the "IsMemberFunctionConst_v" helper
    // template instead however, which just defers to
    // "FunctionTraitsIsMemberFunctionConst_v" and is easier to use.
    //
    // Note that this member is always false for free functions and static
    // member functions (not applicable to them). You may therefore want to
    // call "FunctionTraitsIsMemberFunction_v" to determine if
    // "FunctionTraitsT" targets a non-static member function before
    // invoking the following (if required). Lastly, note that the
    // "FunctionTraitsT" template arg must be a "FunctionTraits"
    // specialization or compilation will normally fail (concept kicks in in
    // C++20 or later so failure is guaranteed - in C++17 or earlier however
    // there is no guarantee compilation will fail but usually will for
    // other reasons - longer story but since C++20 or later is fast
    // becoming the norm we won't worry about it - pass the expected
    // "FunctionTraits" template arg and everything will be fine).
    /////////////////////////////////////////////////////////////////////////
    template <FUNCTION_TRAITS_C FunctionTraitsT>
    inline constexpr bool FunctionTraitsIsMemberFunctionConst_v = FunctionTraitsT::IsConst;

    /////////////////////////////////////////////////////////////////////////
    // FunctionTraitsIsMemberFunctionVolatile_v. Helper template yielding
    // "FunctionTraitsT::IsVolatile" (storing true or false to indicate if
    // the non-static function represented by "FunctionTraitsT" is declared
    // with the "volatile" qualifier), where "FunctionTraitsT" is a
    // "FunctionTraits" specialization. Note that there's usually no benefit
    // to this particular helper template however compared to calling
    // "FunctionTraitsT::IsVolatile" directly (there's no reduction in
    // verbosity), but it's provided for consistency anyway (to ensure a
    // helper template is available for all members of "FunctionTraits"). In
    // most cases you should rely on the "IsMemberFunctionVolatile_v" helper
    // template instead however, which just defers to
    // "FunctionTraitsIsMemberFunctionVolatile_v" and is easier to use.
    //
    // Note that this member is always false for free functions and static
    // member functions (not applicable to them). You may therefore want to
    // call "FunctionTraitsIsMemberFunction_v" to determine if
    // "FunctionTraitsT" targets a non-static member function before
    // invoking the following (if required). Lastly, note that the
    // "FunctionTraitsT" template arg must be a "FunctionTraits"
    // specialization or compilation will normally fail (concept kicks in in
    // C++20 or later so failure is guaranteed - in C++17 or earlier however
    // there is no guarantee compilation will fail but usually will for
    // other reasons - longer story but since C++20 or later is fast
    // becoming the norm we won't worry about it - pass the expected
    // "FunctionTraits" template arg and everything will be fine).
    /////////////////////////////////////////////////////////////////////////
    template <FUNCTION_TRAITS_C FunctionTraitsT>
    inline constexpr bool FunctionTraitsIsMemberFunctionVolatile_v = FunctionTraitsT::IsVolatile;

    /////////////////////////////////////////////////////////////////////////
    // FunctionTraitsIsNoexcept_v. Helper template yielding
    // "FunctionTraitsT::IsNoexcept" (true or false to indicate if the
    // function represented by "FunctionTraitsT" is declared as "noexcept"),
    // where "FunctionTraitsT" is a "FunctionTraits" specialization. Note
    // that there's usually no benefit to this particular helper template
    // however compared to calling "FunctionTraitsT::IsNoexcept" directly
    // (there's no reduction in verbosity), but it's provided for
    // consistency anyway (to ensure a helper template is available for all
    // members of "FunctionTraits"). In most cases you should rely on the
    // "IsNoexcept_v" helper template instead however, which just defers to
    // "FunctionTraitsIsNoexcept_v" and is easier to use.
    //
    // Lastly, note that the "FunctionTraitsT" template arg must be a
    // "FunctionTraits" specialization or compilation will normally fail
    // (concept kicks in in C++20 or later so failure is guaranteed - in
    // C++17 or earlier however there is no guarantee compilation will fail
    // but usually will for other reasons - longer story but since C++20 or
    // later is fast becoming the norm we won't worry about it - pass the
    // expected "FunctionTraits" template arg and everything will be fine).
    /////////////////////////////////////////////////////////////////////////
    template <FUNCTION_TRAITS_C FunctionTraitsT>
    inline constexpr bool FunctionTraitsIsNoexcept_v = FunctionTraitsT::IsNoexcept;

    //////////////////////////////////////////////////////////////////////
    // FunctionTraitsIsVariadic_v. Helper template yielding
    // "FunctionTraitsT::IsVariadic" (storing true or false to indicate
    // if the function represented by "FunctionTraitsT" is variadic),
    // where "FunctionTraitsT" is a "FunctionTraits" specialization. Note
    // that there's usually no benefit to this particular helper template
    // however compared to calling "FunctionTraitsT::IsVariadic" directly
    // (there's no reduction in verbosity), but it's provided for
    // consistency anyway (to ensure a helper template is available for
    // all members of "FunctionTraits"). In most cases you should rely on
    // the "IsVariadic_v" helper template instead however, which just
    // defers to "FunctionTraitsIsVariadic_v" and is easier to use.
    //
    // Note that the "FunctionTraitsT" template arg must be a
    // "FunctionTraits" specialization or compilation will normally fail
    // (concept kicks in in C++20 or later so failure is guaranteed - in
    // C++17 or earlier however there is no guarantee compilation will
    // fail but usually will for other reasons - longer story but since
    // C++20 or later is fast becoming the norm we won't worry about it -
    // pass the expected "FunctionTraits" template arg and everything
    // will be fine).
    //////////////////////////////////////////////////////////////////////
    template <FUNCTION_TRAITS_C FunctionTraitsT>
    inline constexpr bool FunctionTraitsIsVariadic_v = FunctionTraitsT::IsVariadic;

    //////////////////////////////////////////////////////////////////////
    // FunctionTraitsIsVoidReturnType_v. Helper template returning "true"
    // if "FunctionTraitsT::ReturnType" is "void" (after ignoring any
    // cv-qualifiers), or "false" otherwise (where "FunctionTraitsT" is a
    // "FunctionTraits" specialization).
    //
    // Note that the "FunctionTraitsT" template arg must be a
    // "FunctionTraits" specialization or compilation will normally fail
    // (concept kicks in in C++20 or later so failure is guaranteed - in
    // C++17 or earlier however there is no guarantee compilation will
    // fail but usually will for other reasons - longer story but since
    // C++20 or later is fast becoming the norm we won't worry about it -
    // pass the expected "FunctionTraits" template arg and everything
    // will be fine).
    //////////////////////////////////////////////////////////////////////
    template <FUNCTION_TRAITS_C FunctionTraitsT>
    inline constexpr bool FunctionTraitsIsVoidReturnType_v = std::is_void_v<typename FunctionTraitsT::ReturnType>;

    /////////////////////////////////////////////////////////////////////////
    // FunctionTraitsMemberFunctionClass_t. Helper alias template yielding
    // "FunctionTraitsT::Class" (the class for a non-static member function
    // represented by "FunctionTraitsT"), where "FunctionTraitsT" is a
    // "FunctionTraits" specialization. Note that it's less verbose than
    // accessing the "Class" alias in "FunctionTraits" directly so you
    // should normally rely on the following instead (or preferably the
    // "MemberFunctionClass_t" helper alias instead, which just defers to
    // "FunctionTraitsMemberFunctionClass_t" and is even easier to use).
    // Note that this helper applies to non-static member functions only
    // (yielding the class type of non-static member functions). For free
    // functions including static member functions this type is always
    // "void" (not applicable to them noting that the class for static
    // member functions can't be retrieved in this release due to language
    // limitations). Note that you might want to call
    // "FunctionTraitsIsMemberFunction_v" to determine if "FunctionTraitsT"
    // targets a non-static member function before invoking the following
    // (if required). Lastly, the "FunctionTraitsT" template arg must be a
    // "FunctionTraits" specialization or compilation will normally fail
    // (concept kicks in in C++20 or later so failure is guaranteed - in
    // C++17 or earlier however there is no guarantee compilation will fail
    // but usually will for other reasons - longer story but since C++20 or
    // later is fast becoming the norm we won't worry about it - pass the
    // expected "FunctionTraits" template arg and everything will be fine).
    /////////////////////////////////////////////////////////////////////////
    template <FUNCTION_TRAITS_C FunctionTraitsT>
    using FunctionTraitsMemberFunctionClass_t = typename FunctionTraitsT::Class;

    //////////////////////////////////////////////////////////////////////////////////
    // MemberFunctionTraitsClassName. See "FunctionTraitsMemberFunctionClass_t" just
    // above. Simply converts that to a string suitable for display purposes and
    // returns it as a "tstring_view"
    //////////////////////////////////////////////////////////////////////////////////
    template <FUNCTION_TRAITS_C FunctionTraitsT>
    inline constexpr tstring_view FunctionTraitsMemberFunctionClassName_v = TypeName_v<FunctionTraitsMemberFunctionClass_t<FunctionTraitsT>>;

    /////////////////////////////////////////////////////////////////////////
    // FunctionTraitsMemberFunctionRefQualifier_v. Helper template yielding
    // "FunctionTraitsT::RefQualifier" (which stores the reference qualifier
    // "&" or "&&" if the non-static function represented by
    // "FunctionTraitsT" is declared with either), where "FunctionTraitsT"
    // is a "FunctionTraits" specialization. Note that there's usually no
    // benefit to this particular helper template however compared to
    // calling "FunctionTraitsT::RefQualifier" directly (there's no
    // reduction in verbosity), but it's provided for consistency anyway (to
    // ensure a helper template is available for all members of
    // "FunctionTraits"). In most cases you should rely on the
    // "IsMemberFunctionRefQualifier_v" helper template instead however,
    // which just defers to "IsMemberFunctionTraitsRefQualifier_v" and is
    // easier to use.
    //
    // Note that this member is always "RefQualifier::None" for free
    // functions and static member functions (not applicable to them). You
    // may therefore want to call "FunctionTraitsIsMemberFunction_v" to
    // determine if "FunctionTraitsT" targets a non-static member function
    // before invoking the following (if required). Lastly, note that the
    // "FunctionTraitsT" template arg must be a "FunctionTraits"
    // specialization or compilation will normally fail (concept kicks in in
    // C++20 or later so failure is guaranteed - in C++17 or earlier however
    // there is no guarantee compilation will fail but usually will for
    // other reasons - longer story but since C++20 or later is fast
    // becoming the norm we won't worry about it - pass the expected
    // "FunctionTraits" template arg and everything will be fine).
    /////////////////////////////////////////////////////////////////////////
    template <FUNCTION_TRAITS_C FunctionTraitsT>
    inline constexpr RefQualifier FunctionTraitsMemberFunctionRefQualifier_v = FunctionTraitsT::RefQualifier;

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    // FunctionTraitsMemberFunctionRefQualifierName_v. See "FunctionTraitsMemberFunctionRefQualifier_v"
    // just above. Simply converts that to a string suitable for display purposes and returns it as a
    // "tstring_view". Pass "true" (the default) for the "UseAmpersands" template arg if you wish to
    // return this as "&" or "&&" or "false" to return it as "LValue" or "RValue" instead (but in either
    // case however "None" is always returned if "F" has no reference-qualifier which is usually the
    // case for most functions in practice.
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    template <FUNCTION_TRAITS_C FunctionTraitsT, bool UseAmpersands = true>
    inline constexpr tstring_view FunctionTraitsMemberFunctionRefQualifierName_v = RefQualifierToString(FunctionTraitsMemberFunctionRefQualifier_v<FunctionTraitsT>, // refQualifier
                                                                                                        UseAmpersands);

    /////////////////////////////////////////////////////////////////////////
    // FunctionTraitsReturnType_t. Helper alias template yielding
    // "FunctionTraitsT::ReturnType" (the return type of the function
    // represented by "FunctionTraitsT"), where "FunctionTraitsT" is a
    // "FunctionTraits" specialization. Note that it's less verbose than
    // accessing the "ReturnType" alias in "FunctionTraits" directly so you
    // should normally rely on the following instead (or preferably the
    // "ReturnType_t" helper alias instead, which just defers to
    // "FunctionTraitsReturnType_t" and is even easier to use). Note that
    // the "FunctionTraitsT" template arg must be a "FunctionTraits"
    // specialization or compilation will normally fail (concept kicks in in
    // C++20 or later so failure is guaranteed - in C++17 or earlier however
    // there is no guarantee compilation will fail but usually will for
    // other reasons - longer story but since C++20 or later is fast
    // becoming the norm we won't worry about it - pass the expected
    // "FunctionTraits" template arg and everything will be fine).
    //
    // E.g.,
    //
    //       int MyFunction(const std::string &str, int val)
    //       {
    //       }
    //
    //       // "FunctionTraits" for above function
    //       using MyFunctionTraits = FunctionTraits<decltype(MyFunction)>;
    //       // using MyFunctionTraits = FunctionTraits<int (const std::string &, int)>; // This will also work (same as above but passing function's type on-the-fly)
    //       // using MyFunctionTraits = FunctionTraits<int (*)(const std::string &, int)>; // ... and this too (but now using a pointer to the function so the type
    //                                                                                      // will now reflect that - references to the function will also work, and
    //                                                                                      // references to pointers to the function as well)
    //
    //       // Return type for "MyFunction" above (int)
    //       using MyFunctionReturnType = FunctionTraitsReturnType_t<MyFunctionTraits>;
    /////////////////////////////////////////////////////////////////////////
    template <FUNCTION_TRAITS_C FunctionTraitsT>
    using FunctionTraitsReturnType_t = typename FunctionTraitsT::ReturnType;

    ///////////////////////////////////////////////////////////////////////
    // FunctionTraitsReturnTypeName. See "FunctionTraitsReturnType_t" just
    // above. Simply converts that to a string suitable for display
    // purposes and returns it as a "tstring_view" (so a return type like,
    // say, "int" will literally be returned as the string "int", quotes
    // not included)
    ///////////////////////////////////////////////////////////////////////
    template <FUNCTION_TRAITS_C FunctionTraitsT>
    inline constexpr tstring_view FunctionTraitsReturnTypeName_v = TypeName_v<FunctionTraitsReturnType_t<FunctionTraitsT>>;

    ////////////////////////////////////////////////////////////////////////////
    // FunctionTraitsIsEmptyArgList_v. Helper template returning "true" if the
    // function represented by "FunctionTraitsT" has an empty arg list (it has
    // no args whatsoever including variadic args), or "false" otherwise (where
    // "FunctionTraitsT" is a "FunctionTraits" specialization). If true then
    // note that the "FunctionTraitsArgCount_v" helper is guaranteed to return
    // zero (0), and the "FunctionTraitsIsVariadic_v" helper is guaranteed to
    // return false. Please note that in most cases you should rely on the
    // "IsEmptyArgList_v" helper template instead however, which just defers to
    // "FunctionTraitsIsEmptyArgList_v" and is easier to use.
    //
    // IMPORTANT:
    // ----------
    // Note that you should rely on this helper to determine if a function's
    // argument list is completely empty opposed to checking the
    // "FunctionTraitsArgCount_v" helper for zero (0), since the latter returns
    // zero only if the function represented by "FunctionTraitsT" has no
    // non-variadic args. If it has variadic args but no others, i.e., its
    // argument list is "(...)", then the argument list isn't completely empty
    // even though "FunctionTraitsArgCount_v" returns zero (since it still has
    // variadic args). Caution advised.
    //
    // Lastly, note that the "FunctionTraitsT" template arg must be a
    // "FunctionTraits" specialization or compilation will normally fail
    // (concept kicks in in C++20 or later so failure is guaranteed - in C++17
    // or earlier however there is no guarantee compilation will fail but
    // usually will for other reasons - longer story but since C++20 or later
    // is fast becoming the norm we won't worry about it - pass the expected
    // "FunctionTraits" template arg and everything will be fine).
    ////////////////////////////////////////////////////////////////////////////
    template <FUNCTION_TRAITS_C FunctionTraitsT>
    inline constexpr bool FunctionTraitsIsEmptyArgList_v = FunctionTraitsArgCount_v<FunctionTraitsT> == 0 && // Zero (non-variadic) args
                                                           !FunctionTraitsIsVariadic_v<FunctionTraitsT>; // Not variadic

    /////////////////////////////////////////////////////
    // See this constant for details. Following helper
    // templates for function write traits are declared
    // only if function write traits are supported.
    // They're preprocessed out otherwise (so function
    // write traits simply won't be available).
    /////////////////////////////////////////////////////
    #if defined(FUNCTION_WRITE_TRAITS_SUPPORTED)
        /////////////////////////////////////////////////////////////////////////////
        // FunctionTraitsAddVariadicArgs_t. Helper alias for
        // "FunctionTraitsT::AddVariadicArgs" (where "FunctionTraitsT" is a
        // "FunctionTraits" specialization). Returns a type alias for the function
        // "F" represented by "FunctionTraitsT" after adding "..." to the end of its
        // argument list if not already present. Note that the calling convention is
        // also changed to the "Cdecl" calling convention for the given compiler.
        // This is the only supported calling convention for variadic functions in
        // this release but most platforms require this calling convention for
        // variadic functions. It ensures that the calling function (opposed to the
        // called function) pops the stack of arguments after the function is
        // called, which is required by variadic functions. Other calling
        // conventions that also do this are possible though none are currently
        // supported in this release (since none of the currently supported
        // compilers support this - such calling conventions are rare in practice).
        /////////////////////////////////////////////////////////////////////////////
        template <FUNCTION_TRAITS_C FunctionTraitsT>
        using FunctionTraitsAddVariadicArgs_t = typename FunctionTraitsT::AddVariadicArgs;

        /////////////////////////////////////////////////////////////////////////////
        // FunctionTraitsRemoveVariadicArgs_t. Helper alias for
        // "FunctionTraitsT::RemoveVariadicArgs" (where "FunctionTraitsT" is a
        // "FunctionTraits" specialization). If the function "F" represented by
        // "FunctionTraitsT" is a variadic function (its last arg is "..."), yields
        // a type alias for "F" after removing the "..." from the argument list. All
        // non-variadic arguments (if any) remain intact (only the "..." is
        // removed).types alias "F" is a variadic function (its last arg is "..."),
        // yields a type alias for "FunctionTraitsT::RemoveVariadicArgs" after
        // removing the "..." from the argument list (where "FunctionTraitsT" is a
        // "FunctionTraits" specialization). All non-variadic arguments (if any)
        // remain intact (only the "..." is removed).
        /////////////////////////////////////////////////////////////////////////////
        template <FUNCTION_TRAITS_C FunctionTraitsT>
        using FunctionTraitsRemoveVariadicArgs_t = typename FunctionTraitsT::RemoveVariadicArgs;

        /////////////////////////////////////////////////////////////////////////////
        // FunctionTraitsMemberFunctionAddConst_t. Helper alias for
        // "FunctionTraitsT::AddConst_t" (where "FunctionTraitsT" is a
        // "FunctionTraits" specialization). If the function "F" represented by
        // "FunctionTraitsT" represents a non-static member function, yields a type
        // alias for "FunctionTraitsT::AddConst"" after adding the "const"
        // cv-qualifier to the function if not already present (where
        // "FunctionTraitsT" is a "FunctionTraits" specialization). If "F" is a free
        // function including static member functions, yields "F" itself
        // (effectively does nothing since "const" applies to non-static member
        // functions only).
        /////////////////////////////////////////////////////////////////////////////
        template <FUNCTION_TRAITS_C FunctionTraitsT>
        using FunctionTraitsMemberFunctionAddConst_t = typename FunctionTraitsT::AddConst;

        /////////////////////////////////////////////////////////////////////////////
        // FunctionTraitsMemberFunctionRemoveConst_t. Helper alias for
        // "FunctionTraitsT::RemoveConst" (where "FunctionTraitsT" is a
        // "FunctionTraits" specialization). If the function "F" represented by
        // "FunctionTraitsT" is a non-static member function, yields a type alias
        // for "F" after removing the "const" cv-qualifier from the function if
        // present. If "F" is a free function including static member functions,
        // yields "F" itself (effectively does nothing since "const" applies to
        // non-static member functions only so will never be present otherwise).
        /////////////////////////////////////////////////////////////////////////////
        template <FUNCTION_TRAITS_C FunctionTraitsT>
        using FunctionTraitsMemberFunctionRemoveConst_t = typename FunctionTraitsT::RemoveConst;

        /////////////////////////////////////////////////////////////////////////////
        // FunctionTraitsMemberFunctionAddVolatile_t. Helper alias for
        // "FunctionTraitsT::AddVolatile" (where "FunctionTraitsT" is a
        // "FunctionTraits" specialization). If the function "F" represented by
        // "FunctionTraitsT" is a non-static member function, yields a type alias
        // for "F" after adding the "volatile" cv-qualifier to the function if not
        // already present. If "F" is a free function including static member
        // functions, yields "F" itself (effectively does nothing since "volatile"
        // applies to non-static member functions only).
        /////////////////////////////////////////////////////////////////////////////
        template <FUNCTION_TRAITS_C FunctionTraitsT>
        using FunctionTraitsMemberFunctionAddVolatile_t = typename FunctionTraitsT::AddVolatile;

        /////////////////////////////////////////////////////////////////////////////
        // FunctionTraitsMemberFunctionRemoveVolatile_t. Helper alias for
        // "FunctionTraitsT::RemoveVolatile" (where "FunctionTraitsT" is a
        // "FunctionTraits" specialization). If the function "F" represented by
        // "FunctionTraitsT" is a non-static member function, yields a type alias
        // for "F" after removing the "volatile" cv-qualifier from the function if
        // present. If "F" is a free function including static member functions,
        // yields "F" itself (effectively does nothing since "volatile" applies to
        // non-static member functions only so will never be present otherwise).
        /////////////////////////////////////////////////////////////////////////////
        template <FUNCTION_TRAITS_C FunctionTraitsT>
        using FunctionTraitsMemberFunctionRemoveVolatile_t = typename FunctionTraitsT::RemoveVolatile;

        /////////////////////////////////////////////////////////////////////////////
        // FunctionTraitsMemberFunctionAddCV_t. Helper alias for
        // "FunctionTraitsT::AddCV" (where "FunctionTraitsT" is a "FunctionTraits"
        // specialization). If the function "F" represented by "FunctionTraitsT" is
        // a non-static member function, yields a type alias for "F" after adding
        // both the "const" AND "volatile" cv-qualifiers to the function if not
        // already present. If "F" is a free function including static member
        // functions, yields "F" itself (effectively does nothing since "const" and
        // "volatile" apply to non-static member functions only).
        /////////////////////////////////////////////////////////////////////////////
        template <FUNCTION_TRAITS_C FunctionTraitsT>
        using FunctionTraitsMemberFunctionAddCV_t = typename FunctionTraitsT::AddCV;

        /////////////////////////////////////////////////////////////////////////////
        // FunctionTraitsMemberFunctionRemoveCV_t. Helper alias for
        // "FunctionTraitsT::RemoveCV" (where "FunctionTraitsT" is a
        // "FunctionTraits" specialization). If the function "F" represented by
        // "FunctionTraitsT" is a non-static member function, yields a type alias
        // for "F" after removing both the "const" AND "volatile" cv-qualifiers from
        // the function if present. If "F" is a free function including static
        // member functions, yields "F" itself (effectively does nothing since
        // "const" and "volatile" apply to non-static member functions only so will
        // never be present otherwise).
        /////////////////////////////////////////////////////////////////////////////
        template <FUNCTION_TRAITS_C FunctionTraitsT>
        using FunctionTraitsMemberFunctionRemoveCV_t = typename FunctionTraitsT::RemoveCV;

        /////////////////////////////////////////////////////////////////////////////
        // FunctionTraitsMemberFunctionAddLValueReference_t. Helper alias for
        // "FunctionTraitsT::AddLValueReference" (where "FunctionTraitsT" is a
        // "FunctionTraits" specialization). If the function "F" represented by
        // "FunctionTraitsT" is a non-static member function, yields a type alias
        // for "F" after adding the "&" reference-qualifier to the function if not
        // already present (replacing the "&&" reference-qualifier if present). If
        // "F" is a free function including static member functions, yields "F"
        // itself (effectively does nothing since reference-qualifiers apply to
        // non-static member functions only).
        /////////////////////////////////////////////////////////////////////////////
        template <FUNCTION_TRAITS_C FunctionTraitsT>
        using FunctionTraitsMemberFunctionAddLValueReference_t = typename FunctionTraitsT::AddLValueReference;

        /////////////////////////////////////////////////////////////////////////////
        // FunctionTraitsMemberFunctionAddRValueReference_t. Helper alias for
        // "FunctionTraitsT::AddRValueReference" (where "FunctionTraitsT" is a
        // "FunctionTraits" specialization). If the function "F" represented by
        // "FunctionTraitsT" is a non-static member function, yields a type alias
        // for "F" after adding the "&&" reference-qualifier to the function
        // (replacing the "&" reference-qualifier if present). If "F" is a free
        // function including static member functions, yields "F" itself
        // (effectively does nothing since reference-qualifiers apply to non-static
        // member functions only).
        /////////////////////////////////////////////////////////////////////////////
        template <FUNCTION_TRAITS_C FunctionTraitsT>
        using FunctionTraitsMemberFunctionAddRValueReference_t = typename FunctionTraitsT::AddRValueReference;

        /////////////////////////////////////////////////////////////////////////////
        // FunctionTraitsMemberFunctionRemoveReference_t. Helper alias for
        // "FunctionTraitsT::RemoveReference" (where "FunctionTraitsT" is a
        // "FunctionTraits" specialization). If the function "F" represented by
        // "FunctionTraitsT" is a non-static member function, yields a type alias
        // for "F" after removing the "&" or "&&" reference-qualifier from the
        // function if present. If "F" is a free function including static member
        // functions, yields "F" itself (effectively does nothing since
        // reference-qualifiers to non-static member functions only so will never be
        // present otherwise).
        /////////////////////////////////////////////////////////////////////////////
        template <FUNCTION_TRAITS_C FunctionTraitsT>
        using FunctionTraitsMemberFunctionRemoveReference_t = typename FunctionTraitsT::RemoveReference;

        /////////////////////////////////////////////////////////////////////////////
        // FunctionTraitsAddNoexcept_t. Helper alias for
        // "FunctionTraitsT::AddNoexcept" (where "FunctionTraitsT" is a
        // "FunctionTraits" specialization). Returns a type alias for the function
        // "F" represented by "FunctionTraitsT" after adding "noexcept" to "F" if
        // not already present
        /////////////////////////////////////////////////////////////////////////////
        template <FUNCTION_TRAITS_C FunctionTraitsT>
        using FunctionTraitsAddNoexcept_t = typename FunctionTraitsT::AddNoexcept;

        /////////////////////////////////////////////////////////////////////////////
        // FunctionTraitsRemoveNoexcept_t. Helper alias for
        // "FunctionTraitsT::RemoveNoexcept" (where "FunctionTraitsT" is a
        // "FunctionTraits" specialization). Returns a type alias for the function
        // "F" represented by "FunctionTraitsT" after removing "noexcept" from "F"
        // if present
        /////////////////////////////////////////////////////////////////////////////
        template <FUNCTION_TRAITS_C FunctionTraitsT>
        using FunctionTraitsRemoveNoexcept_t = typename FunctionTraitsT::RemoveNoexcept;

        /////////////////////////////////////////////////////////////////////////////
        // FunctionTraitsReplaceCallingConvention_t. Helper alias for
        // "FunctionTraitsT::ReplaceCallingConvention" (where "FunctionTraitsT" is a
        // "FunctionTraits" specialization). Returns a type alias for the function
        // "F" represented by "FunctionTraitsT" after replacing its calling
        // convention with the platform-specific calling convention corresponding to
        // "NewCallingConventionT" (a "CallingConvention" enumerator declared in
        // "TypeTraits.h"). For instance, if you pass  "CallingConvention::FastCall"
        // then the calling convention on "F" is replaced with
        // "__attribute__((cdecl))" on GCC, Clang and others, but "__cdecl" on
        // Microsoft platforms. Note however that the calling convention for
        // variadic functions (those whose last arg is "...") can't be changed in
        // this release. Variadic functions require that the calling function pop
        // the stack to clean up passed arguments and only the "Cdecl" calling
        // convention supports that in this release (on all supported compilers at
        // this writing). Attempts to change it are therefore ignored. Note that you
        // also can't change the calling convention of free functions to
        // "CallingConvention::Thiscall" (including for static member functions
        // which are considered "free" functions). Attempts to do so are ignored
        // since the latter calling convention applies to non-static member
        // functions only. Lastly, please note that compilers will sometimes change
        // the calling convention declared on your functions to the "Cdecl" calling
        // convention depending on the compiler options in effect at the time (in
        // particular when compiling for 64 bits opposed to 32 bits, though the
        // "Vectorcall" calling convention is supported on 64 bits). Therefore, if
        // you specify a calling convention that the compiler changes to "Cdecl"
        // based on the compiler options currently in effect, then
        // "ReplaceCallingConvention_t" will also ignore your calling convention and
        // apply "Cdecl" instead (since that's what the compiler actually uses).
        /////////////////////////////////////////////////////////////////////////////
        template <FUNCTION_TRAITS_C FunctionTraitsT, CallingConvention NewCallingConventionT>
        using FunctionTraitsReplaceCallingConvention_t = typename FunctionTraitsT::template ReplaceCallingConvention<NewCallingConventionT>;

        /////////////////////////////////////////////////////////////////////////////
        // FunctionTraitsMemberFunctionReplaceClass_t. Helper alias for
        // "FunctionTraits::ReplaceClass" (where "FunctionTraitsT" is a
        // "FunctionTraits" specialization). If the function "F" represented by
        // "FunctionTraitsT" is a non-static member function, yields a type alias
        // for "F" after replacing the class this function belongs to with
        // "NewClassT". If "F" is a free function including static member functions,
        // yields "F" itself (effectively does nothing since a "class" applies to
        // non-static member functions only so will never be present otherwise -
        // note that due to limitations in C++ itself, replacing the class for
        // static member functions is not supported).
        /////////////////////////////////////////////////////////////////////////////
        template <FUNCTION_TRAITS_C FunctionTraitsT, IS_CLASS_C NewClassT>
        using FunctionTraitsMemberFunctionReplaceClass_t = typename FunctionTraitsT::template ReplaceClass<NewClassT>;

        /////////////////////////////////////////////////////////////////////////////
        // FunctionTraitsReplaceReturnType_t. Helper alias for
        // "FunctionTraitsT::ReplaceReturnType" (where "FunctionTraitsT" is a
        // "FunctionTraits" specialization). Returns a type alias for the function
        // "F" represented by "FunctionTraitsT" after replacing the return type for
        // "F" with "NewReturnTypeT"
        /////////////////////////////////////////////////////////////////////////////
        template <FUNCTION_TRAITS_C FunctionTraitsT, typename NewReturnTypeT>
        using FunctionTraitsReplaceReturnType_t = typename FunctionTraitsT::template ReplaceReturnType<NewReturnTypeT>;

        /////////////////////////////////////////////////////////////////////////////
        // FunctionTraitsReplaceArgs_t. Helper alias for
        // "FunctionTraitsT::ReplaceArgs" (where "FunctionTraitsT" is a
        // "FunctionTraits" specialization). Returns a type alias for the function
        // "F" represented by "FunctionTraitsT" after replacing all its existing
        // non-variadic arguments with the args given by "NewArgsT" (a parameter
        // pack of the types that become the new argument list). If none are passed
        // then an empty argument list results instead, though if variadic args are
        // present in "F" then they still remain intact (the "..." remains - read
        // on). The resulting alias is identical to "F" itself except that the
        // non-variadic arguments in "F" are completely replaced with "NewArgsT".
        // Note that if "F" is a variadic function (its last parameter is "..."),
        // then it remains a variadic function after the call (the "..." remains in
        // place). If you wish to explicitly add or remove the "..." as well then
        // pass the resulting type to "AddVariadicArgs_t" or "RemoveVariadicArgs_t"
        // respectively (either before or after the call to "ReplaceArgs_t"). Note
        // that if you wish to remove specific arguments instead of all of them,
        // then call "ReplaceNthArg_t" instead. Lastly, you can alternatively use
        // "ReplaceArgsTuple_t" instead of "ReplaceArgs_t" if you have a
        // "std::tuple" of types you wish to use for the argument list instead of a
        // parameter pack. "ReplaceArgsTuple_t" is identical to ""ReplaceArgs_t""
        // otherwise (it ultimately defers to it).
        /////////////////////////////////////////////////////////////////////////////
        template <FUNCTION_TRAITS_C FunctionTraitsT, typename... NewArgsT>
        using FunctionTraitsReplaceArgs_t = typename FunctionTraitsT::template ReplaceArgs<NewArgsT...>;

        /////////////////////////////////////////////////////////////////////////////
        // FunctionTraitsReplaceArgsTuple_t. Helper alias for
        // "FunctionTraitsT::ReplaceArgsTuple" (where "FunctionTraitsT" is a
        // "FunctionTraits" specialization). Identical to
        // "FunctionTraitsReplaceArgs_t" just above except the argument list is
        // passed as a "std::tuple" instead of a parameter pack (via the 2nd
        // template arg). The types in the "std::tuple" are therefore used for the
        // resulting argument list. "FunctionTraitsReplaceArgsTuple_t" is otherwise
        // identical to "FunctionTraitsReplaceArgs_t".
        /////////////////////////////////////////////////////////////////////////////
        template <FUNCTION_TRAITS_C FunctionTraitsT, typename NewArgsTupleT>
        using FunctionTraitsReplaceArgsTuple_t = typename FunctionTraitsT::template ReplaceArgsTuple<NewArgsTupleT>;

        /////////////////////////////////////////////////////////////////////////////
        // FunctionTraitsReplaceNthArg_t. Helper alias for
        // "FunctionTraits::ReplaceNthArg" (where "FunctionTraitsT" is a
        // "FunctionTraits" specialization). Returns a type alias for the function
        // "F" represented by "FunctionTraitsT" after replacing its (zero-based)
        // "Nth" argument with "NewArgT". Pass "N" via the 2nd template arg (e.g.,
        // the zero-based index of the arg you're targeting) and the type you wish
        // to replace it with via the 3rd template arg ("NewArgT"). The resulting
        // alias is therefore identical to "F" except its "Nth" argument is replaced
        // by "NewArgT" (so passing, say, zero-based "2" for "N" and "int" for
        // "NewArgT" would replace the 3rd function argument with an "int"). Note
        // that "N" must be less than the number of arguments in the function or a
        // "static_assert" will occur (new argument types can't be added using this
        // trait, only existing argument types replaced). If you need to replace
        // multiple arguments then recursively call "ReplaceNthArg_t" again, passing
        // the result as the "F" template arg of "ReplaceNthArg_t" as many times as
        // you need to (each time specifying a new "N" and "NewArgT"). If you wish
        // to replace all arguments at once then call "ReplaceArgs_t" or
        // "ReplaceArgsTuple_t" instead. Lastly, note that if "F" has variadic
        // arguments (it ends with "..."), then these remain intact. If you need to
        // remove them then call "RemoveVariadicArgs_t" before or after the call to
        // "ReplaceNthArg_t".
        /////////////////////////////////////////////////////////////////////////////
        template <FUNCTION_TRAITS_C FunctionTraitsT, std::size_t N, typename NewArgT>
        using FunctionTraitsReplaceNthArg_t = typename FunctionTraitsT::template ReplaceNthArg<N, NewArgT>;
    #endif // #if defined(FUNCTION_WRITE_TRAITS_SUPPORTED)

    /////////////////////////////////////////////////////////////////////////
    // ArgCount_v. Helper template for "FunctionTraits::ArgCount" which
    // yields the number of args in function "F" not including variadic args
    // if any (i.e., where the last arg is "..."). Less verbose however than
    // creating a "FunctionTraits" directly from "F" and accessing its
    // "ArgCount" member. The following provides a convenient wrapper.
    //
    // IMPORTANT:
    // ---------
    // Please note that if you wish to check if a function's argument list
    // is completely empty, then inspecting this helper template for zero
    // (0) is not sufficient, since it may return zero but still contain
    // variadic args. To check for a completely empty argument list, call
    // "IsEmptyArgList_v" instead. See this for further details.
    /////////////////////////////////////////////////////////////////////////
    template <TRAITS_FUNCTION_C F>
    inline constexpr std::size_t ArgCount_v = FunctionTraitsArgCount_v<FunctionTraits<F>>; // Defers to the "FunctionTraits" helper further above

    ////////////////////////////////////////////////////////////////////////////
    // ArgType_t. Helper alias for "FunctionTraits::Args" but less verbose than
    // creating a "FunctionTraits" directly from "F" and accessing its "Args"
    // alias (allowing you to retrieve the type of any non-variadic arg in the
    // function). The following provides a convenient wrapper. Yields the type
    // of the (zero-based) "Ith" arg of function "F" where "I" must be less
    // than the number args in function "F" or a compiler error occurs. If "F"
    // has no args or variadic args only (if it's variadic), then a compiler
    // error will always occur so this function should never be called in that
    // case (since no args exist). You can check "ArgCount_v" for this and if
    // zero then it's not legal to use "ArgType_t". Note that variadic args are
    // never included so "I" can't be used to target them (they're effectively
    // ignored as far as "I" is concerned).    
    //
    // E.g.,
    //
    //       void MyFunction(const std::string &str, int val)
    //       {
    //       }
    //
    //       ////////////////////////////////////////////////////////
    //       // Type of second arg of "MyFunction" above (an "int").
    //       // Passing 1 here since index is zero-based.
    //       ////////////////////////////////////////////////////////
    //       using Arg2Type = ArgType_t<decltype(MyFunc), 1>;
    //       // using Arg2Type = ArgType_t<void (const std::string &, int)>; // This will also work (same as above but passing function's type on-the-fly)
    //       // using Arg2Type = ArgType_t<void (*)(const std::string &, int)>; // ... and this too (but now using a pointer to the function - references
    //                                                                          // to the function will also work, and references to pointers to the
    //                                                                          // function as well)
    ////////////////////////////////////////////////////////////////////////////
    template <TRAITS_FUNCTION_C F, std::size_t I /* Zero-based */>
    using ArgType_t = FunctionTraitsArgType_t<FunctionTraits<F>, I>; // Defers to the "FunctionTraits" helper further above

    //////////////////////////////////////////////////////////////////////
    // ArgTypeName(). See "ArgType_t()" just above. Simply converts that
    // to a string suitable for display purposes and returns it as a
    // "tstring_view"
    //////////////////////////////////////////////////////////////////////
    template <TRAITS_FUNCTION_C F, std::size_t I /* Zero-based */>
    inline constexpr tstring_view ArgTypeName_v = FunctionTraitsArgTypeName_v<FunctionTraits<F>, I>; // Defers to the "FunctionTraits" helper further above

    /////////////////////////////////////////////////////////////////////////
    // ArgTypes_t. Helper alias for "FunctionTraits::ArgTypes" which yields
    // a "std::tuple" storing all arg types for function "F" but less
    // verbose than creating a "FunctionTraits" directly from "F" and
    // accessing its "ArgTypes" alias. The following provides a convenient
    // wrapper. Note that using this alias is rarely ever required in
    // practice however since arg types can be individually accessed using
    // the "ArgType_t" helper alias instead (see this for details). Normally
    // "ArgTypes_t" only needs to be accessed if you need to iterate all the
    // arg types for some reason but if so then you can just invoke helper
    // function (template) "ForEachArg()" which exists for this purpose.
    // See this for details.
    //
    // E.g.,
    //
    //     void MyFunction(const std::string &str, int val)
    //     {
    //     }
    //
    //     ////////////////////////////////////////////////////////////////
    //     // "ArgTypes" for "MyFunction" above. A std::tuple" containing
    //     // all args in the function so its size will be 2 in this case
    //     // (first type a "const std::string &" and 2nd type an "int",
    //     // as seen in the function above). Note that you can use
    //     // "std::is_same" to compare this with the "ArgTypes" in another
    //     // "FunctionTraits" to determine if they contain the exact same
    //     // argument types. You can also iterate these types using helper
    //     // "ForEachArg()". See this for details.
    //     ////////////////////////////////////////////////////////////////
    //     using MyFunctionArgTypes = ArgTypes_t<decltype(MyFunction)>;
    //     // using MyFunctionArgTypes = ArgTypes_t<void (const std::string &, int)>; // This will also work (same as above but passing function's type on-the-fly)
    //     // using MyFunctionArgTypes = ArgTypes_t<void (*)(const std::string &, int)>; // ... and this too (but now using a pointer to the function - references
    //                                                                                   // to the function will also work, and references to pointers to the
    //                                                                                   // function as well)
    /////////////////////////////////////////////////////////////////////////
    template <TRAITS_FUNCTION_C F>
    using ArgTypes_t = FunctionTraitsArgTypes_t<FunctionTraits<F>>; // Defers to the "FunctionTraits" helper further above

    ///////////////////////////////////////////////////////////////////////////////
    // CallingConvention_v. Helper template for "FunctionTraits::CallingConvention"
    // which yields the calling convention of function "F" but less verbose than
    // creating a "FunctionTraits" directly from "F" and accessing its
    // "CallingConvention" member. The following provides a convenient wrapper.
    // Note however that compilers will sometimes change the explicitly declared
    // calling convention of a function to "cdecl" (whatever the syntax is for
    // this on the given platform) depending on the compiler options in effect,
    // in particular when compiling for 64 bits. In these cases the following
    // will normally return "CallingConvention::Cdecl" regardless of what the
    // calling convention is explicitly set to on the function (since "cdecl" is
    // the actual calling convention applied by the compiler). Some exceptions
    // exist however, such as the "vectorcall" calling convention, which is
    // supported even in 64 bit builds normally.
    ///////////////////////////////////////////////////////////////////////////////
    template <TRAITS_FUNCTION_C F>
    inline constexpr CallingConvention CallingConvention_v = FunctionTraitsCallingConvention_v<FunctionTraits<F>>; // Defers to the "FunctionTraits" helper further above

    ///////////////////////////////////////////////////////////////////
    // CallingConventionName_v. See "CallingConvention_v" just above.
    // Simply converts that to a string suitable for display purposes
    // and returns it as a "tstring_view"
    ///////////////////////////////////////////////////////////////////
    template <TRAITS_FUNCTION_C F>
    inline constexpr tstring_view CallingConventionName_v = FunctionTraitsCallingConventionName_v<FunctionTraits<F>>; // Defers to the "FunctionTraits" helper further above

    //////////////////////////////////////////////////////////////////////
    // FunctionType_t. Helper alias for "FunctionTraits::Type" which
    // yields the type of function "F" but less verbose than creating a
    // "FunctionTraits" directly and accessing its "Type" alias. The
    // following provides a convenient wrapper.
    //
    // E.g.,
    //
    //       void MyFunction(const std::string &str, int val)
    //       {
    //       }
    //
    //       //////////////////////////////////////////////////////////////
    //       // Type of "MyFunction" above. Yields the following (though
    //       // it might include the calling convention as well depending
    //       // on the platform):
    //       //
    //       //     void (const std::string &, int)
    //       //////////////////////////////////////////////////////////////
    //       using MyFunctionType = FunctionType_t<decltype(MyFunction)>;
    //       // using MyFunctionType = FunctionType_t<void (const std::string &, int)>; // This will also work (same as above but passing function's type on-the-fly)
    //       // using MyFunctionType = FunctionType_t<void (*)(const std::string &, int)>; // ... and this too (but now using a pointer to the function so the type
    //                                                                                     // will now reflect that - references to the function will also work, and
    //                                                                                     // references to pointers to the function as well)
    ////////////////////////////////////////////////////////////////////
    template <TRAITS_FUNCTION_C F>
    using FunctionType_t = FunctionTraitsFunctionType_t<FunctionTraits<F>>; // Defers to the "FunctionTraits" helper further above

    /////////////////////////////////////////////////////////////////////////
    // FunctionTypeName_v. See "FunctionType_t" just above. Simply converts
    // that to a string suitable for display purposes and returns it as a
    // "tstring_view"
    /////////////////////////////////////////////////////////////////////////
    template <TRAITS_FUNCTION_C F>
    inline constexpr tstring_view FunctionTypeName_v = FunctionTraitsTypeName_v<FunctionTraits<F>>; // Defers to the "FunctionTraits" helper further above

    ///////////////////////////////////////////////////////////////////////////
    // IsArgTypeSame_v. Helper template yielding "true" if the (zero-based)
    // "Ith" arg of function "F" is the same as the given type "T" or false
    // otherwise. The function is just a thin wrapper around "std::is_same_v",
    // where the latter template is passed the type of the "Ith" arg in
    // function "F" as its 1st template arg, and "T" as its 2nd template arg.
    // IOW, it simply compares the type of the "Ith" arg in function "F" with
    // type "T". This is a common requirement for many users so this template
    // provides a convenient wrapper.
    // 
    //    using F = int SomeFunc(int, float, double);
    // 
    //    //////////////////////////////////////////////////////////////////
    //    // Identical to the following but more convenient (and less
    //    // verbose):
    //    //
    //    // using Arg2_t = ArgType_t<F, 1>;
    //    // constexpr bool Is2ndArgAFloat = std::is_same_v<Arg2_t, float>;
    //    //////////////////////////////////////////////////////////////////
    //    constexpr bool Is2ndArgAFloat = IsArgTypeSame_v<F, 1, float>; // true
    //    
    ///////////////////////////////////////////////////////////////////////////
    template <TRAITS_FUNCTION_C F, std::size_t I /* Zero-based */, typename T>
    inline constexpr bool IsArgTypeSame_v = FunctionTraitsIsArgTypeSame_v<FunctionTraits<F>, I, T>;

    ///////////////////////////////////////////////////////////////////////////
    // IsEmptyArgList_v. Helper template yielding "true" if the function
    // represented by "F" has an empty arg list (it has no args whatsoever
    // including variadic args), or "false" otherwise. If true then note that
    // the "ArgCount_v" helper is guaranteed to return zero (0), and the
    // "IsVariadic_v" helper is guaranteed to return false. Note that this
    // helper template is less verbose than creating a "FunctionTraits"
    // directly from "F" and calling its own "FunctionTraitsIsEmptyArgList_v"
    // helper. The follow wraps the latter call for you.
    //
    // IMPORTANT:
    // ----------
    // Note that you should rely on this helper to determine if a function's
    // argument list is completely empty opposed to checking the "ArgCount_v"
    // helper for zero (0), since the latter returns zero only if "F" has no
    // non-variadic args. If it has variadic args but no others, i.e., its
    // argument list is "(...)", then the argument list isn't completely empty
    // even though "ArgCount_v" returns zero (since it still has variadic
    // args). Caution advised.
    ///////////////////////////////////////////////////////////////////////////
    template <TRAITS_FUNCTION_C F>
    inline constexpr bool IsEmptyArgList_v = FunctionTraitsIsEmptyArgList_v<FunctionTraits<F>>; // Defers to the "FunctionTraits" helper further above

    //////////////////////////////////////////////////////////////////////////////
    // IsFreeFunction_v. Helper template for "FunctionTraits::IsFreeFunction"
    // which stores "true" if "F" is a free function (including static member
    // functions) or false otherwise. This helper template is less verbose
    // however than creating a "FunctionTraits" directly from "F" and accessing
    // its "IsFreeFunction" member. The following provides a convenient wrapper.
    //////////////////////////////////////////////////////////////////////////////
    template <TRAITS_FUNCTION_C F>
    inline constexpr bool IsFreeFunction_v = FunctionTraitsIsFreeFunction_v<FunctionTraits<F>>; // Defers to the "FunctionTraits" helper further above

    ///////////////////////////////////////////////////////////////////////
    // IsFunctor_v. Helper template for "FunctionTraits::IsFunctor" which
    // stores "true" if "F" is a functor or false otherwise (if "true"
    // then note that "IsMemberFunction_v" is also guaranteed to return
    // true). This helper template is less verbose however than creating a
    // "FunctionTraits" directly from "F" and accessing its "IsFunctor"
    // member. The following provides a convenient wrapper.
    ///////////////////////////////////////////////////////////////////////
    template <TRAITS_FUNCTION_C F>
    inline constexpr bool IsFunctor_v = FunctionTraitsIsFunctor_v<FunctionTraits<F>>; // Defers to the "FunctionTraits" helper further above

    ///////////////////////////////////////////////////////////////////////////
    // IsMemberFunction_v. Helper template for "FunctionTraits::IsMemberFunction"
    // which stores "true" if "F" is a non-static member function (or a
    // functor) or "false" otherwise (in the latter case "F" will always be
    // either a free function or static member function - see
    // "IsFreeFunction_v"). This helper template is less verbose however than
    // creating a "FunctionTraits" directly from "F" and accessing its
    // "IsMemberFunction" member. Note that you might want to invoke the
    // following to determine if "F" is in fact a non-static member function
    // before invoking any other helper template specific to non-static member
    // functions only (if you don't know this ahead of time). The following
    // templates are affected:
    //
    //     1) MemberFunctionClass_t
    //     2) MemberFunctionClassName_v
    //     3) IsMemberFunctionConst_v
    //     4) IsMemberFunctionVolatile_v
    //     4) MemberFunctionRefQualifier_v
    //     6) MemberFunctionRefQualifierName_v
    ///////////////////////////////////////////////////////////////////////////
    template <TRAITS_FUNCTION_C F>
    inline constexpr bool IsMemberFunction_v = FunctionTraitsIsMemberFunction_v<FunctionTraits<F>>; // Defers to the "FunctionTraits" helper further above

    //////////////////////////////////////////////////////////////////////////
    // IsMemberFunctionConst_v. Helper template for "FunctionTraits::IsConst"
    // which stores "true" if "F" is a non-static member function with a
    // "const" cv-qualifier or false otherwise. This helper template is less
    // verbose however than creating a "FunctionTraits" directly from "F"
    // and accessing its "IsConst" member. The following provides a
    // convenient wrapper. Note that this member is always false for free
    // functions and static member functions (not applicable to them). You
    // may therefore want to call "IsMemberFunction_v" to determine if "F"
    // is a non-static member function before invoking the following (if
    // required).
    //////////////////////////////////////////////////////////////////////////
    template <TRAITS_FUNCTION_C F>
    inline constexpr bool IsMemberFunctionConst_v = FunctionTraitsIsMemberFunctionConst_v<FunctionTraits<F>>; // Defers to the "FunctionTraits" helper further above

    /////////////////////////////////////////////////////////////////////////////////
    // IsMemberFunctionVolatile_v. Helper template for "FunctionTraits::IsVolatile"
    // which stores "true" if "F" is a non-static member function with a "volatile"
    // cv-qualifier or false otherwise. This helper template is less verbose however
    // than creating a "FunctionTraits" directly from "F" and accessing its
    // "IsVolatile" member. The following provides a convenient wrapper. Note that
    // this member is always false for free functions and static member functions
    // (not applicable to them). You may therefore want to call
    // "IsMemberFunction_v" to determine if "F" is a non-static member function
    // before invoking the following (if required).
    /////////////////////////////////////////////////////////////////////////////////
    template <TRAITS_FUNCTION_C F>
    inline constexpr bool IsMemberFunctionVolatile_v = FunctionTraitsIsMemberFunctionVolatile_v<FunctionTraits<F>>; // Defers to the "FunctionTraits" helper further above

    /////////////////////////////////////////////////////////////////////////
    // IsNoexcept_v. Helper template for "FunctionTraits::IsNoexcept" which
    // stores "true" if "F" is declared as "noexcept" or false otherwise.
    // This helper template is less verbose however than creating a
    // "FunctionTraits" directly from "F" and accessing its "IsNoexcept"
    // member. The following provides a convenient wrapper.
    /////////////////////////////////////////////////////////////////////////
    template <TRAITS_FUNCTION_C F>
    inline constexpr bool IsNoexcept_v = FunctionTraitsIsNoexcept_v<FunctionTraits<F>>; // Defers to the "FunctionTraits" helper further above

    /////////////////////////////////////////////////////////////////////////
    // IsVariadic_v. Helper template for "FunctionTraits::IsVariadic" which
    // stores "true" if "F" is a variadic function (last arg is "...") or
    // false otherwise. This helper template is less verbose however than
    // creating a "FunctionTraits" directly from "F" and accessing its
    // "IsVariadic" member. The following provides a convenient wrapper.
    /////////////////////////////////////////////////////////////////////////
    template <TRAITS_FUNCTION_C F>
    inline constexpr bool IsVariadic_v = FunctionTraitsIsVariadic_v<FunctionTraits<F>>; // Defers to the "FunctionTraits" helper further above

    ///////////////////////////////////////////////////////////////////////////
    // IsVoidReturnType_v. Helper template that returns "true" if the return
    // type of "F" is void or false otherwise (ignoring cv-qualifiers on a
    // "void" return type if any). This helper template is less verbose than
    // creating a "FunctionTraits" directly from "F" and applying
    // "std::is_void" to its "ReturnType" member. The following provides a
    // convenient wrapper.
    ///////////////////////////////////////////////////////////////////////////
    template <TRAITS_FUNCTION_C F>
    inline constexpr bool IsVoidReturnType_v = FunctionTraitsIsVoidReturnType_v<FunctionTraits<F>>; // Defers to the "FunctionTraits" helper further above

    ///////////////////////////////////////////////////////////////////////////
    // MemberFunctionClass_t. Helper alias for "FunctionTraits::Class" which
    // yields the class for a non-static member "F" but less verbose than
    // creating a "FunctionTraits" directly from "F" and accessing its "Class"
    // alias. The following provides a convenient wrapper. Note that this
    // helper applies to non-static member functions only (yielding the class
    // type of non-static member functions). For free functions including
    // static member functions this type is always "void" (not applicable to
    // them noting that the class for static member functions can't be retrieved
    // in this release due to language limitations). Note that you might want
    // to call "IsMemberFunction_v" to determine if you're dealing with a
    // non-static member function before invoking the following (if required).
    ///////////////////////////////////////////////////////////////////////////
    template <TRAITS_FUNCTION_C F>
    using MemberFunctionClass_t = FunctionTraitsMemberFunctionClass_t<FunctionTraits<F>>; // Defers to the "FunctionTraits" helper further above

    /////////////////////////////////////////////////////////////////////////
    // MemberFunctionClassName_v. See "MemberFunctionClass_t()" just above.
    // Simply converts that to a string suitable for display purposes and
    // returns it as a "tstring_view"
    /////////////////////////////////////////////////////////////////////////
    template <TRAITS_FUNCTION_C F>
    inline constexpr tstring_view MemberFunctionClassName_v = FunctionTraitsMemberFunctionClassName_v<FunctionTraits<F>>; // Defers to the "FunctionTraits" helper further above

    //////////////////////////////////////////////////////////////////////////
    // MemberFunctionRefQualifier_v. Helper template for
    // "FunctionTraits::RefQualifier" which stores the reference qualifier
    // for "F" if it's a non-static member function with a "&" or "&&"
    // reference-qualifier or "RefQualifier::None" otherwise. This helper
    // template is less verbose however than creating a "FunctionTraits"
    // directly from "F" and accessing its "RefQualifier" member. The
    // following provides a convenient wrapper. Note that this member is
    // always "RefQualifier::None" for free functions and static member
    // functions (not applicable to them). You may therefore want to call
    // "IsMemberFunction_v" to determine if "F" is a non-static member
    // function before invoking the following (if required).
    //////////////////////////////////////////////////////////////////////////
    template <TRAITS_FUNCTION_C F>
    inline constexpr RefQualifier MemberFunctionRefQualifier_v = FunctionTraitsMemberFunctionRefQualifier_v<FunctionTraits<F>>; // Defers to the "FunctionTraits" helper further above

    /////////////////////////////////////////////////////////////////////////
    // MemberFunctionRefQualifierName_v. See "MemberFunctionRefQualifier_v"
    // just above. Simply converts that to a string suitable for display
    // purposes and returns it as a "tstring_view". Pass "true" (the
    // default) for the "UseAmpersands" template arg arg if you wish to
    // return this as "&" or "&&" or "false" to return it as "LValue" or
    // "RValue" instead. In either case however "None" is always returned if
    // "F" has no reference-qualifier which is usually the case for most
    // functions in practice.
    /////////////////////////////////////////////////////////////////////////
    template <TRAITS_FUNCTION_C F, bool UseAmpersands = true>
    inline constexpr tstring_view MemberFunctionRefQualifierName_v = FunctionTraitsMemberFunctionRefQualifierName_v<FunctionTraits<F>, UseAmpersands>;  // Defers to the "FunctionTraits" helper further above

    ///////////////////////////////////////////////////////////////////////////
    // ReturnType_t. Helper alias for "FunctionTraits::ReturnType" which
    // yields the return type of function "F" but less verbose than creating
    // a "FunctionTraits" directly from "F" and accessing its "ReturnType"
    // alias. The following provides a convenient wrapper.
    //
    // E.g.,
    //
    //       void MyFunction(const std::string &str, int val)
    //       {
    //       }
    //
    //       // Return type for "MyFunction" above (void)
    //       using MyFunctionReturnType = ReturnType_t<decltype(MyFunction)>;
    //       // using MyFunctionReturnType = ReturnType_t<void (const std::string &, int)>; // This will also work (same as above but passing function's type on-the-fly)
    //       // using MyFunctionReturnType = ReturnType_t<void (*)(const std::string &, int)>; // ... and this too (but now using a pointer to the function - references
    //                                                                                         // to the function will also work, and references to pointers to the
    //                                                                                         // function as well)
    ///////////////////////////////////////////////////////////////////////////
    template <TRAITS_FUNCTION_C F>
    using ReturnType_t = FunctionTraitsReturnType_t<FunctionTraits<F>>; // Defers to the "FunctionTraits" helper further above

    //////////////////////////////////////////////////////////////////////////
    // ReturnTypeName(). See "ReturnType_t()" just above. Simply converts
    // that to a string suitable for display purposes and returns it as a
    // "tstring_view" (so a return type like, say, "int" will literally be
    // returned as the string "int", quotes not included)
    //////////////////////////////////////////////////////////////////////////
    template <TRAITS_FUNCTION_C F>
    inline constexpr tstring_view ReturnTypeName_v = FunctionTraitsReturnTypeName_v<FunctionTraits<F>>; // Defers to the "FunctionTraits" helper further above

    /////////////////////////////////////////////////////
    // See this constant for details. Following helper
    // templates for function write traits are declared
    // only if function write traits are supported.
    // They're preprocessed out otherwise (so function
    // write traits simply won't be available).
    /////////////////////////////////////////////////////
    #if defined(FUNCTION_WRITE_TRAITS_SUPPORTED)
        //////////////////////////////////////////////////////////////////////////
        // AddVariadicArgs_t. Type alias for "F" after adding "..." to the end of
        // its argument list if not already present. Note that the calling
        // convention is also changed to the "Cdecl" calling convention for the
        // given compiler. This is the only supported calling convention for
        // variadic functions in this release but most platforms require this
        // calling convention for variadic functions. It ensures that the calling
        // function (opposed to the called function) pops the stack of arguments
        // after the function is called, which is required by variadic functions.
        // Other calling conventions that also do this are possible though none
        // are currently supported in this release (since none of the currently
        // supported compilers support this - such calling conventions are rare
        // in practice).
        //////////////////////////////////////////////////////////////////////////
        template <TRAITS_FUNCTION_C F>
        using AddVariadicArgs_t = FunctionTraitsAddVariadicArgs_t<FunctionTraits<F>>; // Defers to the "FunctionTraits" helper further above

        //////////////////////////////////////////////////////////////////////////
        // RemoveVariadicArgs_t. If "F" is a variadic function (its last arg is
        // "..."), yields a type alias for "F" after removing the "..." from the
        // argument list. All non-variadic arguments (if any) remain intact (only
        // the "..." is removed).
        //////////////////////////////////////////////////////////////////////////
        template <TRAITS_FUNCTION_C F>
        using RemoveVariadicArgs_t = FunctionTraitsRemoveVariadicArgs_t<FunctionTraits<F>>; // Defers to the "FunctionTraits" helper further above

        //////////////////////////////////////////////////////////////////////////
        // MemberFunctionAddConst_t. If "F" is a non-static member function,
        // yields a type alias for "F" after adding the "const" cv-qualifier to
        // the function if not already present. If "F" is a free function
        // including static member functions, yields "F" itself (effectively does
        // nothing since "const" applies to non-static member functions only).
        //////////////////////////////////////////////////////////////////////////
        template <TRAITS_FUNCTION_C F>
        using MemberFunctionAddConst_t = FunctionTraitsMemberFunctionAddConst_t<FunctionTraits<F>>; // Defers to the "FunctionTraits" helper further above

        //////////////////////////////////////////////////////////////////////////
        // MemberFunctionRemoveConst_t. If "F" is a non-static member function,
        // yields a type alias for "F" after removing the "const" cv-qualifier
        // from the function if present. If "F" is a free function including
        // static member functions, yields "F" itself (effectively does nothing
        // since "const" applies to non-static member functions only so will
        // never be present otherwise).
        //////////////////////////////////////////////////////////////////////////
        template <TRAITS_FUNCTION_C F>
        using MemberFunctionRemoveConst_t = FunctionTraitsMemberFunctionRemoveConst_t<FunctionTraits<F>>; // Defers to the "FunctionTraits" helper further above

        //////////////////////////////////////////////////////////////////////////
        // MemberFunctionAddVolatile_t. If "F" is a non-static member function,
        // yields a type alias for "F" after adding the "volatile" cv-qualifier
        // to the function if not already present. If "F" is a free function
        // including static member functions, yields "F" itself (effectively does
        // nothing since "volatile" applies to non-static member functions only).
        //////////////////////////////////////////////////////////////////////////
        template <TRAITS_FUNCTION_C F>
        using MemberFunctionAddVolatile_t = FunctionTraitsMemberFunctionAddVolatile_t<FunctionTraits<F>>; // Defers to the "FunctionTraits" helper further above

        //////////////////////////////////////////////////////////////////////////
        // MemberFunctionRemoveVolatile_t. If "F" is a non-static member
        // function, yields a type alias for "F" after removing the "volatile"
        // cv-qualifier from the function if present. If "F" is a free function
        // including static member functions, yields "F" itself (effectively does
        // nothing since "volatile" applies to non-static member functions only
        // so will never be present otherwise).
        //////////////////////////////////////////////////////////////////////////
        template <TRAITS_FUNCTION_C F>
        using MemberFunctionRemoveVolatile_t = FunctionTraitsMemberFunctionRemoveVolatile_t<FunctionTraits<F>>; // Defers to the "FunctionTraits" helper further above

        //////////////////////////////////////////////////////////////////////////
        // MemberFunctionAddCV_t. If "F" is a non-static member function, yields
        // a type alias for "F" after adding both the "const" AND "volatile"
        // cv-qualifiers to the function if not already present. If "F" is a free
        // function including static member functions, yields "F" itself
        // (effectively does nothing since "const" and "volatile" apply to
        // non-static member functions only).
        //////////////////////////////////////////////////////////////////////////
        template <TRAITS_FUNCTION_C F>
        using MemberFunctionAddCV_t = FunctionTraitsMemberFunctionAddCV_t<FunctionTraits<F>>; // Defers to the "FunctionTraits" helper further above

        //////////////////////////////////////////////////////////////////////////
        // MemberFunctionRemoveCV_t. If "F" is a non-static member function,
        // yields a type alias for "F" after removing both the "const" AND
        // "volatile" cv-qualifiers from the function if present. If "F" is a
        // free function including static member functions, yields "F" itself
        // (effectively does nothing since "const" and "volatile" apply to
        // non-static member functions only so will never be present otherwise).
        //////////////////////////////////////////////////////////////////////////
        template <TRAITS_FUNCTION_C F>
        using MemberFunctionRemoveCV_t = FunctionTraitsMemberFunctionRemoveCV_t<FunctionTraits<F>>; // Defers to the "FunctionTraits" helper further above

        //////////////////////////////////////////////////////////////////////////
        // MemberFunctionAddLValueReference_t. If "F" is a non-static member
        // function, yields a type alias for "F" after adding the "&"
        // reference-qualifier to the function if not already present (replacing
        // the "&&" reference-qualifier if present). If "F" is a free function
        // including static member functions, yields "F" itself (effectively does
        // nothing since reference-qualifiers apply to non-static member
        // functions only).
        //////////////////////////////////////////////////////////////////////////
        template <TRAITS_FUNCTION_C F>
        using MemberFunctionAddLValueReference_t = FunctionTraitsMemberFunctionAddLValueReference_t<FunctionTraits<F>>; // Defers to the "FunctionTraits" helper further above

        //////////////////////////////////////////////////////////////////////////
        // MemberFunctionAddRValueReference_t. If "F" is a non-static member
        // function, yields a type alias for "F" after adding the "&&"
        // reference-qualifier to the function (replacing the "&"
        // reference-qualifier if present). If "F" is a free function including
        // static member functions, yields "F" itself (effectively does nothing
        // since reference-qualifiers apply to non-static member functions only).
        //////////////////////////////////////////////////////////////////////////
        template <TRAITS_FUNCTION_C F>
        using MemberFunctionAddRValueReference_t = FunctionTraitsMemberFunctionAddRValueReference_t<FunctionTraits<F>>; // Defers to the "FunctionTraits" helper further above

        //////////////////////////////////////////////////////////////////////////
        // MemberFunctionRemoveReference_t. If "F" is a non-static member
        // function, yields a type alias for "F" after removing the "&" or "&&"
        // reference-qualifier from the function if present. If "F" is a free
        // function including static member functions, yields "F" itself
        // (effectively does nothing since reference-qualifiers to non-static
        // member functions only so will never be present otherwise).
        //////////////////////////////////////////////////////////////////////////
        template <TRAITS_FUNCTION_C F>
        using MemberFunctionRemoveReference_t = FunctionTraitsMemberFunctionRemoveReference_t<FunctionTraits<F>>; // Defers to the "FunctionTraits" helper further above

        //////////////////////////////////////////////////////////////////////////
        // AddNoexcept_t. Type alias for "F" after adding "noexcept" to "F" if
        // not already present
        //////////////////////////////////////////////////////////////////////////
        template <TRAITS_FUNCTION_C F>
        using AddNoexcept_t = FunctionTraitsAddNoexcept_t<FunctionTraits<F>>; // Defers to the "FunctionTraits" helper further above

        //////////////////////////////////////////////////////////////////////////
        // RemoveNoexcept_t. Type alias for "F" after removing "noexcept" from
        // "F" if present
        //////////////////////////////////////////////////////////////////////////
        template <TRAITS_FUNCTION_C F>
        using RemoveNoexcept_t = FunctionTraitsRemoveNoexcept_t<FunctionTraits<F>>; // Defers to the "FunctionTraits" helper further above

        //////////////////////////////////////////////////////////////////////////
        // ReplaceCallingConvention_t. Type alias for "F" after replacing its
        // calling convention with the platform-specific calling convention
        // corresponding to "NewCallingConventionT" (a "CallingConvention"
        // enumerator declared in "TypeTraits.h"). For instance, if you pass
        // "CallingConvention::FastCall" then the calling convention on "F" is
        // replaced with "__attribute__((cdecl))" on GCC, Clang and others, but
        // "__cdecl" on Microsoft platforms. Note however that the calling
        // convention for variadic functions (those whose last arg is "...")
        // can't be changed in this release. Variadic functions require that the
        // calling function pop the stack to clean up passed arguments and only
        // the "Cdecl" calling convention supports that in this release (on all
        // supported compilers at this writing). Attempts to change it are
        // therefore ignored. Note that you also can't change the calling
        // convention of free functions to "CallingConvention::Thiscall"
        // (including for static member functions which are considered "free"
        // functions). Attempts to do so are ignored since the latter calling
        // convention applies to non-static member functions only. Lastly, please
        // note that compilers will sometimes change the calling convention
        // declared on your functions to the "Cdecl" calling convention depending
        // on the compiler options in effect at the time (in particular when
        // compiling for 64 bits opposed to 32 bits, though the "Vectorcall"
        // calling convention is supported on 64 bits). Therefore, if you specify
        // a calling convention that the compiler changes to "Cdecl" based on the
        // compiler options currently in effect, then "ReplaceCallingConvention_t"
        // will also ignore your calling convention and apply "Cdecl" instead
        // (since that's what the compiler actually uses).
        //////////////////////////////////////////////////////////////////////////
        template <TRAITS_FUNCTION_C F, CallingConvention NewCallingConventionT>
        using ReplaceCallingConvention_t = FunctionTraitsReplaceCallingConvention_t<FunctionTraits<F>, NewCallingConventionT>; // Defers to the "FunctionTraits" helper further above

        //////////////////////////////////////////////////////////////////////////
        // MemberFunctionReplaceClass_t. If "F" is a non-static member function,
        // yields a type alias for "F" after replacing the class this function
        // belongs to with "NewClassT". If "F" is a free function including
        // static member functions, yields "F" itself (effectively does nothing
        // since a "class" applies to non-static member functions only so will
        // never be present otherwise - note that due to limitations in C++
        // itself, replacing the class for static member functions is not
        // supported).
        //////////////////////////////////////////////////////////////////////////
        template <TRAITS_FUNCTION_C F, IS_CLASS_C NewClassT>
        using MemberFunctionReplaceClass_t = FunctionTraitsMemberFunctionReplaceClass_t<FunctionTraits<F>, NewClassT>; // Defers to the "FunctionTraits" helper further above

        //////////////////////////////////////////////////////////////////////////
        // ReplaceReturnType_t. Type alias for "F" after replacing its return
        // type with "NewReturnTypeT"
        //////////////////////////////////////////////////////////////////////////
        template <TRAITS_FUNCTION_C F, typename NewReturnTypeT>
        using ReplaceReturnType_t = FunctionTraitsReplaceReturnType_t<FunctionTraits<F>, NewReturnTypeT>; // Defers to the "FunctionTraits" helper further above

        //////////////////////////////////////////////////////////////////////////
        // ReplaceArgs_t. Type alias for "F" after replacing all its existing
        // non-variadic arguments with the args given by "NewArgsT" (a parameter
        // pack of the types that become the new argument list). If none are
        // passed then an empty argument list results instead, though if variadic
        // args are present in "F" then they still remain intact (the "..."
        // remains - read on). The resulting alias is identical to "F" itself
        // except that the non-variadic arguments in "F" are completely replaced
        // with "NewArgsT". Note that if "F" is a variadic function (its last
        // parameter is "..."), then it remains a variadiac function after the
        // call (the "..." remains in place). If you wish to explicitly add or
        // remove the "..." as well then pass the resulting type to
        // "AddVariadicArgs_t" or "RemoveVariadicArgs_t" respectively (either
        // before or after the call to "ReplaceArgs_t"). Note that if you wish to
        // remove specific arguments instead of all of them, then call
        // "ReplaceNthArg_t" instead. Lastly, you can alternatively use
        // "ReplaceArgsTuple_t" instead of "ReplaceArgs_t" if you have a
        // "std::tuple" of types you wish to use for the argument list instead of
        // a parameter pack. "ReplaceArgsTuple_t" is identical to
        // "ReplaceArgs_t" otherwise (it ultimately defers to it).
        //////////////////////////////////////////////////////////////////////////
        template <TRAITS_FUNCTION_C F, typename... NewArgsT>
        using ReplaceArgs_t = FunctionTraitsReplaceArgs_t<FunctionTraits<F>, NewArgsT...>; // Defers to the "FunctionTraits" helper further above

        //////////////////////////////////////////////////////////////////////////
        // ReplaceArgsTuple_t. Identical to "ReplaceArgs_t" just above except the
        // argument list is passed as a "std::tuple" instead of a parameter pack
        // (via the 2nd template arg). The types in the "std::tuple" are
        // therefore used for the resulting argument list. "ReplaceArgsTuple_t"
        // is otherwise identical to "ReplaceArgs_t" (it ultimately defers to
        // it).
        //////////////////////////////////////////////////////////////////////////
        template <TRAITS_FUNCTION_C F, TUPLE_C NewArgsTupleT>
        using ReplaceArgsTuple_t = FunctionTraitsReplaceArgsTuple_t<FunctionTraits<F>, NewArgsTupleT>; // Defers to the "FunctionTraits" helper further above

        //////////////////////////////////////////////////////////////////////////
        // ReplaceNthArg_t. Type alias for "F" after replacing its (zero-based)
        // "Nth" argument with "NewArgT". Pass "N" via the 2nd template arg
        // (e.g., the zero-based index of the arg you're targeting) and the type
        // you wish to replace it with via the 3rd template arg ("NewArgT"). The
        // resulting alias is therefore identical to "F" except its "Nth"
        // argument is replaced by "NewArgT" (so passing, say, zero-based "2" for
        // "N" and "int" for "NewArgT" would replace the 3rd function argument
        // with an "int"). Note that "N" must be less than the number of
        // arguments in the function or a "static_assert" will occur (new
        // argument types can't be added using this trait, only existing argument
        // types replaced). If you need to replace multiple arguments then
        // recursively call "ReplaceNthArg_t" again, passing the result as the
        // "F" template arg of "ReplaceNthArg_t" as many times as you need to
        // (each time specifying a new "N" and "NewArgT"). If you wish to replace
        // all arguments at once then call "ReplaceArgs_t" or
        // "ReplaceArgsTuple_t" instead. Lastly, note that if "F" has variadic
        // arguments (it ends with "..."), then these remain intact. If you need
        // to remove them then call "RemoveVariadicArgs_t" before or after the
        // call to "ReplaceNthArg_t".
        //////////////////////////////////////////////////////////////////////////
        template <TRAITS_FUNCTION_C F, std::size_t N, typename NewArgT>
        using ReplaceNthArg_t = FunctionTraitsReplaceNthArg_t<FunctionTraits<F>, N, NewArgT>; // Defer to this in private namespace just above
    #endif // #if defined(FUNCTION_WRITE_TRAITS_SUPPORTED)

    ////////////////////////////////////////////////////////////////////////////
    // "IsForEachFunctor" (primary template). Determines if template arg "T" is
    // a functor type whose "operator()" member has the following signature and
    // is also callable in the context of whether "T" is "const" and/or
    // "volatile" and or an lvalue or rvalue (read on):
    //
    //     template <std::size_t I>
    //     bool operator()() [const] [volatile] [&|&&];
    //
    // If "T" contains this function AND it can also be called on an instance
    // of "T" (again, read on), then "T" qualifies as a functor that can be
    // passed to function template "ForEach()" declared later on. Note that the
    // specialization just below does the actual work of checking for this. It
    // simply invokes the above function template in an unevaluated context
    // using "std::declval()" as follows:
    //
    //     T::operator<0>()
    //
    // If the call succeeds then "operator()" must not only exist as a member
    // of "T" with a "bool" return type (we check for this as well), but it's
    // also callable in the context of whether "T" is "const" and/or "volatile"
    // and or an lvalue or rvalue. Note that to qualify as an lvalue for
    // purposes of "IsForEachFunctor", "T" must be an lvalue reference such as
    // "int &". To qualify as an rvalue, "T" must either be a non-reference
    // type such as a plain "int" OR an rvalue reference such as "int &&". The
    // behavior of what qualifies as an lvalue and rvalue for purposes of
    // using "IsForEachFunctor" therefore follows the usual perfect forwarding
    // rules for function template arguments (where an lvalue reference for "T"
    // means the function's "T &&" argument collapses to an lvalue reference,
    // and a non-reference type or rvalue reference type for "T" means the
    // function's "T &&" argument is an rvalue reference - read up on perfect
    // forwarding for details if you're not already familiar this).
    //
    // As an example, if "T" contains the above "operator()" template but it's
    // not declared with the "const" qualifier but "T" itself is (declared
    // "const"), then calling the function as shown above fails since you can't
    // call a non-const member function using a constant object. The primary
    // template therefore kicks in, inheriting from "std::false_type".
    // Similarly, if "operator()" is declared with the "&&" reference qualifier
    // (rare as this is in practice), but "T" is an lvalue reference, then
    // again, the above call fails since you can't invoke a member function
    // declared with the "&&" (rvalue) reference qualifier using an lvalue
    // reference. Again, the primary template will therefore kick in (once
    // again inheriting from "std::false_type"). The upshot is that not only
    // must "operator()" exist as a member of "T", it must also be callable in
    // the context of "T" itself (whether "T" is "const" and/or "volatile"
    // and/or an lvalue reference, rvalue reference or non-reference - in all
    // cases "operator()" must be properly declared to support calls based on
    // the cv-qualifiers and/or lvalue/rvalue state of "T" itself as described
    // above). If it is then the specialization kicks in and "IsForEachFunctor"
    // therefore inherits from "std::true_type" (meaning "T" is a suitable
    // functor type for passing as the 2nd template arg of function template
    // "ForEach()").
    ////////////////////////////////////////////////////////////////////////////
    template <typename T, typename = void>
    struct IsForEachFunctor : std::false_type
    {
    };

    /////////////////////////////////////////////////////////////////
    // Specialization of "IsForEachFunctor" (primary) template just
    // above. This specialization does all the work. See primary
    // template above for details. Note that calls to "declval(T)"
    // return "T &&" (unless "T" is void - see "declval()" docs) so
    // the following call to this works according to the usual
    // perfect forwarding rules in C++. That is, if "T" is an lvalue
    // reference then "declval()" returns an lvalue reference (due
    // to the C++ reference collapsing rules), otherwise "T" must be
    // an a non-reference or an rvalue reference so "declval"
    // returns an rvalue reference (in either case). In all cases
    // "declval()" therefore returns the same function argument type
    // used in a perfect forwarding function. Such arguments are
    // always of the type "T &&" which resolves to an lvalue
    // reference only if "T" is an lvalue reference (again, due to
    // the C++ reference collapsing rules), or an rvalue reference
    // if "T" is a non-reference or rvalue reference. "declval()"
    // therefore returns the exact same type as a perfectly
    // forwarded argument after plugging in "T" which is what we
    // require here (we're invoking "operator()" on that argument in
    // the following call in an unevaluated context simply to make
    // sure it can be invoked - the primary template kicks in via
    // SFINAE otherwise).
    /////////////////////////////////////////////////////////////////
    template <typename T>
    struct IsForEachFunctor<T,
                            // See explanation of "declval()" in comments above
                            std::enable_if_t<std::is_same_v<decltype(std::declval<T>().template operator()<std::size_t(0)>()), bool>>
                           > : std::true_type
    {
    };

    /////////////////////////////////////////////////////////
    // Helper variable template for "IsForEachFunctor" just
    // above (with the usual "_v" suffix).
    /////////////////////////////////////////////////////////
    template <typename T>
    inline constexpr bool IsForEachFunctor_v = IsForEachFunctor<T>::value;
#endif // #if !defined(DECLARE_MACROS_ONLY)

    //////////////////////////////////////////////
    // Concept for above template (see following
    // #defined constant for details)
    //////////////////////////////////////////////
    #if defined(USE_CONCEPTS)
        #if !defined(DECLARE_MACROS_ONLY)
            template <typename T>
            concept ForEachFunctor_c = IsForEachFunctor_v<T>;
        #endif

        #define FOR_EACH_FUNCTOR_C StdExt::ForEachFunctor_c
    #else
        #define STATIC_ASSERT_IS_FOR_EACH_FUNCTOR(T) static_assert(StdExt::IsForEachFunctor_v<T>);

        #define FOR_EACH_FUNCTOR_C typename
    #endif

#if !defined(DECLARE_MACROS_ONLY)
    /////////////////////////////////////////////////////
    // In C++20 and later we rely on a locally defined
    // lambda declared in function template "ForEach()"
    // instead (that does the same thing as class
    // "ForEachImpl" does below in C++17 or earlier).
    /////////////////////////////////////////////////////
    #if CPP17_OR_EARLIER
        /////////////////////////////////////////////////////
        // For internal use only (by "ForEach()" just after
        // this namespace)
        /////////////////////////////////////////////////////
        namespace Private
        {
            /////////////////////////////////////////////////////////////
            // class ForEachImpl. Private implementation class used by
            // function template "ForEach()" declared just after this
            // private namespace. See this for details. Note that in the
            // following class, "ForEachFunctorT" is always the same
            // template type passed to "ForEach()", so either "T &" for
            // the lvalue case or just plain "T" or "T &&" for the rvalue
            // case (for the "rvalue" case however usually just plain
            // "T" as per the usual perfect forwarding rules when
            // invoking such functions via implicit type deduction).
            ///////////////////////////////////////////////////////////
            template <std::size_t N, typename ForEachFunctorT>
            class ForEachImpl
            {
                STATIC_ASSERT_IS_FOR_EACH_FUNCTOR(ForEachFunctorT);

            public:
                /////////////////////////////////////////////////////////
                // Constructor. Note that we always pass "functor" by a
                // "std::forward" reference so "functor" resolves to "&"
                // or "&&" accordingly
                /////////////////////////////////////////////////////////
                constexpr ForEachImpl(ForEachFunctorT&& functor) noexcept
                    ///////////////////////////////////////////////////////////
                    // "std::forward" required when "ForEachFunctorT" is "&&".
                    // Both "functor" and "m_Functor" are therefore "&&" but
                    // "functor" is still an lvalue because it's named (rvalue
                    // references that are named are lvalues!). Therefore
                    // illegal to bind an lvalue (functor) to an rvalue
                    // reference (m_Functor) so "std::forward" is required to
                    // circumvent this (for the "&&" case it returns an "&&"
                    // reference but it's now unnamed so an rvalue and therefore
                    // suitable for initializing "m_Functor")
                    ///////////////////////////////////////////////////////////
                    : m_Functor(std::forward<ForEachFunctorT>(functor))
                {
                }

                template <std::size_t I>
                constexpr bool Process() const
                {
                    if constexpr (I < N)
                    {
                        //////////////////////////////////////////////////////////
                        // Note: Call to "std::forward()" here required to:
                        //
                        //    1) Perfect forward "m_Functor" back to "&" or "&&"
                        //       accordingly
                        //    2) In the "&&" case, invoke "operator()" in the
                        //       context of an rvalue (in particular, can't do
                        //       this on "m_Functor" directly, without invoking
                        //       "std::forward", since "m_Functor" is an lvalue
                        //       so the lvalue version of "operator()" would kick
                        //       in in the following call instead!!)
                        //////////////////////////////////////////////////////////
                        if (std::forward<ForEachFunctorT>(m_Functor).template operator()<I>())
                        {
                            ////////////////////////////////////////////////
                            // Recursive call (in spirit anyway - actually
                            // stamps out and runs another version of this
                            // function specialized on I + 1)
                            ////////////////////////////////////////////////
                            return Process<I + 1>();
                        }
                        else
                        {
                            return false;
                        }
                    }
                    else
                    {
                        return true;
                    }
                }

            private:
                /////////////////////////////////////////////////////
                // Note: Resolves to "&" in the lvalue case or "&&"
                // otherwise (by the usual C++ reference collapsing
                // rules - "ForEachFunctorT" is always "T &" in the
                // former case or either plain "T" or "T &&"
                // otherwise)
                /////////////////////////////////////////////////////
                ForEachFunctorT &&m_Functor;
            };
        } // namespace Private
    #endif // #if CPP17_OR_EARLIER

    ///////////////////////////////////////////////////////////////////
    // ForEach(). Generic function template effectively equivalent to
    // a "for" loop but used at compile time to execute the given
    // "functor" "N" times (unless the "functor" arg returns false on
    // any of those iterations in which case processing immediately
    // stops and "false" is immediately returned - read on).
    //
    // Note that "operator()" in your functor must be declared as
    // follows (and will typically be "const" as seen though it can be
    // non-const as well if required, and/or noexcept as well)
    //
    //     template <std::size_t I>
    //     bool operator()() const
    //     {
    //         // Process "I" accordingly
    //
    //         /////////////////////////////////////////////////////
    //         // "true" means continue processing so this function
    //         // will immediately be called again specialized on
    //         // "I + 1" until "N" is reached (EXclusive). Return
    //         // false instead to immediately stop processing
    //         // (equivalent to a "break" statement in a regular
    //         // "for" loop)
    //         /////////////////////////////////////////////////////
    //         return true;
    //     }
    //
    // "ForEach()" effectively stamps out "N" copies of "operator()"
    // above, once for each "I" in the range 0 to N - 1 inclusive
    // (unless "N" is zero in which case "operator()" is still stamped
    // out for "I" equal zero but never actually called - it's only
    // stamped out to validate that "ForEachFunctorT" is a valid
    // template arg but this means your functor shouldn't do anything
    // that might be illegal when "N" is zero, like invoke
    // "std::tuple_element_t" for instance, since index zero would be
    // illegal in this case so compilation will fail). In any case,
    // note that your functor should return true as seen above unless
    // you wish to "break" like in a normal "for" loop, in which case
    // it should return false. Either value (true or false) is
    // ultimately returned by the function. If true then "I" is simply
    // incremented by 1 and "operator()" is immediately called again
    // with the newly incremented "I" (stamping out a new copy of
    // "operator()" with the newly incremented "I"). If false then
    // processing immediately exits instead and no new copies of
    // "operator()" are stamped out (for "I + 1" and beyond).
    //
    //    Example (iterates all elements of a "std::tuple" but
    //             see the helper function "ForEachTupleType()"
    //             declared later which wraps the following so
    //             it's cleaner - for tuples you should rely
    //             on it instead normally but it ultimately
    //             just defers to "ForEach()" anyway)
    //    -----------------------------------------------------
    //    std::Tuple<int, float, double> myTuple; // Tuple with 3 elements whose types we wish to iterate (and display)
    //    using TupleT = decltype(myTuple); // Type of the above tuple
    //
    //    //////////////////////////////////////////////////////
    //    // Lambda (template) stamped out and invoked 3 times
    //    // by "ForEach()" below (for I=0, I=1 and I=2). See
    //    // "ForEach()" comments below.
    //    //////////////////////////////////////////////////////
    //    const auto displayTupleElement = []<std::size_t I>()
    //                                     {
    //                                         // Type of the this tuple element (for the current "I")
    //                                         using TupleElement_t = std::tuple_element_t<I, TupleT>;
    //
    //                                         /////////////////////////////////////////////////
    //                                         // Displays (literally) "int" when I=0, "float"
    //                                         // when I=1 and "double" when I=2 (for this
    //                                         // particular tuple - see "myTuple above)
    //                                         /////////////////////////////////////////////////
    //                                         std::cout << TypeName_v<TupleElement_t> << "\n";
    //
    //                                         //////////////////////////////////////////////
    //                                         // Return false here instead if you need to
    //                                         // stop processing (equivalent to "break" in
    //                                         // a normal "for" loop). Returning "true"
    //                                         // instead as we're doing here means all
    //                                         // tuple elements will be processed.
    //                                         //////////////////////////////////////////////
    //                                         return true;
    //                                     };
    //
    //     ///////////////////////////////////////////////////////////////////
    //     // Call "ForEach()" which iterates (effectively does a "foreach")
    //     // of the above lambda "N" times, where "N" is explicitly passed
    //     // as the 1st template arg to "ForEach()" (the number of elements
    //     // in our tuple, 3 in this case), and the 2nd template arg, the
    //     // type of the functor, "ForEachFunctorT", is implicitly deduced
    //     // from the arg itself. We're passing "displayTupleElement" so
    //     // the 2nd template arg is implicitly deduced to the type of this
    //     // functor, i.e., whatever class the compiler generates for the
    //     // above lambda behind the scenes (recall that a lambda is just
    //     // syntactic sugar for a compiler-generated functor - the functor's
    //     // class is called its "closure type"). The upshot is that
    //     // "operator()" in this functor (the above lambda) is effectively
    //     // stamped out 3 times and invoked for each (for I=0, I=1 and I=2)
    //     ///////////////////////////////////////////////////////////////////
    //     ForEach<std::tuple_size_v<TupleT>>(displayTupleElement);
    ///////////////////////////////////////////////////////////////////
    template <std::size_t N, FOR_EACH_FUNCTOR_C ForEachFunctorT>
    inline constexpr bool ForEach(ForEachFunctorT&& functor)
    {
        // Lambda templates not supported until C++20
        #if CPP20_OR_LATER
            ////////////////////////////////////////////////////////////
            // Lambda template we'll be calling "N" times for the given
            // template arg "N" in "ForEach()" (the function you're now
            // reading). Each iteration invokes "functor", where
            // processing stops after "N" calls to "functor" or
            // "functor" returns false, whichever comes first (false only
            // returned if "functor" wants to break like a normal "for"
            // loop, which rarely happens in practice so we usually
            // iterate "N" times)
            //
            // IMPORTANT:
            // ---------
            // The parameter "functor" (we'll call this the "outer"
            // functor) is always a reference type so it's always of
            // the form "T&" or "T&&". The following lambda captures it
            // by (lvalue) reference as seen so it effectively creates
            // an (lvalue) reference (member) variable that either
            // looks like this:
            //
            //     T& &functor;
            //
            // or this:
            //
            //     T&& &functor;
            //
            // We'll call this the "inner" functor. In either case
            // however the usual reference collapsing rules then kick
            // in so it always resolves to this (Google for these
            // rules if you're not already familiar):
            //
            //     T& functor;
            //
            // IOW, the inner "functor" is always an lvalue reference
            // inside the lambda even when the outer "functor" is an
            // rvalue reference. We therefore need to invoke
            // "std::forward" on it in the lambda below so that we get
            // back the correct "&" or "&&" reference of the outer
            // "functor" itself, which we then invoke in the correct
            // "&" or "&&" context below. See comments below.
            ////////////////////////////////////////////////////////////
            const auto process = [&functor]<std::size_t I>
                                 (const auto &process)
                                 {
                                     if constexpr (I < N)
                                     {
                                         //////////////////////////////////////////////////////////
                                         // See comments above. We call "std::forward()" here to
                                         // convert the inner "functor" back to the correct "&" or
                                         // "&&" reference type of the outer "functor" (note that
                                         // the inner "functor" member variable we're working with
                                         // here is always an lvalue reference as described in the
                                         // comments above). We then immediately invoke the
                                         // functor in this context noting that it wouldn't work
                                         // correctly in the "&&" case if we assigned the return
                                         // value of "std::forward" to a variable first and then
                                         // invoked the functor. We need to invoke the functor
                                         // using the return value of "std::forward" directly,
                                         // on-the-fly as seen. If we assigned it to a variable
                                         // first instead then in the "&&" case we'd have a
                                         // variable of type "&&" but since the variable has a
                                         // name it would be an lvalue, since named variables are
                                         // always lvalues (even for "&&" reference variables).
                                         // Invoking the functor on that lvalue variable would
                                         // therefore call the functor in the context of an
                                         // lvalue, not an rvalue which is what we require (so,
                                         // for instance, if the "operator()" member of "functor"
                                         // has an "&&" reference qualifier, though this would be
                                         // rare in practice but still always possible, a compiler
                                         // error would occur since we'd be attempting to call
                                         // that function using an lvalue which isn't legal -
                                         // doing it using the rvalue returned directly by
                                         // "std::forward()" works correctly).
                                         //////////////////////////////////////////////////////////
                                         if (std::forward<ForEachFunctorT>(functor).template operator()<I>())
                                         {
                                             ////////////////////////////////////////////////
                                             // Recursive call (in spirit anyway - actually
                                             // stamps out and runs another version of this
                                             // function specialized on I + 1)
                                             ////////////////////////////////////////////////
                                             return process.template operator()<I + 1>(process);
                                         }
                                         else
                                         {
                                             return false;
                                         }
                                     }
                                     else
                                     {
                                         return true;
                                     }
                                 };

            return process.template operator()<0>(process);
        #else
            STATIC_ASSERT_IS_FOR_EACH_FUNCTOR(ForEachFunctorT);

            //////////////////////////////////////////////////////////
            // Create an instance of "Private::ForEachImpl" and
            // invoke its "Process()" member function template,
            // passing 0 for its template arg (to start processing
            // the first iteration). The latter function then
            // recursively invokes itself, calling "functor" "N"
            // times or until "functor" returns false, whichever
            // comes first (false only returned if "functor" wants to
            // break like a normal "for" loop, which rarely happens
            // in practice so we usually iterate "N" times)
            //
            // Note: The "ForEachFunctorT" template arg we're passing
            // here as the 2nd template arg of class of
            // "Private::ForEachImpl" means that the latter's
            // constructor will take the same arg type as "functor"
            // itself (& or && as usual). We then call "std::forward"
            // as seen to pass "functor" to the constructor. Note
            // that "std::forward" is mandatory here because
            // "functor" is an lvalue even in the "&&" case (since
            // it's named - named rvalue references are still
            // lvalues). Attempting to pass "functor" directly would
            // therefore cause a compiler error since you can't bind
            // an lvalue (functor) to an rvalue reference (the arg
            // type that the "Private::ForEachImpl" constructor is
            // expecting in this case).
            ///////////////////////////////////////////////////////////
            return Private::ForEachImpl<N, ForEachFunctorT>(std::forward<ForEachFunctorT>(functor)).template Process<0>();
        #endif
    }

    ////////////////////////////////////////////////////////////////////////////
    // "IsForEachTupleFunctor" (primary template). Determines if template arg
    // "T" is a functor type whose "operator()" member has the following
    // signature and is also callable in the context of whether "T" is "const"
    // and/or "volatile" and or an lvalue or rvalue (read on):
    //
    //     template <std::size_t I, typename TupleElementT>
    //     bool operator()() [const] [volatile] [&|&&];
    //
    // If "T" contains this function AND it can also be called on an instance
    // of "T" (again, read on), then "T" qualifies as a functor that can be
    // passed to function template "ForEachTupleType()" declared later on. Note
    // that the specialization just below does the actual work of checking for
    // this. It simply invokes the above function template in an unevaluated
    // context using "std::declval()" as follows:
    //
    //     T::operator<0, void>()
    //
    // If the call succeeds then "operator()" must not only exist as a member
    // of "T" with a "bool" return type (we check for this as well), but it's
    // also callable in the context of whether "T" is "const" and/or "volatile"
    // and or an lvalue or rvalue. Note that to qualify as an lvalue for
    // purposes of "IsForEachTupleFunctor", "T" must be an lvalue reference
    // such as "int &". To qualify as an rvalue, "T" must either be a
    // non-reference type such as a plain "int" OR an rvalue reference such as
    // "int &&". The behavior of what qualifies as an lvalue and rvalue for
    // purposes of using "IsForEachTupleFunctor" therefore follows the usual
    // perfect forwarding rules for function template arguments (where an
    // lvalue reference for "T" means the function's "T &&" argument collapses
    // to an lvalue reference, and a non-reference type or rvalue reference
    // type for "T" means the function's "T &&" argument is an rvalue reference
    // - read up on perfect forwarding for details if you're not already
    // familiar this).
    //
    // As an example, if "T" contains the above "operator()" template but it's
    // not declared with the "const" qualifier but "T" itself is (declared
    // "const"), then calling the function as shown above fails since you can't
    // call a non-const member function using a constant object. The primary
    // template therefore kicks in, inheriting from "std::false_type".
    // Similarly, if "operator()" is declared with the "&&" reference qualifier
    // (rare as this is in practice), but "T" is an lvalue reference, then
    // again, the above call fails since you can't invoke a member function
    // declared with the "&&" (rvalue) reference qualifier using an lvalue
    // reference. Again, the primary template will therefore kick in (once
    // again inheriting from "std::false_type"). The upshot is that not only
    // must "operator()" exist as a member of "T", it must also be callable in
    // the context of "T" itself (whether "T" is "const" and/or "volatile"
    // and/or an lvalue reference, rvalue reference or non-reference - in all
    // cases "operator()" must be properly declared to support calls based on
    // the cv-qualifiers and/or lvalue/rvalue state of "T" itself as described
    // above). If it is then the specialization kicks in and
    // "IsForEachTupleFunctor" therefore inherits from "std::true_type"
    // (meaning "T" is a suitable functor type for passing as the 2nd template
    // arg of function template "ForEachTupleType()").
    ////////////////////////////////////////////////////////////////////////////
    template <typename T, typename = void>
    struct IsForEachTupleFunctor : std::false_type
    {
    };

    /////////////////////////////////////////////////////////////////
    // Specialization of "IsForEachTupleFunctor" (primary) template
    // just above. This specialization does all the work. See
    // primary template above for details. Note that calls to
    // "declval(T)" return "T &&" (unless "T" is void - see
    // "declval()" docs) so the following call to this works
    // according to the usual perfect forwarding rules in C++. That
    // is, if "T" is an lvalue reference then "declval()" returns an
    // lvalue reference (due to the C++ reference collapsing rules),
    // otherwise "T" must be a non-reference or an rvalue reference
    // so "declval" returns an rvalue reference (in either case). In
    // all cases "declval()" therefore returns the same function
    // argument type used in a perfect forwarding function. Such
    // arguments are always of the type "T &&" which resolves to an
    // lvalue reference only if "T" is an lvalue reference (again,
    // due to the C++ reference collapsing rules), or an rvalue
    // reference if "T" is a non-reference or rvalue reference.
    // "declval()" therefore returns the exact same type as a
    // perfectly forwarded argument after plugging in "T" which is
    // what we require here (we're invoking "operator()" on that
    // argument in the following call in an unevaluated context
    // simply to make sure it can be invoked - the primary template
    // kicks in via SFINAE otherwise).
    /////////////////////////////////////////////////////////////////
    template <typename T>
    struct IsForEachTupleFunctor<T,
                                 // See explanation of "declval()" in comments above
                                 std::enable_if_t<std::is_same_v<decltype(std::declval<T>().template operator()<std::size_t(0), void>()), bool>>
                                > : std::true_type
    {
    };

    /////////////////////////////////////////////////////////
    // Helper variable template for "IsForEachTupleFunctor"
    // just above (with the usual "_v" suffix).
    /////////////////////////////////////////////////////////
    template <typename T>
    inline constexpr bool IsForEachTupleFunctor_v = IsForEachTupleFunctor<T>::value;
#endif // #if !defined(DECLARE_MACROS_ONLY)

    //////////////////////////////////////////////
    // Concept for above template (see following
    // #defined constant for details)
    //////////////////////////////////////////////
    #if defined(USE_CONCEPTS)
        #if !defined(DECLARE_MACROS_ONLY)
            template <typename T>
            concept ForEachTupleFunctor_c = IsForEachTupleFunctor_v<T>;
        #endif

        #define FOR_EACH_TUPLE_FUNCTOR_C StdExt::ForEachTupleFunctor_c
    #else
        #define STATIC_ASSERT_IS_FOR_EACH_TUPLE_FUNCTOR(T) static_assert(StdExt::IsForEachTupleFunctor_v<T>);

        #define FOR_EACH_TUPLE_FUNCTOR_C typename
    #endif

#if !defined(DECLARE_MACROS_ONLY)
    ///////////////////////////////////////////////////////////////////////////
    // For C++17 only we'll rely on the "ProcessTupleType" functor declared
    // just below (used exclusively by function template "ForEachTupleType()"
    // declared just after). For C++20 and later we'll rely on a lambda
    // template instead (the equivalent of the struct just below but locally
    // declared in function template "ForEachTupleType()" so it's cleaner).
    // Note that lambda templates aren't supported until C++20. Lastly, note
    // that we're currently inside a "#if CPP17_OR_LATER" preprocessor block
    // (we're processing code for C++17 or later) so if the #defined constant
    // CPP17_OR_EARLIER we're testing just below is false (0), then we're
    // guaranteed to be processing C++20 or later (again, because we know that
    // the #defined constant CPP17_OR_LATER is currently true).
    // /////////////////////////////////////////////////////////////////////////
    #if CPP17_OR_EARLIER
        ///////////////////////////////////////////////////
        // For internal use only (by "ForEachTupleType()"
        // just after this namespace)
        ///////////////////////////////////////////////////
        namespace Private
        {
            /////////////////////////////////////////////////////////////
            // class ProcessTupleType. Private implementation class
            // used by function template "ForEachTupleType()" declared
            // just after this private namespace. See this for
            // details.  Note that in the following class,
            // "ForEachTupleFunctorT" is always the same template type
            // passed to "ForEachTupleType()", so either "T &" for the
            // lvalue case or just plain "T" or "T &&" for the rvalue
            // case (for the "rvalue" case however usually just plain
            // "T" as per the usual perfect forwarding rules when
            // invoking such functions via implicit type deduction).
            ///////////////////////////////////////////////////////////
            template <typename TupleT, typename ForEachTupleFunctorT>
            class ProcessTupleType
            {
                STATIC_ASSERT_IS_TUPLE(TupleT);
                STATIC_ASSERT_IS_FOR_EACH_TUPLE_FUNCTOR(ForEachTupleFunctorT);

            public:
                /////////////////////////////////////////////////////////
                // Constructor. Note that we always pass "functor" by a
                // "std::forward" reference so "functor" resolves to "&"
                // or "&&" accordingly
                /////////////////////////////////////////////////////////
                constexpr ProcessTupleType(ForEachTupleFunctorT &&functor) noexcept
                    ///////////////////////////////////////////////////////////
                    // "std::forward" required when "ForEachTupleFunctorT" is
                    // "&&". Both "functor" and "m_Functor" are therefore "&&"
                    // but "functor" is still an lvalue because it's named
                    // (rvalue references that are named are lvalues!).
                    // Therefore illegal to bind an lvalue (functor) to an
                    // rvalue reference (m_Functor) so "std::forward" is
                    // required to circumvent this (for the "&&" case it
                    // returns an "&&" reference but it's now unnamed so an
                    // rvalue and therefore suitable for initializing
                    // "m_Functor")
                    ///////////////////////////////////////////////////////////
                    : m_Functor(std::forward<ForEachTupleFunctorT>(functor))
                {
                }

                template <std::size_t I>
                constexpr bool operator()() const
                {
                    using TupleElement_t = std::tuple_element_t<I, TupleT>;

                    //////////////////////////////////////////////////////////
                    // Note: Call to "std::forward()" here required to:
                    //
                    //    1) Perfect forward "m_Functor" back to "&" or "&&"
                    //       accordingly
                    //    2) In the "&&" case, invoke "operator()" in the
                    //       context of an rvalue (in particular, can't do
                    //       this on "m_Functor" directly, without invoking
                    //       "std::forward", since "m_Functor" is an lvalue
                    //       so the lvalue version of "operator()" would kick
                    //       in in the following call instead!!)
                    //////////////////////////////////////////////////////////
                    return std::forward<ForEachTupleFunctorT>(m_Functor).template operator()<I, TupleElement_t>();
                }

            private:
                /////////////////////////////////////////////////////
                // Note: Resolves to "&" in the lvalue case or "&&"
                // otherwise (by the usual C++ reference collapsing
                // rules - "ForEachTupleFunctorT" is always "T &"
                // in the former case or either plain "T" or "T &&"
                // otherwise)
                /////////////////////////////////////////////////////
                ForEachTupleFunctorT &&m_Functor;
            };
        } // namespace Private
    #endif // #if CPP17_OR_EARLIER

    /////////////////////////////////////////////////////////////////////////
    // ForEachTupleType(). Iterates all tuple elements in "TupleT" where
    // "TupleT" is a "std::tuple" specialization, and invokes the given
    // "functor" for each. The effect of this function is that all types in
    // "TupleT" are iterated or none if "TupleT" is empty. For each type in
    // "TupleT", the given "functor" is invoked as noted. "functor" must be
    // a functor with the following functor (template) signature (where
    // items in square brackets are optional - note that an invalid functor
    // will trigger the FOR_EACH_TUPLE_FUNCTOR_C concept in C++20 or later,
    // or a "static_assert" in C++17, since we don't support earlier
    // versions - compile fails either way)
    //
    //     template <std::size_t I, typename TupleElementT>
    //     bool operator()() [const] [volatile] [ref-qualifier] [noexcept];
    //
    // Note that lambdas with this signature are also (therefore) supported
    // in C++20 or later (since lambda templates aren't supported in earlier
    // versions of C++ so you'll have to roll your own functor template if
    // targeting C++17). Note that free functions or other non-static member
    // functions besides "operator()" are not currently supported (but may
    // be in a future release):
    //
    //     Example
    //     -------
    //     ////////////////////////////////////////////////////////
    //     // Some tuple whose types you wish to iterate (display
    //     // in this example - see "displayTupleType" lambda
    //     // below).
    //     ////////////////////////////////////////////////////////
    //     std::tuple<int, float, double> someTuple;
    //
    //     /////////////////////////////////////////////////////////////////
    //     // Lambda that will be invoked just below, once for each type in
    //     // "someTuple()" above (where template arg "I" is the zero-based
    //     // "Ith" index of the type in "someTuple", and "TupleElementT" is
    //     // its type). Note that lambda templates are supported in C++20
    //     // and later only. For C++17 (this header doesn't support earlier
    //     // versions), you need to roll your own functor instead (with a
    //     // template version of "operator()" equivalent to this lambda)
    //     /////////////////////////////////////////////////////////////////
    //     const auto displayTupleType = []<std::size_t I, typename TupleElementT>()
    //                                   {
    //                                       /////////////////////////////////////////////////////////
    //                                       // Display the type of the (zero-based) "Ith" type in
    //                                       // "someTuple" using the following format (shown here
    //                                       // for "I" equals zero so "TupleElementT" is therefore
    //                                       // an "int", but whatever "I" and "TupleElementT" are
    //                                       // on each iteration - note that we increment zero-based
    //                                       // "I" by 1 however to display it as 1-based since it's
    //                                       // more natural for display purposes):
    //                                       //
    //                                       //    1) int
    //                                       //
    //                                       // Note that we never come through here if the tuple
    //                                       // represented by "someTuple" is empty.
    //                                       /////////////////////////////////////////////////////////
    //                                       tcout << (I + 1) << _T(") ") << TypeName_v<TupleElementT> << _T("\n");
    //
    //                                       //////////////////////////////////////////////
    //                                       // Return true to continue iterating (false
    //                                       // would stop iterating, equivalent to a
    //                                       // "break" statement in a regular "for" loop)
    //                                       //////////////////////////////////////////////
    //                                       return true;
    //                                   };
    //
    //     ///////////////////////////////////////////////////////////////
    //     // Iterate all tuple elements in "someTuple" meaning once for
    //     // each type in "someTuple()" above. Invokes "displayTupleType"
    //     // above for each. Outputs the following:
    //     //
    //     //     1) int
    //     //     2) float
    //     //     3) double
    //     //
    //     // Note that "ForEachTupleType()" returns true if the functor
    //     // you pass ("displayTupleType" in this example) returns true
    //     // or false otherwise (useful if your functor needs to "break"
    //     // like a normal "for" loop for any reason - iteration
    //     // immediately stops if it returns false and the following
    //     // therefore returns false). Always returns true in the above
    //     // example (and typically in most real-world code as well)
    //     ///////////////////////////////////////////////////////////////
    //     ForEachTupleType<decltype(someTuple)>(displayTupleType);
    /////////////////////////////////////////////////////////////////////////
    template <TUPLE_C TupleT,
              FOR_EACH_TUPLE_FUNCTOR_C ForEachTupleFunctorT>
    inline constexpr bool ForEachTupleType(ForEachTupleFunctorT&& functor)
    {
        // See this constant for details
        #if !defined(USE_CONCEPTS)
            ///////////////////////////////////////////////////
            // Kicks in if concepts not supported, otherwise
            // TUPLE_C and FOR_EACH_TUPLE_FUNCTOR_C concepts
            // kick in in the template declaration above
            // instead (both simply resolve to the "typename"
            // keyword when concepts aren't supported)
            ///////////////////////////////////////////////////
            STATIC_ASSERT_IS_TUPLE(TupleT)
            STATIC_ASSERT_IS_FOR_EACH_TUPLE_FUNCTOR(ForEachTupleFunctorT)
        #endif

        /////////////////////////////////////////////////////////////////
        // IMPORTANT
        // ---------
        // (Note: You need not read this (very) long explanation since
        // we've correctly worked around it as described below, but I've
        // included it here for posterity).
        //
        // The following call checks for zero as seen (true only when
        // "TupleT" is empty), in order to completely bypass the call to
        // "ForEach()" seen further below (when "TupleT" isn't empty).
        // There's no need to call "ForEach()" further below if the
        // tuple is empty since there's nothing to iterate. Note however
        // that the latter call to "ForEach()" itself will also handle
        // an empty tuple, since it will immediately return true if its
        // "N" template arg is zero (0). The following check for an
        // empty tuple would therefore seem to be redundant
        // (unnecessary), since "ForEach()" also correctly deals with
        // the situation. However, there's a subtle issue at play here
        // that requires us not to call "ForEach()" when the tuple is in
        // fact empty. Doing so would mean that the "operator()"
        // template member of the "processTupleType" functor we're
        // passing to "ForEach" below will be specialized on zero (its
        // "std::size_t" template arg will be zero), even though it will
        // never actually be called by "ForEach()" (since the tuple is
        // empty). "operator()" is still specialized on zero however
        // because when "ForEach()" is stamped out on the template args
        // we pass it, "IsForEachFunctor_v" is called to check if the
        // 2nd template arg of "ForEach()" is a valid functor type. This
        // occurs via a "static_assert" in C++17 or a concept in C++20
        // or later, but in either case "IsForEachFunctor_v" is called
        // and the implementation for this stamps out a copy of the
        // functor's "operator()" template arg, again, doing so with
        // template arg zero (0). "operator()" isn't actually called at
        // this time however, only stamped out (specialized on template
        // arg zero), simply to make sure the functor type being passed
        // is valid. If not then "IsForEachFunctor_v" will return false
        // and the "static_assert" in C++17 or concept in C++20 or later
        // will correctly trap the situation at compile time (i.e., an
        // invalid type is being passed for the 2nd template arg of
        // "ForEach"). However, if the 1st template arg of "ForEach",
        // namely "N" (the number of iterations it will be called for),
        // is zero itself, then although the functor's "operator()"
        // template member will never be called since "N" is zero as
        // noted (so there will be no iterations), "IsForEachFunctor_v"
        // itself will still stamp out "operator()" with its
        // "std::size_t" template arg set to zero (0) as just described.
        // Note that "IsForEachFunctor_v" has no choice but to stamp out
        // "operator()" since there's no other way in C++ for it do its
        // job at this writing (check if a type is a functor with a
        // template member, in this case "operator()" with a
        // "std::size_t" template arg).
        //
        // There's a problem with this situation however. Our
        // (identical) implementation of "operator()" seen in both the
        // lambda below (for C++20 and later), or in class
        // "Private::ProcessTupleType" (C++17 only), both call
        // "std::tuple_element_t<I, TupleT>", but if the "TupleT"
        // template are we're passing to this is empty, then it's
        // illegal to call it for any index including zero (since the
        // tuple is empty so even index zero is out of bounds). Template
        // "I" in this call (zero) is therefore an invalid index so
        // compilation will therefore fail (but not in C++17 for reasons
        // unknown - read on). The upshot is that when dealing with an
        // empty tuple, i.e., the "TupleT" template arg of the function
        // you're now reading is empty, we can't call "ForEach()"
        // because even though it won't iterate the functor it's being
        // passed (since the "N" template arg of "ForEach()" will be
        // zero in this case), the call to "IsForEachFunctor_v" that
        // occurs merely by stamping out "ForEach()" will cause the call
        // to "std::tuple_element_t<I, TupleT>" in our functor to choke
        // during compilation (since it's stamped out with its "I"
        // template arg set to zero but the "TupleT" template arg is
        // empty so "I" is an illegal index). Again, this occurs even
        // though the code is never actually called, but it's illegal
        // even to stamp it out in this case.
        //
        // Please note that as previously mentioned, this situation only
        // occurs in C++20 or later. In this case the lambda seen in the
        // code below would trigger the problem without explicit action
        // to circumvent it (which the following "if constexpr" does),
        // but in C++17 the problem doesn't occur. In C++17 the same
        // code seen in the lambda below exists but in
        // "Private::ProcessTupleType::operator()" instead (the lambda
        // below targets C++20 or later only), but for reasons unknown
        // (I haven't investigated), in C++17 no compilation error
        // occurs on the call to "std::tuple_element_t<I, TupleT>" (as
        // described). It's likely (presumably) not being stamped out
        // in this case. Testing shows however that if we allowed the
        // same code to be compiled in C++20, instead of the lambda
        // below (that we actually rely on in C++20 or later), it will
        // also fail. IOW, this whole situation surrounding
        // "std::tuple_element_t<I, TupleT>" only starts surfacing in
        // C++20 so something clearly has changed between C++17 and
        // C++20 (since what compiled without error in C++17 now fails
        // in C++20). This should be investigated but it's a moot point
        // for now since the following "if constexpr" statement prevents
        // the issue altogether.
        /////////////////////////////////////////////////////////////////
        if constexpr (std::tuple_size_v<TupleT> != 0)
        {
            // Lambda templates not supported until C++20
            #if CPP20_OR_LATER
                ////////////////////////////////////////////////////////
                // Lambda template we'll be calling once for each type
                // in template arg "TupleT" (equivalent to the
                // "Private::ProcessTupleType" functor in C++17 or
                // earlier below). Each iteration invokes "functor",
                // where processing stops after all types in "TupleT"
                // have been processed or "functor" returns false,
                // whichever comes first  (false only returned if
                // "functor" wants to break like a normal "for" loop,
                // which rarely happens in practice so we usually
                // iterate all tuple types in "TupleT")
                //
                // IMPORTANT:
                // ---------
                // See the IMPORTANT comments about the capture of
                // "functor" in function template "ForEach()" (in the
                // comments preceding its own lambda template). The
                // same situation described there applies here as well.
                ////////////////////////////////////////////////////////
                const auto processTupleType = [&functor]<std::size_t I>()
                                              {
                                                  using TupleElement_t = std::tuple_element_t<I, TupleT>;

                                                  /////////////////////////////////////////////////////
                                                  // IMPORTANT:
                                                  // ---------
                                                  // See the IMPORTANT comments inside the lambda of
                                                  // function template "ForEach()" (just before its
                                                  // own call to "std::forward()"). The same situation
                                                  // described there applies here as well.
                                                  /////////////////////////////////////////////////////
                                                  return std::forward<ForEachTupleFunctorT>(functor).template operator()<I, TupleElement_t>();
                                              };
            #else // C++17 (using our own functor instead, equivalent to above lambda) ...
                ////////////////////////////////////////////////////////////
                // Create an instance of "Private::ProcessTupleType", a
                // functor we'll be calling once for each type in template
                // arg "TupleT" (equivalent to the lambda in C++20 or later
                // above). Each iteration invokes "functor", where
                // processing stops after all types in "TupleT" have been
                // processed or "functor" returns false, whichever comes
                // first (false only returned if "functor" wants to break
                // like a normal "for" loop, which rarely happens in
                // practice so we usually iterate all types in "TupleT")
                //
                // Note: The "ForEachTupleFunctorT" template arg we're
                // passing here as the 2nd template arg of class of
                // "Private::ProcessTupleType" means that the latter's
                // constructor will take the same arg type as "functor"
                // itself (& or && as usual). We then call "std::forward"
                // as seen to pass "functor" to the constructor. Note that
                // "std::forward" is mandatory here because "functor" is an
                // lvalue even in the && case (since it's named - named
                // rvalue references are still lvalues). Attempting to pass
                // "functor" directly would therefore cause a compiler
                // error since you can't bind an lvalue (functor) to an
                // rvalue reference (the arg type that the
                // "Private::ProcessTupleType" constructor is expecting in
                // this case).
                ////////////////////////////////////////////////////////////
                const Private::ProcessTupleType<TupleT, ForEachTupleFunctorT> processTupleType(std::forward<ForEachTupleFunctorT>(functor));
            #endif

            ///////////////////////////////////////////////////////////////
            // Defer to this function template further above, passing the
            // number of elements in the tuple as the function's template
            // arg, and "processTupleType" as the function's functor arg.
            // The "ForEachFunctorT" template arg of "ForEach()" is
            // therefore deduced to be the type of "processTupleType".
            // "ForEach()" will then invoke this functor once for each
            // element in the tuple given by "TupleT".
            ///////////////////////////////////////////////////////////////
            return ForEach<std::tuple_size_v<TupleT>>(processTupleType);
        }
        else
        {
            return true;
        }
    }

    /////////////////////////////////////////////////////////////////////////
    // ForEachFunctionTraitsArg(). Iterates all tuple elements in member
    // "FunctionTraitsT::ArgTypes", where "FunctionTraitsT" is a
    // "FunctionTraits" specialization, and invokes the given "functor"
    // for each. Also see helper function "ForEachArg()" just below which
    // just defers to the function you're now reading (and easier to use
    // so you should normally use it instead - the function you're now
    // reading should normally be used only if you already have an
    // existing "FunctionTraits" you're working with, otherwise
    // "ForEachArg()" creates a "FunctionTraits" for you so it's less
    // verbose than calling the following function directly).
    //
    // The effect of this function is that all (non-variadic) args in the
    // function that the given "FunctionTraits" was specialized on are
    // iterated or none if that function has no args. For each argument
    // type, the given "functor" is invoked as noted. "functor" must be a
    // functor with the following functor (template) signature (where
    // items in square brackets are optional - note that an invalid
    // functor will trigger the FOR_EACH_TUPLE_FUNCTOR_C concept in C++20
    // or later, or a "static_assert" in C++17, since we don't support
    // earlier versions - compile fails either way):
    //
    //     template <std::size_t I, typename ArgTypeT>
    //     bool operator()() [const] [volatile] [ref-qualifier] [noexcept];
    //
    // Note that lambdas with this signature are also (therefore)
    // supported in C++20 or later (since lambda templates aren't
    // supported in earlier versions of C++ so you'll have to roll your
    // own functor template if targeting C++17). Note that free functions
    // or other non-static member functions besides "operator()" are not
    // currently supported (but may be in a future release):
    //
    //
    //     Example
    //     -------
    //     ////////////////////////////////////////////////////
    //     // Prototype of some function whose arg types you
    //     // wish to iterate (display in this example - see
    //     // "displayArgType" lambda below). Note that we're
    //     // using a raw C++ function type in this example
    //     // (just a free-function prototype), but pointers
    //     // and references to functions are also supported
    //     // (though references to non-static member functions
    //     // are illegal in C++), as well as references to
    //     // pointers, and non-overloaded functors (or
    //     // references to functors)
    //     ////////////////////////////////////////////////////
    //     int SomeFunc(int arg1, float arg2, double arg3);
    //
    //     ///////////////////////////////////////////////////////////////
    //     // "FunctionTraits" specialization for the above function.
    //     // Note that you can pass any function type for its template
    //     // arg (free functions, member functions, pointers to
    //     // functions, references to functions, references to pointers
    //     // to functions and non-overloaded functors). The resulting
    //     // "FunctionTraits" specialization therefore captures all
    //     // (most) info about whatever function you pass (its return
    //     // type, argument types and other info). See "FunctionTraits"
    //     // for details (we'll be iterating its "ArgTypes" member
    //     // below).
    //     ///////////////////////////////////////////////////////////////
    //     using SomeFuncTraits = FunctionTraits<decltype(SomeFunc)>;
    //
    //     /////////////////////////////////////////////////////////////////
    //     // Lambda that will be invoked just below, once for each arg in
    //     // "SomeFunc()" above (where template arg "I" is the zero-based
    //     // "Ith" argument of "SomeFunc()", and "ArgTypeT" is its type).
    //     // Note that lambda templates are supported in C++20 and later
    //     // only. For C++17 (this header doesn't support earlier versions),
    //     // you need to roll your own functor instead (with a template
    //     // version of "operator()" equivalent to this lambda)
    //     /////////////////////////////////////////////////////////////////
    //     const auto displayArgType = []<std::size_t I, typename ArgTypeT>()
    //                                 {
    //                                     /////////////////////////////////////////////////////////
    //                                     // Display the type of the (zero-based) "Ith" arg in
    //                                     // "SomeFunc" using the following format (shown here for
    //                                     // "I" equals zero so "ArgTypeT" is therefore an "int",
    //                                     // but whatever "I" and "ArgTypeT" are on each iteration
    //                                     // - note that we increment zero-based "I" by 1 however
    //                                     // to display it as 1-based since it's more natural for
    //                                     // display purposes):
    //                                     //
    //                                     //    1) int
    //                                     //
    //                                     // Note that we never come through here if the function
    //                                     // represented by "FunctionTraitsT" has no "args (or
    //                                     // variadic args only - variadic args aren't processed)
    //                                     /////////////////////////////////////////////////////////
    //                                     tcout << (I + 1) << _T(") ") << TypeName_v<ArgTypeT> << _T("\n");
    //
    //                                     //////////////////////////////////////////////
    //         // Return true to continue iterating (false
    //         // would stop iterating, equivalent to a
    //         // "break" statement in a regular "for" loop)
    //         //////////////////////////////////////////////
    //         return true;
    //     };
    //
    //     /////////////////////////////////////////////////////////////
    //     // Iterate all tuple elements in "SomeFuncTraits::ArgTypes"
    //     // meaning once for each argument in "SomeFunc()" above.
    //     // Invokes "displayArgType" above for each. Outputs the
    //     // following:
    //     //
    //     //     1) int
    //     //     2) float
    //     //     3) double
    //     //
    //     // Note that "ForEachFunctionTraitsArg()" returns true if
    //     // the functor you pass ("displayArgType" in this example)
    //     // returns true or false otherwise (useful if your functor
    //     // needs to "break" like a normal "for" loop for any
    //     // reason - iteration immediately stops if it returns false
    //     // and the following therefore returns false). Always
    //     // returns true in the above example (and typically in most
    //     // real-world code as well)
    //     /////////////////////////////////////////////////////////////
    //     ForEachFunctionTraitsArg<SomeFuncTraits>(displayArgType);
    /////////////////////////////////////////////////////////////////////////
    template <FUNCTION_TRAITS_C FunctionTraitsT,
              FOR_EACH_TUPLE_FUNCTOR_C ForEachTupleFunctorT>
    inline constexpr bool ForEachFunctionTraitsArg(ForEachTupleFunctorT&& functor)
    {
        // See this constant for details
        #if !defined(USE_CONCEPTS)
            ///////////////////////////////////////////////////
            // Kicks in if concepts not supported, otherwise
            // FUNCTION_TRAITS_C and FOR_EACH_TUPLE_FUNCTOR_C
            // concepts kick in in the template declaration
            // above instead (both simply resolve to the
            // "typename" keyword when concepts aren't
            // supported)
            ///////////////////////////////////////////////////
            STATIC_ASSERT_IS_FUNCTION_TRAITS(FunctionTraitsT);
            STATIC_ASSERT_IS_FOR_EACH_TUPLE_FUNCTOR(ForEachTupleFunctorT)
        #endif

        /////////////////////////////////////////////////////////
        // Defer to function template "ForEachTupleType()" just
        // above, specializing it on the tuple type member of
        // "FunctionTraitsT" (i.e., the "std::tuple" containing
        // all non-variadic arg types for the function
        // represented by "FunctionTraitsT"). All tuple types
        // (i.e., all arg types in the "FunctionTraitsT"
        // function) are therefore iterated, invoking "functor"
        // for each (we pass the latter).
        /////////////////////////////////////////////////////////
        return ForEachTupleType<FunctionTraitsArgTypes_t<FunctionTraitsT>>(std::forward<ForEachTupleFunctorT>(functor));
    }

    ///////////////////////////////////////////////////////////////////
    // ForEachArg(). Thin wrapper around "ForEachFunctionTraitsArg()"
    // just above. Simply creates a "FunctionTraits" from the function
    // given by template arg "F" and then invokes the latter function.
    // Therefore iterates all tuple elements corresponding to each
    // (non-variadic) arg in function "F" (or none if "F" has no
    // non-variadic args) and invokes "functor" for each. See above
    // function for further details.
    //
    // Note that you should normally rely on the following function
    // instead of "ForEachFunctionTraitsArg()" above since it's easier
    // (though you're free to use the latter function if you've already
    // created a "FunctionTraits" for other purposes). The following
    // provides the same example seen in "ForEachFunctionTraitsArg()"
    // to demonstrate this.
    //
    //     Example
    //     -------
    //     ///////////////////////////////////////////////////
    //     // Prototype of some function whose arg types you
    //     // wish to iterate (display in this example - see
    //     // "displayArgType" lambda below). Note that we're
    //     // using a raw C++ function type in this example
    //     // (just a free-function prototype), but pointers
    //     // and references to functions are also supported
    //     // (though references to non-static member functions
    //     // are illegal in C++), as well as references to
    //     // pointers, and non-overloaded functors (or
    //     // references to functors)
    //     ///////////////////////////////////////////////////
    //     int SomeFunc(int arg1, float arg2, double arg3);
    //
    //     /////////////////////////////////////////////////////////////////
    //     // Lambda that will be invoked just below, once for each arg in
    //     // "SomeFunc()" above (where template arg "I" is the zero-based
    //     // "Ith" argument of "SomeFunc()", and "ArgTypeT" is its type).
    //     // Note that lambda templates are supported in C++20 and later
    //     // only. For C++17 (this header doesn't support earlier versions),
    //     // you need to roll your own functor instead (with a template
    //     // version of "operator()" equivalent to this lambda)
    //     /////////////////////////////////////////////////////////////////
    //     const auto displayArgType = []<std::size_t I, typename ArgTypeT>()
    //                                 {
    //                                     //////////////////////////////////////////////////////
    //                                     // Display the type of the (zero-based) "Ith" arg in
    //                                     // "SomeFunc" using the following format (shown here
    //                                     // for "I" equals zero so "ArgTypeT" is therefore an
    //                                     // "int", but whatever "I" and "ArgTypeT" are on each
    //                                     // iteration - note that we increment zero-based "I"
    //                                     // by 1 however to display it as 1-based since it's
    //                                     // more natural for display purposes):
    //                                     //
    //                                     //    1) int
    //                                     //
    //                                     // Note that we never come through here if function
    //                                     // "F" has no args (or variadic args only - variadic
    //                                     // args aren't processed)
    //                                     //////////////////////////////////////////////////////
    //                                     tcout << (I + 1) << _T(") ") << TypeName_v<ArgTypeT> << _T("\n");
    //
    //                                     //////////////////////////////////////////////
    //                                     // Return true to continue iterating (false
    //                                     // would stop iterating, equivalent to a
    //                                     // "break" statement in a regular "for" loop)
    //                                     //////////////////////////////////////////////
    //                                     return true;
    //                                 };
    //
    //     ///////////////////////////////////////////////////////////////
    //     // Iterate all argument types in "SomeFunc", invoking
    //     // "displayArgType" above for each. Outputs the following:
    //     //
    //     //     1) int
    //     //     2) float
    //     //     3) double
    //     //
    //     // Note that "ForEachArg" returns true if the functor you pass
    //     // ("displayArgType" in this example) returns true or false
    //     // otherwise (useful if your functor needs to "break" like a
    //     // normal "for" loop for some reason - iteration immediately
    //     // stops if it returns false and the following therefore
    //     // returns false). Always returns true in the above example
    //     // (and typically in most real-world code as well)
    //     ///////////////////////////////////////////////////////////////
    //     using F = decltype(SomeFunc);
    //     ForEachArg<F>(displayArgType);
    ///////////////////////////////////////////////////////////////////
    template <TRAITS_FUNCTION_C F,
              FOR_EACH_TUPLE_FUNCTOR_C ForEachTupleFunctorT>
    inline constexpr bool ForEachArg(ForEachTupleFunctorT&& functor)
    {
        // See this constant for details
        #if !defined(USE_CONCEPTS)
            ///////////////////////////////////////////////////
            // Kicks in if concepts not supported, otherwise
            // TRAITS_FUNCTION_C and FOR_EACH_TUPLE_FUNCTOR_C
            // concepts kick in in the template declaration
            // above instead (both simply resolve to the
            // "typename" keyword when concepts aren't
            // supported)
            ///////////////////////////////////////////////////
            STATIC_ASSERT_IS_TRAITS_FUNCTION(F);
            STATIC_ASSERT_IS_FOR_EACH_TUPLE_FUNCTOR(ForEachTupleFunctorT)
        #endif

        ///////////////////////////////////////////////////////////
        // Create (specialize) a "FunctionTraits" from "F" and
        // defer to function template "ForEachFunctionTraitsArg()"
        // just above (the "FunctionTraits" version of this
        // function). All non-variadic arg types for the function
        // represented by "FunctionTraitsT" are therefore iterated,
        // invoking "functor" for each (we pass the latter).
        ///////////////////////////////////////////////////////////
        return ForEachFunctionTraitsArg<FunctionTraits<F>>(std::forward<ForEachTupleFunctorT>(functor));
    }

    ///////////////////////////////////////////////////////////////////////
    // Private namespace (for internal use only) used to implement
    // function template "DisplayAllFunctionTraits()" declared just after
    // this namespace. Allows you to display (stream) all traits for a
    // given function to any stream. See "DisplayAllFunctionTraits()"
    // just after this namespace for full details (it just defers to
    // "DisplayAllFunctionTraitsImpl::Process()" in the following
    // namespace).
    ///////////////////////////////////////////////////////////////////////
    namespace Private
    {
        template <typename CharT, typename CharTraitsT>
        class DisplayAllFunctionTraitsImpl
        {
        public:
            // Constructor
            DisplayAllFunctionTraitsImpl(std::basic_ostream<CharT, CharTraitsT>& stream) noexcept
                : m_Stream(stream)
            {
            }

            ////////////////////////////////////////////////////////////////////
            // Implementation function for "StdExt::DisplayAllFunctionTraits()"
            // declared just after this class. See that function for details
            // (it just defers to us).
            ////////////////////////////////////////////////////////////////////
            template <typename F>
            std::basic_ostream<CharT, CharTraitsT>& Process() const
            {
                /////////////////////////////////////////////////////////////
                // Lambda for displaying the "#)" seen at the start of each
                // output line (where "#" is the argument # starting at 1),
                // but not the "#)" seen for each arg in the "Arguments"
                // section of the output, which are later handled by a call
                // to (member function) "StreamArgTypes()".
                /////////////////////////////////////////////////////////////
                auto streamNumBracket = [this, num = 1]() mutable -> auto&
                {
                    return this->m_Stream << num++ << _T(") ");
                };

                ///////////////////////////////
                // #) Type
                ///////////////////////////////
                streamNumBracket() << _T("Type: ") << FunctionTypeName_v<F> << _T("\n");

                ///////////////////////////////
                // #) Classification
                ///////////////////////////////
                streamNumBracket() << _T("Classification: ");

                // Non-static member function? (includes functors)
                if (IsMemberFunction_v<F>)
                {
                    m_Stream << (IsFunctor_v<F> ? _T("Functor") : _T("Non-static member"));
                    m_Stream << _T(" (class/struct=\"") << MemberFunctionClassName_v<F> << _T("\")\n");
                }
                // Free function (which includes static member functions)
                else
                {
                    m_Stream << _T("Free or static member\n");
                }

                ///////////////////////////////
                // #) Calling convention
                ///////////////////////////////
                streamNumBracket() << _T("Calling convention: ") << CallingConventionName_v<F> << _T("\n");

                ///////////////////////////////
                // #) Return
                ///////////////////////////////
                streamNumBracket() << _T("Return: ") << ReturnTypeName_v<F> << _T("\n");

                ///////////////////////////////
                // #) Arguments
                ///////////////////////////////
                streamNumBracket() << _T("Arguments (") <<
                                      ArgCount_v<F> <<
                                      (IsVariadic_v<F> ? _T(" + variadic") : _T("")) <<
                                      _T("):");

                //////////////////////////////////////////////
                // Function's arg list not empty, i.e., not
                // "Func()" or (equivalently) "Func(void)"?
                // (so it has at least 1 non-variadic arg
                // and/or is variadic)
                //////////////////////////////////////////////
                if (!IsEmptyArgList_v<F>)
                {
                    m_Stream << _T("\n");
                    
                    //////////////////////////////////////////////
                    // Stream all args as per this example (note
                    // that a tab character precedes each number
                    // and a linefeed ("\n") follows the last
                    // line - the "..." line only exists if the
                    // function is variadic):
                    //
                    //    1) const std::basic_string<wchar_t> &
                    //    2) const char*
                    //    3) short
                    //    4) ...
                    //////////////////////////////////////////////
                    StreamArgTypes<F>();
                }
                else
                {
                    ////////////////////////////////////////
                    // Function's arg list is "(void)" (it
                    // has no args and is not variadic)
                    ////////////////////////////////////////
                    m_Stream << _T(" None\n");
                }

                ///////////////////////////////////////////////////
                // Is a non-static member function which includes
                // functors. If so then display traits applicable
                // to those only (i.e., cv and ref qualifiers
                // which aren't applicable to free functions or
                // static member functions)
                ///////////////////////////////////////////////////
                if (IsMemberFunction_v<F>)
                {
                    ///////////////////////////////
                    // #) const
                    ///////////////////////////////
                    streamNumBracket() << _T("const: ") << std::boolalpha << IsMemberFunctionConst_v<F> << _T("\n");

                    ///////////////////////////////
                    // #) volatile
                    ///////////////////////////////
                    streamNumBracket() << _T("volatile: ") << std::boolalpha << IsMemberFunctionVolatile_v<F> << _T("\n");

                    ///////////////////////////////
                    // #) Ref-qualifier
                    ///////////////////////////////
                    streamNumBracket() << _T("Ref-qualifier: ") << MemberFunctionRefQualifierName_v<F> << _T("\n");
                }

                ///////////////////////////////
                // #) noexcept
                ///////////////////////////////
                streamNumBracket() << _T("noexcept: ") << std::boolalpha << IsNoexcept_v<F>;

                return m_Stream;
            }

        private:
            ///////////////////////////////////////////////////////////////
            // OutputArgI(). Streams the "Ith" arg to "m_Stream" (formatted
            // as seen below). Note that "I" refers to the (zero-based)
            // "Ith" arg in the function we're being called for (in
            // left-to-right order), and "argTypeAsStr" is its type
            // (converted to a WYSIWYG string).
            ///////////////////////////////////////////////////////////////
            void OutputArgI(const std::size_t i, // "Ith" arg (zero-based)
                            tstring_view argTypeAsStr) const // Type of the "Ith" arg just above (as a string)
            {
                ////////////////////////////////////////
                // E.g., (using zero-based i = 2 as an
                // example, i.e., the 3rd arg of the
                // function we're being called for)
                // 
                //     "\t3) double\n"
                ////////////////////////////////////////
                m_Stream << _T("\t") <<
                            (i + 1) << // "Ith" arg ("i" is zero-based so we add 1 to display
                                        // it as 1-based - more natural for display purposes)
                            _T(") ") <<
                            argTypeAsStr <<
                            _T("\n");
            }

            /////////////////////////////////////////////////////////////////
            // OutputArgI(). Converts "ArgTypeT" template arg to a "pretty"
            // (WYSIWYG) string and passes it to above (non-template)
            // overload where it's streamed to "m_Stream" (formatted as
            // seen in the above overload). Note that "I" refers to the
            // (zero-based) "Ith" arg in the function we're being called for
            // (in left-to-right order) and "ArgTypeT" is its type.
            /////////////////////////////////////////////////////////////////
            template <std::size_t I, typename ArgTypeT>
            void OutputArgI() const // "Ith" arg (zero-based)
            {
                ////////////////////////////////////////
                // Convert "ArgTypeT" to a string (its
                // user-friendly name) and pass it to
                // the overload just above
                ////////////////////////////////////////
                OutputArgI(I, TypeName_v<ArgTypeT>);
            }

            ////////////////////////////////////////////////////////////
            // C++17? Note that we don't support earlier versions and
            // everything would have already been preprocessed out if
            // compiling for versions prior to C++17 (see check for
            // CPP17_OR_LATER near to top of file). For C++20 or later
            // however we replace the following class (StreamArgType)
            // with its lambda template equivalent at point of use
            // later on (in "StreamArgTypes()"). Both just invoke
            // "OutputArgI()" above so are identical but the lambda is
            // more convenient.
            ////////////////////////////////////////////////////////////
            #if CPP17
                class StreamArgType
                {
                public:
                    StreamArgType(const DisplayAllFunctionTraitsImpl &displayAllFunctionTraitsImpl) noexcept
                        : m_DisplayAllFunctionTraitsImpl(displayAllFunctionTraitsImpl)
                    {
                    }

                    ///////////////////////////////////////////////////
                    // Functor we'll be invoking once for each arg in
                    // the function whose traits we're displaying
                    // (where "I" is the zero-based number of the arg,
                    // and "ArgTypeT" is its type). Following is a
                    // template so it effectively gets stamped out
                    // once for each argument in the function whose
                    // traits we're displaying. See call to
                    // "ForEachArg()" in "StreamArgTypes()" (which
                    // processes this class in C++ 17 only).
                    ///////////////////////////////////////////////////
                    template <std::size_t I, typename ArgTypeT>
                    bool operator()() const
                    {
                        /////////////////////////////////////////////
                        // Defer to this function (template) which
                        // streams (zero-based) arg "I" (whose type
                        // is "ArgTypeT") to "m_Stream". Both "I"
                        // (actually "I+1" to make it one-based for
                        // display purposes) and the type name of
                        // "ArgType"T are therefore displayed.
                        /////////////////////////////////////////////
                        m_DisplayAllFunctionTraitsImpl.OutputArgI<I, ArgTypeT>();

                        //////////////////////////////////////////////
                        // Return true to continue iterating so this
                        // function gets called again with "I + 1"
                        // (until all args in the function whose
                        // traits are being displayed are processed).
                        // Note that returning false would stop
                        // iterating, equivalent to a "break"
                        // statement in a regular"for" loop.
                        //////////////////////////////////////////////
                        return true;
                    }

                private:
                    const DisplayAllFunctionTraitsImpl& m_DisplayAllFunctionTraitsImpl;
                };
            #endif

            ///////////////////////////////////////////////////////////////////////////
            // "StreamArgTypes()". Displays all arguments for function "F" to
            // "m_Stream" as per the following example:
            //
            // If "F" is a function of the following type:
            //
            //       int (const std::string &, const char *, short, ...) noexcept;
            //
            // Then displays (outputs to "m_Stream") the following (note that the
            // format of the type names below are compiler-dependent but are usually
            // identical or very similar):
            //
            //       1) const std::basic_string<char> &
            //       2) const char *
            //	     3) short
            //	     4) ...
            // 
            // Note that a tab character precedes each line above, and a linefeed
            // ("\n") is output after the final line. Note that if "F" has no args
            // however (including variadic args), then the function does nothing
            // (nothing gets streamed to "m_Stream")
            ///////////////////////////////////////////////////////////////////////////
            template <TRAITS_FUNCTION_C F>
            void StreamArgTypes() const
            {
                // Lambda templates available starting in C++20
                #if CPP20_OR_LATER
                    ///////////////////////////////////////////////////
                    // Lambda we'll be invoking once for each arg in
                    // function "F" (where "I" is the zero-based
                    // number of the arg, and "ArgTypeT" is its type).
                    // Following is a template so it effectively gets
                    // stamped out once for each argument in function
                    // "F" (in the call to "ForEachArg()" below).
                    ///////////////////////////////////////////////////
                    const auto streamArgType = [&]<std::size_t I, typename ArgTypeT>()
                    {
                        /////////////////////////////////////////////
                        // Defer to this function (template) which
                        // streams (zero-based) arg "I" (whose type
                        // is "ArgTypeT") to "m_Stream". Both "I"
                        // (actually "I+1" to make it one-based for
                        // display purposes) and the type name of
                        // "ArgType"T are therefore displayed.
                        /////////////////////////////////////////////
                        OutputArgI<I, ArgTypeT>();

                        ////////////////////////////////////////////////
                        // Return true to continue iterating so this
                        // function gets called again with "I + 1"
                        // (until all args in function "F" are
                        // processed). Note that returning false would
                        // stop iterating, equivalent to a "break"
                        // statement in a regular "for" loop.
                        ////////////////////////////////////////////////
                        return true;
                    };

                    ////////////////////////////////////////////////////
                    // Roll our own functor for C++17 (identical to
                    // lambda above). Note that we don't support C++
                    // versions < C++17 so if we drop through here we
                    // must be targeting C++17 (since CPP17_OR_LATER
                    // was checked at top of file and is now in effect)
                    ////////////////////////////////////////////////////
                #else
                    const StreamArgType streamArgType(*this);
                #endif

                ////////////////////////////////////////////////////////
                // Invoke the above functor ("streamArgType") once for
                // each arg in the function whose traits we're
                // processing ("F"). Note that the following generic
                // function (template) "ForEachArg()" will effectively
                // stamp out one copy of member function (template)
                // "streamArgType.operator()<I, ArgTypeT>" for each
                // argument in function "F". IOW, we wind up with "N"
                // versions of "streamArgType.operator()", where "N" is
                // the number of non-variadic args in function "F" and
                // the template args of each "streamArgType.operator()"
                // reflect the "Ith" arg in function "F" (where "I" is
                // in the range 0 to N - 1)
                ////////////////////////////////////////////////////////
                ForEachArg<F>(streamArgType);

                // Is function variadic? (i.e., last arg is "...")
                if (IsVariadic_v<F>)
                {
                    OutputArgI(ArgCount_v<F>, // i
                               _T("...")); // argTypeAsStr
                }
            }

            std::basic_ostream<CharT, CharTraitsT>& m_Stream;
        }; // class DisplayAllFunctionTraitsImpl
    } // namespace Private

    /////////////////////////////////////////////////////////////////////////
    // DisplayAllFunctionTraits(). Displays (streams) all function traits
    // for function "F" to "stream" and returns "stream". The format of the
    // streamed traits is shown in the examples below. Please note however
    // that the format of the displayed types in these examples can vary
    // slightly depending on the target compiler (since the compiler itself
    // is responsible for them). Other trivial changes to the format not
    // shown below might also occur depending on the function but the
    // general format shown below remains the same.
    //
    //      // Standard C++ headers
    //      #include <iostream>
    //      #include <string>
    //      
    //      ////////////////////////////////////////////////////////
    //      // Only file in this repository you need to explicitly
    //      // #include
    //      ////////////////////////////////////////////////////////
    //      #include "TypeTraits.h"
    //
    //      /////////////////////////////////////////////////////////
    //      // Namespace with sample function whose traits you wish
    //      // to display (all of them)
    //      /////////////////////////////////////////////////////////
    //      namespace Test
    //      {
    //          class SomeClass
    //          {
    //          public:
    //              int DoSomething(const std::basic_string<wchar_t>&,
    //                              const char*,
    //                              short int,
    //                              int,
    //                              float,
    //                              long int,
    //                              double,
    //                              ...) const volatile && noexcept;
    //          };
    //      }
    //      
    //      int main()
    //      {
    //          // Everything in the library declared in this namespace
    //          using namespace StdExt;
    //      
    //          // Above member function whose traits you wish to display
    //          using F = decltype(&Test::SomeClass::DoSomething);
    //      
    //          //////////////////////////////////////////
    //          // Display all traits for "F" to "tcout"
    //          // (or pass any other required stream)
    //          //////////////////////////////////////////
    //          DisplayAllFunctionTraits<F>(tcout);
    //      
    //          return 0;
    //      }
    //
    // This will stream the traits for function "Test::SomeClass::DoSomething()"
    // above to "tcout" in the following format (note that the format may vary
    // slightly depending on "F" and the target compiler):
    //
    //      1) Type: int (Test::SomeClass::*)(const std::basic_string<wchar_t>&, const char*, short int, int, float, long int, double, ...) const volatile && noexcept
    //      2) Classification: Non-static member (class/struct="Test::SomeClass")
    //      3) Calling convention: cdecl
    //      4) Return: int
    //      5) Arguments (7 + variadic):
    //              1) const std::basic_string<wchar_t> &
    //              2) const char*
    //              3) short int
    //              4) int
    //              5) float
    //              6) long int
    //              7) double
    //              8) ...
    //      6) const: true
    //      7) volatile: true
    //      8) Ref-qualifier: &&
    //      9) noexcept: true
    //
    // Note that for free functions (including static member functions),
    // which don't support "const", "volatile" or ref-qualifiers, items 6, 7
    // and 8 above will therefore be removed. Instead, item 9 above,
    // "noexcept", will simply be renumbered. The "Classification" above
    // will also indicate "Free or static member".
    //
    // Lastly, note that if "F" is a functor type (including a lambda type),
    // then the format will be identical to the above except that the
    // "Classification" will now indicate "Functor" instead of "Non-static
    // member", and the type itself in item 1 above will reflect the
    // function type of "F::operator()"
    /////////////////////////////////////////////////////////////////////////
    template <TRAITS_FUNCTION_C F, typename CharT, typename CharTraitsT = std::char_traits<CharT>>
    std::basic_ostream<CharT, CharTraitsT>& DisplayAllFunctionTraits(std::basic_ostream<CharT, CharTraitsT> &stream)
    {
        ////////////////////////////////////////////////
        // Implementation class that does all the work
        // (for internal use only so declared in
        // namespace "Private")
        ////////////////////////////////////////////////
        const Private::DisplayAllFunctionTraitsImpl<CharT, CharTraitsT> displayAllFunctionTraitsImpl(stream);

        return displayAllFunctionTraitsImpl.template Process<F>();
    }
} // namespace StdExt
#else
    // Done with this
    #undef DECLARE_MACROS_ONLY
#endif // #if !defined(DECLARE_MACROS_ONLY)

// Done with this (if currently #defined - for internal use in this file only)
#if defined(MSVC_FROM_VISUAL_STUDIO_2017)
    #undef MSVC_FROM_VISUAL_STUDIO_2017
#endif

#endif // #if CPP17_OR_LATER
#endif // #ifndef TYPETRAITS (#include guard)