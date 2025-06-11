//
// SqratTable: Table Binding
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

#if !defined(_SCRAT_TABLE_H_)
#define _SCRAT_TABLE_H_

#include <squirrel.h>
#include <string.h>

#include "sqratObject.h"
#include "sqratFunction.h"
#include "sqratGlobalMethods.h"

namespace Sqrat {

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// The base class for Table that implements almost all of its functionality
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template <Squirk Q>
class TableBase : public Object<Q> {
public:

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Default constructor (null)
    ///
    /// \param v VM that the table will exist in
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    TableBase(HSQUIRRELVM<Q> v = DefaultVM<Q>::Get()) : Object<Q>(v, true) {
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Construct the TableBase from an Object that already exists
    ///
    /// \param obj An Object that should already represent a Squirrel table
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    TableBase(const Object<Q>& obj) : Object<Q>(obj) {
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Construct the TableBase from a HSQOBJECT and HSQUIRRELVM that already exist
    ///
    /// \param o Squirrel object that should already represent a Squirrel table
    /// \param v Squirrel VM that contains the Squirrel object given
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    TableBase(HSQOBJECT<Q> o, HSQUIRRELVM<Q> v = DefaultVM<Q>::Get()) : Object<Q>(o, v) {
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Binds a Table or Class to the Table (can be used to facilitate namespaces)
    ///
    /// \param name The key in the table being assigned a Table or Class
    /// \param obj  Table or Class that is being placed in the table
    ///
    /// \remarks
    /// Bind cannot be called "inline" like other functions because it introduces order-of-initialization bugs.
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    void Bind(const SQChar* name, Object<Q>& obj) {
        sq_pushobject(this->vm, this->GetObject());
        sq_pushstring(this->vm, name, -1);
        sq_pushobject(this->vm, obj.GetObject());
        sq_newslot(this->vm, -3, false);
        sq_pop(this->vm,1); // pop table
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Binds a raw Squirrel closure to the Table
    ///
    /// \param name The key in the table being assigned a function
    /// \param func Squirrel function that is being placed in the Table
    ///
    /// \return The Table itself so the call can be chained
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    TableBase<Q>& SquirrelFunc(const SQChar* name, SQFUNCTION<Q> func) {
        sq_pushobject(this->vm, this->GetObject());
        sq_pushstring(this->vm, name, -1);
        sq_newclosure(this->vm, func, 0);
        sq_newslot(this->vm, -3, false);
        sq_pop(this->vm,1); // pop table
        return *this;
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Sets a key in the Table to a specific value
    ///
    /// \param name The key in the table being assigned a value
    /// \param val  Value that is being placed in the Table
    ///
    /// \tparam V Type of value (usually doesnt need to be defined explicitly)
    ///
    /// \return The Table itself so the call can be chained
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    template<class V>
    TableBase<Q>& SetValue(const SQChar* name, const V& val) {
        this->BindValue(name, val, false);
        return *this;
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Sets an index in the Table to a specific value
    ///
    /// \param index The index in the table being assigned a value
    /// \param val   Value that is being placed in the Table
    ///
    /// \tparam V Type of value (usually doesnt need to be defined explicitly)
    ///
    /// \return The Table itself so the call can be chained
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    template<class V>
    TableBase<Q>& SetValue(const SQInteger index, const V& val) {
        this->BindValue(index, val, false);
        return *this;
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Sets a key in the Table to a specific instance (like a reference)
    ///
    /// \param name The key in the table being assigned a value
    /// \param val  Pointer to the instance that is being placed in the Table
    ///
    /// \tparam V Type of instance (usually doesnt need to be defined explicitly)
    ///
    /// \return The Table itself so the call can be chained
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    template<class V>
    TableBase<Q>& SetInstance(const SQChar* name, V* val) {
        this->BindInstance(name, val, false);
        return *this;
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Sets an index in the Table to a specific instance (like a reference)
    ///
    /// \param index The index in the table being assigned a value
    /// \param val   Pointer to the instance that is being placed in the Table
    ///
    /// \tparam V Type of instance (usually doesnt need to be defined explicitly)
    ///
    /// \return The Table itself so the call can be chained
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    template<class V>
    TableBase<Q>& SetInstance(const SQInteger index, V* val) {
        this->BindInstance(index, val, false);
        return *this;
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Sets a key in the Table to a specific function
    ///
    /// \param name   The key in the table being assigned a value
    /// \param method Function that is being placed in the Table
    ///
    /// \tparam F Type of function (only define this if you need to choose a certain template specialization or overload)
    ///
    /// \return The Table itself so the call can be chained
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    template<class F>
    TableBase<Q>& Func(const SQChar* name, F method) {
        BindFunc(name, &method, sizeof(method), SqGlobalFunc(method));
        return *this;
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Sets a key in the Table to a specific function and allows the key to be overloaded with functions of a different amount of arguments
    ///
    /// \param name   The key in the table being assigned a value
    /// \param method Function that is being placed in the Table
    ///
    /// \tparam F Type of function (only define this if you need to choose a certain template specialization or overload)
    ///
    /// \return The Table itself so the call can be chained
    ///
    /// \remarks
    /// Overloading in Sqrat does not work for functions with the same amount of arguments (just like in Squirrel).
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    template<class F>
    TableBase<Q>& Overload(const SQChar* name, F method) {
        BindOverload(name, &method, sizeof(method), SqGlobalOverloadedFunc(method), SqOverloadFunc(method), SqGetArgCount(method));
        return *this;
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Checks if the given key exists in the table
    ///
    /// \param name Key to check
    ///
    /// \return True on success, otherwise false
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    bool HasKey(const SQChar* name)
    {
        sq_pushobject(this->vm, this->obj);
        sq_pushstring(this->vm, name, -1);
        if (SQ_FAILED(sq_get(this->vm, -2))) {
            sq_pop(this->vm, 1);
            return false;
        }
        sq_pop(this->vm, 2);
        return true;
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Returns the value at a given key
    ///
    /// \param name Key of the element
    ///
    /// \tparam T Type of value (fails if value is not of this type)
    ///
    /// \return SharedPtr containing the value (or null if failed)
    ///
    /// \remarks
    /// This function MUST have its Error handled if it occurred.
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    template <typename T>
    SharedPtr<T> GetValue(const SQChar* name)
    {
        sq_pushobject(this->vm, this->obj);
        sq_pushstring(this->vm, name, -1);
#if !defined (SCRAT_NO_ERROR_CHECKING)
        if (SQ_FAILED(sq_get(this->vm, -2))) {
            sq_pop(this->vm, 1);
            SQTHROW(this->vm, _SC("illegal index"));
            return SharedPtr<T>();
        }
#else
        sq_get(this->vm, -2);
#endif
        SQTRY()
        Var<SharedPtr<T>, Q> entry(this->vm, -1);
        SQCATCH_NOEXCEPT(this->vm) {
            sq_pop(this->vm, 2);
            return SharedPtr<T>();
        }
        sq_pop(this->vm, 2);
        return entry.value;
        SQCATCH(this->vm) {
#if defined (SCRAT_USE_EXCEPTIONS)
            SQUNUSED(this->e); // avoid "unreferenced local variable" warning
#endif
            sq_pop(this->vm, 2);
            SQRETHROW(this->vm);
        }
        return SharedPtr<T>(); // avoid "not all control paths return a value" warning
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Returns the value at a given index
    ///
    /// \param index Index of the element
    ///
    /// \tparam T Type of value (fails if value is not of this type)
    ///
    /// \return SharedPtr containing the value (or null if failed)
    ///
    /// \remarks
    /// This function MUST have its Error handled if it occurred.
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    template <typename T>
    SharedPtr<T> GetValue(int index)
    {
        sq_pushobject(this->vm, this->obj);
        sq_pushinteger(this->vm, index);
#if !defined (SCRAT_NO_ERROR_CHECKING)
        if (SQ_FAILED(sq_get(this->vm, -2))) {
            sq_pop(this->vm, 1);
            SQTHROW(this->vm, _SC("illegal index"));
            return SharedPtr<T>();
        }
#else
        sq_get(this->vm, -2);
#endif
        SQTRY()
        Var<SharedPtr<T>, Q> entry(this->vm, -1);
        SQCATCH_NOEXCEPT(this->vm) {
            sq_pop(this->vm, 2);
            return SharedPtr<T>();
        }
        sq_pop(this->vm, 2);
        return entry.value;
        SQCATCH(this->vm) {
#if defined (SCRAT_USE_EXCEPTIONS)
            SQUNUSED(this->e); // avoid "unreferenced local variable" warning
#endif
            sq_pop(this->vm, 2);
            SQRETHROW(this->vm);
        }
        return SharedPtr<T>(); // avoid "not all control paths return a value" warning
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Gets a Function from a key in the Table
    ///
    /// \param name The key in the table that contains the Function
    ///
    /// \return Function found in the Table (null if failed)
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    Function<Q> GetFunction(const SQChar* name) {
        HSQOBJECT<Q> funcObj;
        sq_pushobject(this->vm, this->GetObject());
        sq_pushstring(this->vm, name, -1);
#if !defined (SCRAT_NO_ERROR_CHECKING)
        if(SQ_FAILED(sq_get(this->vm, -2))) {
            sq_pop(this->vm, 1);
            return Function<Q>();
        }
        SQObjectType value_type = sq_gettype(this->vm, -1);
        if (value_type != OT_CLOSURE && value_type != OT_NATIVECLOSURE) {
            sq_pop(this->vm, 2);
            return Function<Q>();
        }
#else
        sq_get(this->vm, -2);
#endif
        sq_getstackobj(this->vm, -1, &funcObj);
        Function<Q> ret(this->vm, this->GetObject(), funcObj); // must addref before the pop!
        sq_pop(this->vm, 2);
        return ret;
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Gets a Function from an index in the Table
    ///
    /// \param index The index in the table that contains the Function
    ///
    /// \return Function found in the Table (null if failed)
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    Function<Q> GetFunction(const SQInteger index) {
        HSQOBJECT<Q> funcObj;
        sq_pushobject(this->vm, this->GetObject());
        sq_pushinteger(this->vm, index);
#if !defined (SCRAT_NO_ERROR_CHECKING)
        if(SQ_FAILED(sq_get(this->vm, -2))) {
            sq_pop(this->vm, 1);
            return Function<Q>();
        }
        SQObjectType value_type = sq_gettype(this->vm, -1);
        if (value_type != OT_CLOSURE && value_type != OT_NATIVECLOSURE) {
            sq_pop(this->vm, 2);
            return Function<Q>();
        }
#else
        sq_get(this->vm, -2);
#endif
        sq_getstackobj(this->vm, -1, &funcObj);
        Function<Q> ret(this->vm, this->GetObject(), funcObj); // must addref before the pop!
        sq_pop(this->vm, 2);
        return ret;
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Represents a table in Squirrel
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template <Squirk Q>
class Table : public TableBase<Q> {
public:

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Default constructor (null)
    ///
    /// \remarks
    /// The Table is invalid until it is given a VM to exist in.
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    Table() {
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Constructs a Table
    ///
    /// \param v VM to create the Table in
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    Table(HSQUIRRELVM<Q> v) : TableBase<Q>(v) {
        sq_newtable(this->vm);
        sq_getstackobj(this->vm,-1,&this->obj);
        sq_addref(this->vm, &this->obj);
        sq_pop(this->vm,1);
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Construct the Table from an Object that already exists
    ///
    /// \param obj An Object that should already represent a Squirrel table
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    Table(const Object<Q>& obj) : TableBase<Q>(obj) {
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Construct the Table from a HSQOBJECT and HSQUIRRELVM that already exist
    ///
    /// \param o Squirrel object that should already represent a Squirrel table
    /// \param v Squirrel VM that contains the Squirrel object given
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    Table(HSQOBJECT<Q> o, HSQUIRRELVM<Q> v = DefaultVM<Q>::Get()) : TableBase<Q>(o, v) {
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Table that is a reference to the Squirrel root table for a given VM
/// The Squirrel root table is usually where all globals are stored by the Squirrel language.
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template <Squirk Q>
class RootTable : public TableBase<Q> {
public:

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Constructs a RootTable object to represent the given VM's root table
    ///
    /// \param v VM to get the RootTable for
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    RootTable(HSQUIRRELVM<Q> v = DefaultVM<Q>::Get()) : TableBase<Q>(v) {
        sq_pushroottable(this->vm);
        sq_getstackobj(this->vm,-1,&this->obj);
        sq_addref(this->vm, &this->obj);
        sq_pop(v,1); // pop root table
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Table that is a reference to the Squirrel registry table for a given VM
/// The Squirrel registry table is where non-Squirrel code can store Squirrel objects without worrying about Squirrel code messing with them.
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template <Squirk Q>
class RegistryTable : public TableBase<Q> {
public:

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Constructs a RegistryTable object to represent the given VM's registry table
    ///
    /// \param v VM to get the RegistryTable for
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    RegistryTable(HSQUIRRELVM<Q> v = DefaultVM<Q>::Get()) : TableBase<Q>(v) {
        sq_pushregistrytable(v);
        sq_getstackobj(this->vm,-1,&this->obj);
        sq_addref(this->vm, &this->obj);
        sq_pop(v,1); // pop the registry table
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Used to get and push Table instances to and from the stack as references (tables are always references in Squirrel)
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template<Squirk Q>
struct Var<Table<Q>, Q> {

    Table<Q> value; ///< The actual value of get operations

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Attempts to get the value off the stack at idx as a Table
    ///
    /// \param vm  Target VM
    /// \param idx Index trying to be read
    ///
    /// \remarks
    /// This function MUST have its Error handled if it occurred.
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    Var(HSQUIRRELVM<Q> vm, SQInteger idx) {
        HSQOBJECT<Q> obj;
        sq_resetobject(&obj);
        sq_getstackobj(vm,idx,&obj);
        value = Table<Q>(obj, vm);
#if !defined (SCRAT_NO_ERROR_CHECKING)
        SQObjectType value_type = sq_gettype(vm, idx);
        if (value_type != OT_TABLE) {
            SQTHROW(vm, FormatTypeError(vm, idx, _SC("table")));
        }
#endif
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Called by Sqrat::PushVar to put an Table reference on the stack
    ///
    /// \param vm    Target VM
    /// \param value Value to push on to the VM's stack
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static void push(HSQUIRRELVM<Q> vm, const Table<Q>& value) {
        HSQOBJECT<Q> obj;
        sq_resetobject(&obj);
        obj = value.GetObject();
        sq_pushobject(vm,obj);
    }
};


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Used to get and push Table instances to and from the stack as references (tables are always references in Squirrel)
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template<Squirk Q>
struct Var<Table<Q>&, Q> : Var<Table<Q>, Q> {Var(HSQUIRRELVM<Q> vm, SQInteger idx) : Var<Table<Q>, Q>(vm, idx) {}};


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Used to get and push Table instances to and from the stack as references (tables are always references in Squirrel)
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template<Squirk Q>
struct Var<const Table<Q>&, Q> : Var<Table<Q>, Q> {Var(HSQUIRRELVM<Q> vm, SQInteger idx) : Var<Table<Q>, Q>(vm, idx) {}};

}

#endif
