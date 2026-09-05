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
// Demo program showing the capabilities of the "FunctionTraits" library
// declared in "TypeTraits.h" (used to retrieve information about any
// function including its return type, argument types, and other information
// - write traits also fully supported but not demo'd in this program - see
// link below for examples). Just run the program to display all available
// traits for the 3 sample functions seen in namespace "TestFuncs" later in
// this file (traits displayed to the stdout). You can update these 3
// functions with any signature whose traits you want to display, and re-run
// the app. No other changes are required. Note however that all 3 functions
// are ultimately passed to function template
// "DisplayAllFunctionTraits<F>()" in "TypeTraits.h" itself, which displays
// all traits for function "F" (see "DisplayAllFunctionTraits" at the link
// below). You can therefore just call this function directly if you wish,
// passing any function type you want for template arg "F" (free functions
// which includes static member functions, non-static member functions,
// pointers and references to functions, references to pointers to
// functions, and functors (including lambdas). Note however that inspecting
// this particular function in "TypeTraits.h" shows some of the major
// features of the library at work, so it's a very good learning exercise
// for your own code (demonstrating what can be accomplished). Moreover,
// "DisplayAllFunctionTraits<F>()" is just a client of the library itself,
// no different than your own code will be, since it simply relies on the
// other templates in the library to carry out its own work. It therefore
// just calls the other templates similar to what you'll do in your own
// code, so you can see an actual client of the library in action (even
// though this particular client happens to belong to the library itself).
// It's also reasonably short for what it does, and no tracing into the
// actual implementation details of the library itself is required (to
// understand how to use the library). You only need to trace into the
// implementation of "DisplayAllFunctionTraits<F>()" itself (not any of the
// library's other templates it calls). Again, because it's just a client of
// the library, it utilizes the other templates the same way you will, so
// you'll find it a useful learning experience for your own code. Note that
// other examples of how to use the library are also available at the link
// below. See the "Usage" section for full details.
//
// Lastly, note that the program defaults to C++20. If you wish to change
// this then add -DCMAKE_CXX_STANDARD=?? to the CMake command line options,
// setting "??" to the target version (which must be 17 or any later version
// of C++ - "FunctionTraits" doesn't support earlier versions). In Compiler
// Explorer, the CMake command line options are normally located near the
// top-left corner of the source code window on the left, just above the
// project name "FunctionTraitsDemo". For further details on the CMake
// command line options for this demo, see the top of the "CMakeLists.txt"
// file in the Compiler Explorer source code window.
//
// For complete details on "FunctionTraits" (fully documented), see
// https://github.com/HexadigmSystems/FunctionTraits
/////////////////////////////////////////////////////////////////////////////

// #included first so we can check CPP17_OR_LATER just below
#include "CompilerVersions.h"

// We only support C++17 or later (stop compiling otherwise)
#if CPP17_OR_LATER

    ////////////////////////////////////////////////////////////
    // "import std" not currently in effect? (C++23 or later).
    // If it is in effect then no need to #include the headers
    // below (they're already available)
    ////////////////////////////////////////////////////////////
    #if !defined(STDEXT_IMPORTED_STD) 
        // Standard C/C++ headers
        #include <cstddef>
        #include <iostream>
        #include <string>
        #include <string_view>
    #endif

    ////////////////////////////////////////////////////////
    // Our headers (only file in this repository you need
    // to explicitly #include)
    ////////////////////////////////////////////////////////
    #include "TypeTraits.h"

    //////////////////////////////////////
    // Everything in "TypeTraits.h" just
    // above declared in this namespace
    //////////////////////////////////////
    using namespace StdExt;

    /////////////////////////////////////////////////////////////////////////////
    // "DisplayFunctionTraits()"
    /////////////////////////////////////////////////////////////////////////////
    template <typename F>
    void DisplayFunctionTraits(tstring_view caption)
    {
        // Display the caption for this function to the stdout
        tcout << _T("************************************\n");
        tcout << _T("* ") << caption << _T("\n");
        tcout << _T("************************************\n");

        /////////////////////////////////////////////////
        // Stream the function's traits to "tcout". The
        // following function from the "FunctionTraits"
        // library itself does all the work. See here
        // for details:
        // 
        //    https://github.com/HexadigmSystems/FunctionTraits#DisplayAllFunctionTraits
        /////////////////////////////////////////////////
        DisplayAllFunctionTraits<F>(tcout);
    }

    //////////////////////////////////////////////////////////////////////////////
    // namespace TestFuncs. Contains 3 functions whose traits this demo displays.
    // Functions need not be defined in any namespace however (for this demo to
    // work). Doing so just to keep things organized (and placed in this
    // namespace for easy access - you can quickly change all functions in this
    // one location). Note that you can experiment with this program by simply
    // changing the signatures of these functions to whatever you wish and
    // re-running the app (but don't change the function names themselves unless
    // you also change them where they're actually used later on, though there's
    // normally no need to - the function signature is all that matters, not the
    // name).
    //////////////////////////////////////////////////////////////////////////////
    namespace TestFuncs
    {
        //////////////////////////////////////////////////////
        // Free function. Note: Need not be defined for this
        // demo to work and it's not (only the function's
        // type matters)
        //////////////////////////////////////////////////////
        int FreeFunc(const std::wstring &, const char *, short, int, float, long, double, ...) noexcept;

        class SomeClass
        {
        public:
            //////////////////////////////////////////////////////
            // Non-static member function (static functions also
            // supported but processed like a free function so
            // the one above is used). Note: Need not be defined
            // for this demo to work and it's not (only the
            // function's type matters)
            //////////////////////////////////////////////////////
            int STDEXT_CC_STDCALL DoSomething(double &, const std::string_view &) const volatile && noexcept;

            ///////////////////////////////////////////////////
            // Adding this to make the class a functor (which
            // are also supported)
            ///////////////////////////////////////////////////        
            std::basic_string<tchar> operator()(std::size_t) const;
        };
    }

    //////////////////////////////////////////////////////////////////////////////
    // DisplayFreeFunctionTraits()
    //////////////////////////////////////////////////////////////////////////////
    void DisplayFreeFunctionTraits()
    {
        ////////////////////////////////////////////////////////////
        // Free function type or static member function type whose
        // traits we wish to display (non-static member functions
        // are handled by "DisplayMemberFunctionTraits()" below).
        // Note that pointers or reference to the function are also
        // supported, as well as references to function pointers.
        // The following is the actual (raw) function type however,
        // neither pointer nor reference, though most developers
        // don't work with the actual function type in code too
        // often (since function names decay to a pointer to the
        // function in most scenarios so pointers to functions are
        // much more common in most code - "FunctionTraits" will
        // handle any legal type that identifies the function
        // however, whether the function's actual (raw) C++ type,
        // or pointers or references to functions, or references
        // to pointers to functions)
        ////////////////////////////////////////////////////////
        using F = decltype(TestFuncs::FreeFunc);
        // using F = decltype(&TestFuncs::FreeFunc); // Will also work (pointer to function and usually
                                                     // more common in practice)
        // using F = decltype(TestFuncs::FreeFunc) &; // Will also work (lvalue reference to function,
                                                      // though rvalue works too - both are rare in
                                                      // practice though, noting that this particular
                                                      // "decltype" syntax is even more rare - usually
                                                      // you'll work with a reference to the function
                                                      // in code instead, such as "int (&)()" for
                                                      // example (reference to a function taking no
                                                      // args and returning an "int"), preferably
                                                      // using an alias to simplify the latter
                                                      // (cryptic) syntax

        DisplayFunctionTraits<F>(_T("Free function traits demo"));
    }

    //////////////////////////////////////////////////////////////////////////////
    // DisplayMemberFunctionTraits()
    //////////////////////////////////////////////////////////////////////////////
    void DisplayMemberFunctionTraits()
    {
        ///////////////////////////////////////////////////////////
        // Non-static member function type whose traits we wish to
        // display. Always a member function pointer (references
        // to member function pointers also supported), unlike the
        // free function version in "DisplayFreeFunctionTraits()"
        // above, which supports the function's actual (raw) C++
        // type as well. We don't support the raw C++ type for
        // non-static member functions however, just member
        // function pointers only (or references to such pointers).
        // To this end, when passing a raw function type, it's
        // always assumed to be a free function (which for our
        // purposes also includes static member functions).
        ///////////////////////////////////////////////////////////
        using F = decltype(&TestFuncs::SomeClass::DoSomething);

        DisplayFunctionTraits<F>(_T("Member function traits demo"));
    }

    //////////////////////////////////////////////////////////////////////////////
    // DisplayFunctorTraits()
    //////////////////////////////////////////////////////////////////////////////
    void DisplayFunctorTraits()
    {
        ///////////////////////////////////////////////////////
        // Functor type whose traits we wish to display. For
        // functors, "F" must always be the class type itself,
        // where the class has a single member function called
        // "operator()" (by definition of a functor). Note
        // that overloads of "operator()" are ambiguous
        // however so aren't supported (a compiler error will
        // result). If the function has overloads of this
        // member (rare), then you should take the address of
        // the overload you're interested in and use that
        // instead (i.e., just a normal non-static member
        // function pointer to that overload). In fact, even
        // for functors with just one "operator()" member
        // (like the following example), "operator()" is still
        // just a member function no different than any other,
        // so internally "FunctionTraits" defers to the member
        // function specialization by simply targeting
        // "&F::operator()"
        ///////////////////////////////////////////////////////
        using F = TestFuncs::SomeClass;

        DisplayFunctionTraits<F>(_T("Functor traits demo"));
    }

    /////////////////////////////////////////////////////////////////////////////
    // main()
    /////////////////////////////////////////////////////////////////////////////
    int main()
    {
        tcout << _T("FunctionTraits demo (detected compiler: ") << GetCompilerName() << _T("). See top of \"Demo.cpp\" for details.\n");
        tcout << _T("For complete details on \"FunctionTraits\" see https://github.com/HexadigmSystems/FunctionTraits\n\n");

        DisplayFreeFunctionTraits();
        tcout << _T("\n\n");
        DisplayMemberFunctionTraits();
        tcout << _T("\n\n");
        DisplayFunctorTraits();

        return 0;
    }
#else
    #error "This program is only supported in C++17 or later (an earlier version was detected). Please set the appropriate compiler option to target C++17 or later and try again (minimum of "-std=c++17" for GCC, Clang and Intel, or "/std:c++17" for Microsoft)"
#endif // #if CPP17_OR_LATER