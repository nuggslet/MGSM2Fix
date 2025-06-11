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
template <Squirk Q>
class Enumeration : public Object<Q> {
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
    Enumeration(HSQUIRRELVM<Q> v = DefaultVM<Q>::Get(), bool createTable = true) : Object<Q>(v, false) {
        if(createTable) {
            sq_newtable(this->vm);
            sq_getstackobj(this->vm,-1,&this->obj);
            sq_addref(this->vm, &this->obj);
            sq_pop(this->vm,1);
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
    virtual Enumeration<Q>& Const(const SQChar* name, const int val) {
        this->BindValue(name, val, false);
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
    virtual Enumeration<Q>& Const(const SQChar* name, const float val) {
        this->BindValue(name, val, false);
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
    virtual Enumeration<Q>& Const(const SQChar* name, const SQChar* val) {
        this->BindValue(name, val, false);
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
template <Squirk Q>
class ConstTable : public Enumeration<Q> {
public:
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Constructs a ConstTable object to represent the given VM's const table
    ///
    /// \param v VM to get the ConstTable for
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ConstTable(HSQUIRRELVM<Q> v = DefaultVM<Q>::Get()) : Enumeration<Q>(v, false) {
        sq_pushconsttable(this->vm);
        sq_getstackobj(this->vm,-1, &this->obj);
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
    virtual ConstTable<Q>& Const(const SQChar* name, const int val) {
        Enumeration<Q>::Const(name, val);
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
    virtual ConstTable<Q>& Const(const SQChar* name, const float val) {
        Enumeration<Q>::Const(name, val);
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
    virtual ConstTable<Q>& Const(const SQChar* name, const SQChar* val) {
        Enumeration<Q>::Const(name, val);
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
    ConstTable<Q>& Enum(const SQChar* name, Enumeration<Q>& en) {
        sq_pushobject(this->vm, this->GetObject());
        sq_pushstring(this->vm, name, -1);
        sq_pushobject(this->vm, en.GetObject());
        sq_newslot(this->vm, -3, false);
        sq_pop(this->vm,1); // pop table
        return *this;
    }
};

}

#endif
