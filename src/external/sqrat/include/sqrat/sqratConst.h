//
// SqratConst: Constant and Enumeration Binding
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

#if !defined(_SCRAT_CONST_H_)
#define _SCRAT_CONST_H_

#include <squirrel.h>
#include <string.h>

#include "sqratObject.h"

namespace Sqrat {

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Facilitates exposing a C++ enumeration to Squirrel
///
/// \remarks
/// The Enumeration class only facilitates binding C++ enumerations that contain only integers,
/// floats, and strings because the Squirrel constant table can only contain these types of
/// values. Other types of enumerations can be bound using Class::SetStaticValue instead.
///
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Enumeration : public Object {
public:
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Constructs the Enumeration object
    ///
    /// An Enumeration object doesnt do anything on its own.
    /// It must be told what constant values it contains.
    /// This is done using Enumeration::Const.
    /// Then the Enumeration must be exposed to Squirrel.
    /// This is done by calling ConstTable::Enum with the Enumeration.
    ///
    /// \param v           Squirrel virtual machine to create the Enumeration for
    /// \param createTable Whether the underlying table that values are bound to is created by the constructor
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    Enumeration(HSQUIRRELVM v = DefaultVM::Get(), bool createTable = true) : Object(v, false) {
        if(createTable) {
            sq_newtable(vm);
            sq_getstackobj(vm,-1,&obj);
            sq_addref(vm, &obj);
            sq_pop(vm,1);
        }
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Binds an enumeration value
    ///
    /// \param name Name of the value as it will appear in Squirrel
    /// \param val  Value to bind
    ///
    /// \return The Enumeration itself so the call can be chained
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual Enumeration& Const(const SQChar* name, const int val) {
        BindValue<int>(name, val, false);
        return *this;
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Binds an enumeration value
    ///
    /// \param name Name of the value as it will appear in Squirrel
    /// \param val  Value to bind
    ///
    /// \return The Enumeration itself so the call can be chained
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual Enumeration& Const(const SQChar* name, const float val) {
        BindValue<float>(name, val, false);
        return *this;
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Binds an enumeration value
    ///
    /// \param name Name of the value as it will appear in Squirrel
    /// \param val  Value to bind
    ///
    /// \return The Enumeration itself so the call can be chained
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual Enumeration& Const(const SQChar* name, const SQChar* val) {
        BindValue<const SQChar*>(name, val, false);
        return *this;
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Facilitates exposing a C++ constant to Squirrel
///
/// \remarks
/// The ConstTable class only facilitates binding C++ constants that are integers,
/// floats, and strings because the Squirrel constant table can only contain these types of
/// values. Other types of constants can be bound using Class::SetStaticValue instead.
///
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ConstTable : public Enumeration {
public:
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Constructs a ConstTable object to represent the given VM's const table
    ///
    /// \param v VM to get the ConstTable for
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ConstTable(HSQUIRRELVM v = DefaultVM::Get()) : Enumeration(v, false) {
        sq_pushconsttable(vm);
        sq_getstackobj(vm,-1, &obj);
        sq_pop(v,1); // No addref needed, since the consttable is always around
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Binds a constant value
    ///
    /// \param name Name of the value as it will appear in Squirrel
    /// \param val  Value to bind
    ///
    /// \return The ConstTable itself so the call can be chained
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ConstTable& Const(const SQChar* name, const int val) {
        Enumeration::Const(name, val);
        return *this;
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Binds a constant value
    ///
    /// \param name Name of the value as it will appear in Squirrel
    /// \param val  Value to bind
    ///
    /// \return The ConstTable itself so the call can be chained
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ConstTable& Const(const SQChar* name, const float val) {
        Enumeration::Const(name, val);
        return *this;
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Binds a constant value
    ///
    /// \param name Name of the value as it will appear in Squirrel
    /// \param val  Value to bind
    ///
    /// \return The ConstTable itself so the call can be chained
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ConstTable& Const(const SQChar* name, const SQChar* val) {
        Enumeration::Const(name, val);
        return *this;
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Binds an Enumeration to the ConstTable
    ///
    /// \param name Name of the enumeration as it will appear in Squirrel
    /// \param en   Enumeration to bind
    ///
    /// \return The ConstTable itself so the call can be chained
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ConstTable& Enum(const SQChar* name, Enumeration& en) {
        sq_pushobject(vm, GetObject());
        sq_pushstring(vm, name, -1);
        sq_pushobject(vm, en.GetObject());
        sq_newslot(vm, -3, false);
        sq_pop(vm,1); // pop table
        return *this;
    }
};

}

#endif
