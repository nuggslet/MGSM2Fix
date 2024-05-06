//
// SqratArray: Array Binding
//

//
// Copyright 2011 Alston Chen
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

#if !defined(_SCRAT_ARRAY_H_)
#define _SCRAT_ARRAY_H_

#include <squirrel.h>
#include <string.h>

#include "sqratObject.h"
#include "sqratFunction.h"
#include "sqratGlobalMethods.h"

namespace Sqrat {

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// The base class for Array that implements almost all of its functionality
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template <Squirk Q>
class ArrayBase : public Object<Q> {
public:

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Default constructor (null)
    ///
    /// \param v VM that the array will exist in
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ArrayBase(HSQUIRRELVM<Q> v = DefaultVM<Q>::Get()) : Object<Q>(v, true) {
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Construct the ArrayBase from an Object that already exists
    ///
    /// \param obj An Object that should already represent a Squirrel array
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ArrayBase(const Object<Q>& obj) : Object<Q>(obj) {
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Construct the ArrayBase from a HSQOBJECT and HSQUIRRELVM that already exist
    ///
    /// \param o Squirrel object that should already represent a Squirrel array
    /// \param v Squirrel VM that contains the Squirrel object given
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ArrayBase(HSQOBJECT<Q> o, HSQUIRRELVM<Q> v = DefaultVM<Q>::Get()) : Object<Q>(o, v) {
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Binds a Table or Class to the Array (can be used to facilitate namespaces)
    ///
    /// \param index The index in the array being assigned a Table or Class
    /// \param obj   Table or Class that is being placed in the Array
    ///
    /// \remarks
    /// Bind cannot be called "inline" like other functions because it introduces order-of-initialization bugs.
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    void Bind(const SQInteger index, Object<Q>& obj) {
        sq_pushobject(this->vm, this->GetObject());
        sq_pushinteger(this->vm, index);
        sq_pushobject(this->vm, obj.GetObject());
        sq_set(this->vm, -3);
        sq_pop(this->vm,1); // pop array
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Binds a raw Squirrel closure to the Array
    ///
    /// \param index The index in the array being assigned a function
    /// \param func  Squirrel function that is being placed in the Array
    ///
    /// \return The Array itself so the call can be chained
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ArrayBase<Q>& SquirrelFunc(const SQInteger index, SQFUNCTION<Q> func) {
        sq_pushobject(this->vm, this->GetObject());
        sq_pushinteger(this->vm, index);
        sq_newclosure(this->vm, func, 0);
        sq_set(this->vm, -3);
        sq_pop(this->vm,1); // pop array
        return *this;
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Sets an index in the Array to a specific value
    ///
    /// \param index The index in the array being assigned a value
    /// \param val   Value that is being placed in the Array
    ///
    /// \tparam V Type of value (usually doesnt need to be defined explicitly)
    ///
    /// \return The Array itself so the call can be chained
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    template<class V>
    ArrayBase& SetValue(const SQInteger index, const V& val) {
        sq_pushobject(this->vm, this->GetObject());
        sq_pushinteger(this->vm, index);
        PushVar(this->vm, val);
        sq_set(this->vm, -3);
        sq_pop(this->vm,1); // pop array
        return *this;
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Sets an index in the Array to a specific instance (like a reference)
    ///
    /// \param index The index in the array being assigned a value
    /// \param val   Pointer to the instance that is being placed in the Array
    ///
    /// \tparam V Type of instance (usually doesnt need to be defined explicitly)
    ///
    /// \return The Array itself so the call can be chained
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    template<class V>
    ArrayBase<Q>& SetInstance(const SQInteger index, V* val) {
        this->BindInstance(index, val, false);
        return *this;
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Sets an index in the Array to a specific function
    ///
    /// \param index  The index in the array being assigned a value
    /// \param method Function that is being placed in the Array
    ///
    /// \tparam F Type of function (only define this if you need to choose a certain template specialization or overload)
    ///
    /// \return The Array itself so the call can be chained
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    template<class F>
    ArrayBase<Q>& Func(const SQInteger index, F method) {
        BindFunc(index, &method, sizeof(method), SqGlobalFunc(method));
        return *this;
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Returns the element at a given index
    ///
    /// \param index Index of the element
    ///
    /// \tparam T Type of element (fails if element is not of this type)
    ///
    /// \return SharedPtr containing the element (or null if failed)
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
        Var<SharedPtr<T>, Q> element(this->vm, -1);
        SQCATCH_NOEXCEPT(this->vm) {
            sq_pop(this->vm, 2);
            return SharedPtr<T>();
        }
        sq_pop(this->vm, 2);
        return element.value;
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
    /// Gets a Function from an index in the Array
    ///
    /// \param index The index in the array that contains the Function
    ///
    /// \return Function found in the Array (null if failed)
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

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Fills a C array with the elements of the Array
    ///
    /// \param array C array to be filled
    /// \param size  The amount of elements to fill the C array with
    ///
    /// \tparam T Type of elements (fails if any elements in Array are not of this type)
    ///
    /// \remarks
    /// This function MUST have its Error handled if it occurred.
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    template <typename T>
    void GetArray(T* array, int size)
    {
        HSQOBJECT<Q> value = this->GetObject();
        sq_pushobject(this->vm, value);
#if !defined (SCRAT_NO_ERROR_CHECKING)
        if (size > sq_getsize(this->vm, -1)) {
            sq_pop(this->vm, 1);
            SQTHROW(this->vm, _SC("array buffer size too big"));
            return;
        }
#endif
        sq_pushnull(this->vm);
        SQInteger i;
        while (SQ_SUCCEEDED(sq_next(this->vm, -2))) {
            sq_getinteger(this->vm, -2, &i);
            if (i >= size) break;
            SQTRY()
            Var<const T&, Q> element(this->vm, -1);
            SQCATCH_NOEXCEPT(this->vm) {
                sq_pop(this->vm, 4);
                return;
            }
            sq_pop(this->vm, 2);
            array[i] = element.value;
            SQCATCH(this->vm) {
#if defined (SCRAT_USE_EXCEPTIONS)
                SQUNUSED(this->e); // avoid "unreferenced local variable" warning
#endif
                sq_pop(this->vm, 4);
                SQRETHROW(this->vm);
            }
        }
        sq_pop(this->vm, 2); // pops the null iterator and the array object
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Appends a value to the end of the Array
    ///
    /// \param val Value that is being placed in the Array
    ///
    /// \tparam V Type of value (usually doesnt need to be defined explicitly)
    ///
    /// \return The Array itself so the call can be chained
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    template<class V>
    ArrayBase<Q>& Append(const V& val) {
        sq_pushobject(this->vm, this->GetObject());
        PushVar(this->vm, val);
        sq_arrayappend(this->vm, -2);
        sq_pop(this->vm,1); // pop array
        return *this;
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Appends an instance to the end of the Array (like a reference)
    ///
    /// \param val Pointer to the instance that is being placed in the Array
    ///
    /// \tparam V Type of instance (usually doesnt need to be defined explicitly)
    ///
    /// \return The Array itself so the call can be chained
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    template<class V>
    ArrayBase<Q>& Append(V* val) {
        sq_pushobject(this->vm, this->GetObject());
        PushVar(this->vm, val);
        sq_arrayappend(this->vm, -2);
        sq_pop(this->vm,1); // pop array
        return *this;
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Inserts a value in a position in the Array
    ///
    /// \param destpos Index to put the new value in
    /// \param val     Value that is being placed in the Array
    ///
    /// \tparam V Type of value (usually doesnt need to be defined explicitly)
    ///
    /// \return The Array itself so the call can be chained
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    template<class V>
    ArrayBase<Q>& Insert(const SQInteger destpos, const V& val) {
        sq_pushobject(this->vm, this->GetObject());
        PushVar(this->vm, val);
        sq_arrayinsert(this->vm, -2, destpos);
        sq_pop(this->vm,1); // pop array
        return *this;
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Inserts an instance in a position in the Array (like a reference)
    ///
    /// \param destpos Index to put the new value in
    /// \param val     Pointer to the instance that is being placed in the Array
    ///
    /// \tparam V Type of instance (usually doesnt need to be defined explicitly)
    ///
    /// \return The Array itself so the call can be chained
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    template<class V>
    ArrayBase<Q>& Insert(const SQInteger destpos, V* val) {
        sq_pushobject(this->vm, this->GetObject());
        PushVar(this->vm, val);
        sq_arrayinsert(this->vm, -2, destpos);
        sq_pop(this->vm,1); // pop array
        return *this;
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Removes the last element from the Array
    ///
    /// \return Object for the element that was removed (null if failed)
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    Object<Q> Pop() {
        HSQOBJECT<Q> slotObj;
        sq_pushobject(this->vm, this->GetObject());
        if(SQ_FAILED(sq_arraypop(this->vm, -1, true))) {
            sq_pop(this->vm, 1);
            return Object<Q>(); // Return a NULL object
        } else {
            sq_getstackobj(this->vm, -1, &slotObj);
            Object<Q> ret(slotObj, this->vm);
            sq_pop(this->vm, 2);
            return ret;
        }
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Removes an element at a specific index from the Array
    ///
    /// \param itemidx Index of the element being removed
    ///
    /// \return The Array itself so the call can be chained
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ArrayBase<Q>& Remove(const SQInteger itemidx) {
        sq_pushobject(this->vm, this->GetObject());
        sq_arrayremove(this->vm, -1, itemidx);
        sq_pop(this->vm,1); // pop array
        return *this;
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Resizes the Array
    ///
    /// \param newsize Desired size of the Array in number of elements
    ///
    /// \return The Array itself so the call can be chained
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ArrayBase<Q>& Resize(const SQInteger newsize) {
        sq_pushobject(this->vm, this->GetObject());
        sq_arrayresize(this->vm, -1, newsize);
        sq_pop(this->vm,1); // pop array
        return *this;
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Reverses the elements of the array in place
    ///
    /// \return The Array itself so the call can be chained
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ArrayBase<Q>& Reverse() {
        sq_pushobject(this->vm, this->GetObject());
        sq_arrayreverse(this->vm, -1);
        sq_pop(this->vm,1); // pop array
        return *this;
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Returns the length of the Array
    ///
    /// \return Length in number of elements
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    SQInteger Length() const
    {
        sq_pushobject(this->vm, this->obj);
        SQInteger r = sq_getsize(this->vm, -1);
        sq_pop(this->vm, 1);
        return r;
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Represents an array in Squirrel
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template <Squirk Q>
class Array : public ArrayBase<Q> {
public:

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Default constructor (null)
    ///
    /// \remarks
    /// The Array is invalid until it is given a VM to exist in.
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    Array() {
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Constructs an Array
    ///
    /// \param v    VM to create the Array in
    /// \param size An optional size hint
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    Array(HSQUIRRELVM<Q> v, const SQInteger size = 0) : ArrayBase<Q>(v) {
        sq_newarray(this->vm, size);
        sq_getstackobj(this->vm,-1,&this->obj);
        sq_addref(this->vm, &this->obj);
        sq_pop(this->vm,1);
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Construct the Array from an Object that already exists
    ///
    /// \param obj An Object that should already represent a Squirrel array
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    Array(const Object<Q>& obj) : ArrayBase<Q>(obj) {
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Construct the Array from a HSQOBJECT and HSQUIRRELVM that already exist
    ///
    /// \param o Squirrel object that should already represent a Squirrel array
    /// \param v Squirrel VM that contains the Squirrel object given
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    Array(HSQOBJECT<Q> o, HSQUIRRELVM<Q> v = DefaultVM<Q>::Get()) : ArrayBase<Q>(o, v) {
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Used to get and push Array instances to and from the stack as references (arrays are always references in Squirrel)
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template<Squirk Q>
struct Var<Array<Q>, Q> {

    Array<Q> value; ///< The actual value of get operations

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Attempts to get the value off the stack at idx as an Array
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
        value = Array(obj, vm);
#if !defined (SCRAT_NO_ERROR_CHECKING)
        SQObjectType value_type = sq_gettype(vm, idx);
        if (value_type != OT_ARRAY) {
            SQTHROW(vm, FormatTypeError(vm, idx, _SC("array")));
        }
#endif
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Called by Sqrat::PushVar to put an Array reference on the stack
    ///
    /// \param vm    Target VM
    /// \param value Value to push on to the VM's stack
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static void push(HSQUIRRELVM<Q> vm, const Array<Q>& value) {
        HSQOBJECT<Q> obj;
        sq_resetobject(&obj);
        obj = value.GetObject();
        sq_pushobject(vm,obj);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Used to get and push Array instances to and from the stack as references (arrays are always references in Squirrel)
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template<Squirk Q>
struct Var<Array<Q>&, Q> : Var<Array<Q>, Q> {Var(HSQUIRRELVM<Q> vm, SQInteger idx) : Var<Array<Q>, Q>(vm, idx) {}};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Used to get and push Array instances to and from the stack as references (arrays are always references in Squirrel)
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template<Squirk Q>
struct Var<const Array<Q>&, Q> : Var<Array<Q>, Q> {Var(HSQUIRRELVM<Q> vm, SQInteger idx) : Var<Array<Q>, Q>(vm, idx) {}};

}

#endif
