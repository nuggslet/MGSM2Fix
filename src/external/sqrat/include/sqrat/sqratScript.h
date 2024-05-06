//
// SqratScript: Script Compilation and Execution
//

//
// Copyright (c) 2009 Brandon Jones
//
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
//  1. The origin of this software must not be misrepresented; you must not
//  claim that you wrote the original software. If you use this software
//  in a product, an acknowledgment in the product documentation would be
//  appreciated but is not required.
//
//  2. Altered source versions must be plainly marked as such, and must not be
//  misrepresented as being the original software.
//
//  3. This notice may not be removed or altered from any source
//  distribution.
//

#if !defined(_SCRAT_SCRIPT_H_)
#define _SCRAT_SCRIPT_H_

#include <squirrel.h>
#include <sqstdio.h>
#include <string.h>

#include "sqratObject.h"

namespace Sqrat {

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Helper class for managing Squirrel scripts
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template <Squirk Q>
class Script : public Object<Q> {
public:

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Default constructor
    ///
    /// \param v VM that the Script will be associated with
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    Script(HSQUIRRELVM<Q> v = DefaultVM<Q>::Get()) : Object<Q>(v, true) {
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Sets up the Script using a string containing a Squirrel script
    ///
    /// \param script String containing a file path to a Squirrel script
    /// \param name   Optional string containing the script's name (for errors)
    ///
    /// \remarks
    /// This function MUST have its Error handled if it occurred.
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    void CompileString(const string& script, const string& name = _SC("")) {
        if(!sq_isnull(this->obj)) {
            sq_release(this->vm, &this->obj);
            sq_resetobject(&this->obj);
        }

#if !defined (SCRAT_NO_ERROR_CHECKING)
        if(SQ_FAILED(sq_compilebuffer(this->vm, script.c_str(), static_cast<SQInteger>(script.size() /** sizeof(SQChar)*/), name.c_str(), true))) {
            SQTHROW(this->vm, LastErrorString(this->vm));
            return;
        }
#else
        sq_compilebuffer(this->vm, script.c_str(), static_cast<SQInteger>(script.size() /** sizeof(SQChar)*/), name.c_str(), true);
#endif
        sq_getstackobj(this->vm,-1,&this->obj);
        sq_addref(this->vm, &this->obj);
        sq_pop(this->vm, 1);
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Sets up the Script using a string containing a Squirrel script
    ///
    /// \param script String containing a file path to a Squirrel script
    /// \param errMsg String that is filled with any errors that may occur
    /// \param name   Optional string containing the script's name (for errors)
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    bool CompileString(const string& script, string& errMsg, const string& name = _SC("")) {
        if(!sq_isnull(this->obj)) {
            sq_release(this->vm, &this->obj);
            sq_resetobject(&this->obj);
        }

#if !defined (SCRAT_NO_ERROR_CHECKING)
        if(SQ_FAILED(sq_compilebuffer(this->vm, script.c_str(), static_cast<SQInteger>(script.size() /** sizeof(SQChar)*/), name.c_str(), true))) {
            errMsg = LastErrorString(this->vm);
            return false;
        }
#else
        sq_compilebuffer(this->vm, script.c_str(), static_cast<SQInteger>(script.size() /** sizeof(SQChar)*/), name.c_str(), true);
#endif
        sq_getstackobj(this->vm,-1,&this->obj);
        sq_addref(this->vm, &this->obj);
        sq_pop(this->vm, 1);
        return true;
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Sets up the Script using a file containing a Squirrel script
    ///
    /// \param path File path containing a Squirrel script
    ///
    /// \remarks
    /// This function MUST have its Error handled if it occurred.
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    void CompileFile(const string& path) {
        if(!sq_isnull(this->obj)) {
            sq_release(this->vm, &this->obj);
            sq_resetobject(&this->obj);
        }

#if !defined (SCRAT_NO_ERROR_CHECKING)
        if(SQ_FAILED(sqstd_loadfile(this->vm, path.c_str(), true))) {
            SQTHROW(this->vm, LastErrorString(this->vm));
            return;
        }
#else
        sqstd_loadfile(this->vm, path.c_str(), true);
#endif
        sq_getstackobj(this->vm,-1,&this->obj);
        sq_addref(this->vm, &this->obj);
        sq_pop(this->vm, 1);
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Sets up the Script using a file containing a Squirrel script
    ///
    /// \param path   File path containing a Squirrel script
    /// \param errMsg String that is filled with any errors that may occur
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    bool CompileFile(const string& path, string& errMsg) {
        if(!sq_isnull(this->obj)) {
            sq_release(this->vm, &this->obj);
            sq_resetobject(&this->obj);
        }

#if !defined (SCRAT_NO_ERROR_CHECKING)
        if(SQ_FAILED(sqstd_loadfile(this->vm, path.c_str(), true))) {
            errMsg = LastErrorString(this->vm);
            return false;
        }
#else
        sqstd_loadfile(this->vm, path.c_str(), true);
#endif
        sq_getstackobj(this->vm,-1,&this->obj);
        sq_addref(this->vm, &this->obj);
        sq_pop(this->vm, 1);
        return true;
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Runs the script
    ///
    /// \remarks
    /// This function MUST have its Error handled if it occurred.
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    void Run() {
#if !defined (SCRAT_NO_ERROR_CHECKING)
        if(!sq_isnull(this->obj)) {
            SQRESULT result;
            SQInteger top = sq_gettop(this->vm);
            sq_pushobject(this->vm, this->obj);
            sq_pushroottable(this->vm);
            result = sq_call(this->vm, 1, false, true);
            sq_settop(this->vm, top);
            if(SQ_FAILED(result)) {
                SQTHROW(this->vm, LastErrorString(this->vm));
                return;
            }
        }
#else
        SQInteger top = sq_gettop(this->vm);
        sq_pushobject(this->vm, this->obj);
        sq_pushroottable(this->vm);
        sq_call(this->vm, 1, false, true);
        sq_settop(this->vm, this->top);
#endif
    }

#if !defined (SCRAT_NO_ERROR_CHECKING)
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Runs the script
    ///
    /// \param errMsg String that is filled with any errors that may occur
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    bool Run(string& errMsg) {
        if(!sq_isnull(this->obj)) {
            SQRESULT result;
            SQInteger top = sq_gettop(this->vm);
            sq_pushobject(this->vm, this->obj);
            sq_pushroottable(this->vm);
            result = sq_call(this->vm, 1, false, true);
            sq_settop(this->vm, top);
            if(SQ_FAILED(result)) {
                errMsg = LastErrorString(this->vm);
                return false;
            }
            return true;
        }
        return false;
    }
#endif

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Writes the byte code of the Script to a file
    ///
    /// \param path File path to write to
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    void WriteCompiledFile(const string& path) {
#if !defined (SCRAT_NO_ERROR_CHECKING)
        if(!sq_isnull(this->obj)) {
            sq_pushobject(this->vm, this->obj);
            sqstd_writeclosuretofile(this->vm, path.c_str());
        }
#else
        sq_pushobject(this->vm, this->obj);
        sqstd_writeclosuretofile(this->vm, path.c_str());
#endif
        sq_pop(this->vm, 1); // needed?
    }
};

}

#endif
