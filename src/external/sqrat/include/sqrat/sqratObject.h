//
// SqratObject: Referenced Squirrel Object Wrapper
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

#if !defined(_SCRAT_OBJECT_H_)
#define _SCRAT_OBJECT_H_

#include <squirrel.h>
#include <string.h>

#include "sqratAllocator.h"
#include "sqratTypes.h"
#include "sqratOverloadMethods.h"
#include "sqratUtil.h"

namespace Sqrat {

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// The base class for classes that represent Squirrel objects
///
/// \remarks
/// All Object and derived classes MUST be destroyed before calling sq_close or your application will crash when exiting.
///
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template <Squirk Q>
class Object {
protected:

/// @cond DEV
    HSQUIRRELVM<Q> vm;
    HSQOBJECT<Q> obj;
    bool release;

    Object(HSQUIRRELVM<Q> v, bool releaseOnDestroy = true) : vm(v), release(releaseOnDestroy) {
        sq_resetobject(&obj);
    }
/// @endcond

public:

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Default constructor (null)
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    Object() : vm(0), release(true) {
        sq_resetobject(&obj);
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Copy constructor
    ///
    /// \param so Object to copy
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    Object(const Object<Q>& so) : vm(so.vm), obj(so.obj), release(so.release) {
        sq_addref(vm, &obj);
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Constructs an Object from a Squirrel object
    ///
    /// \param o Squirrel object
    /// \param v VM that the object will exist in
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    Object(HSQOBJECT<Q> o, HSQUIRRELVM<Q> v = DefaultVM<Q>::Get()) : vm(v), obj(o), release(true) {
        sq_addref(vm, &obj);
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Constructs an Object from a C++ instance
    ///
    /// \param instance Pointer to a C++ class instance that has been bound already
    /// \param v        VM that the object will exist in
    ///
    /// \tparam T Type of instance
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    template<class T>
    Object(T* instance, HSQUIRRELVM<Q> v = DefaultVM<Q>::Get()) : vm(v), release(true) {
        ClassType<T, Q>::PushInstance(vm, instance);
        sq_getstackobj(vm, -1, &obj);
        sq_addref(vm, &obj);
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Destructor
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~Object() {
        if(release) {
            Release();
            release = false;
        }
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Assignment operator
    ///
    /// \param so Object to copy
    ///
    /// \return The Object itself
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    Object<Q>& operator=(const Object<Q>& so) {
        if(release) {
            Release();
        }
        vm = so.vm;
        obj = so.obj;
        release = so.release;
        sq_addref(vm, &GetObject());
        return *this;
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Gets the Squirrel VM for this Object (reference)
    ///
    /// \return Squirrel VM associated with the Object
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    HSQUIRRELVM<Q>& GetVM() {
        return vm;
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Gets the Squirrel VM for this Object (copy)
    ///
    /// \return Squirrel VM associated with the Object
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    HSQUIRRELVM<Q> GetVM() const {
        return vm;
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Gets the type of the Object as defined by the Squirrel API
    ///
    /// \return SQObjectType for the Object
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    SQObjectType GetType() const {
        return GetObject()._type;
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Checks whether the Object is null
    ///
    /// \return True if the Object currently has a null value, otherwise false
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    bool IsNull() const {
        return sq_isnull(GetObject());
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Gets the Squirrel object for this Object (copy)
    ///
    /// \return Squirrel object
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual HSQOBJECT<Q> GetObject() const {
        return obj;
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Gets the Squirrel object for this Object (reference)
    ///
    /// \return Squirrel object
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual HSQOBJECT<Q>& GetObject() {
        return obj;
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Allows the Object to be inputted directly into places that expect a HSQOBJECT
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    operator HSQOBJECT<Q>&() {
        return GetObject();
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Sets the Object to null (removing its references to underlying Squirrel objects)
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    void Release() {
        sq_release(vm, &obj);
        sq_resetobject(&obj);
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Attempts to get the value of a slot from the object
    ///
    /// \param slot Name of the slot
    ///
    /// \return An Object representing the value of the slot (can be a null object if nothing was found)
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    Object<Q> GetSlot(const SQChar* slot) const {
        HSQOBJECT<Q> slotObj;
        sq_pushobject(vm, GetObject());
        sq_pushstring(vm, slot, -1);

#if !defined (SCRAT_NO_ERROR_CHECKING)
        if(SQ_FAILED(sq_get(vm, -2))) {
            sq_pop(vm, 1);
            return Object(vm); // Return a NULL object
        } else {
            sq_getstackobj(vm, -1, &slotObj);
            Object<Q> ret(slotObj, vm); // must addref before the pop!
            sq_pop(vm, 2);
            return ret;
        }
#else
        sq_get(vm, -2);
        sq_getstackobj(vm, -1, &slotObj);
        Object<Q> ret(slotObj, vm); // must addref before the pop!
        sq_pop(vm, 2);
        return ret;
#endif
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Attempts to get the value of an index from the object
    ///
    /// \param index Index of the slot
    ///
    /// \return An Object representing the value of the slot (can be a null object if nothing was found)
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    Object<Q> GetSlot(SQInteger index) const {
        HSQOBJECT<Q> slotObj;
        sq_pushobject(vm, GetObject());
        sq_pushinteger(vm, index);

#if !defined (SCRAT_NO_ERROR_CHECKING)
        if(SQ_FAILED(sq_get(vm, -2))) {
            sq_pop(vm, 1);
            return Object<Q>(vm); // Return a NULL object
        } else {
            sq_getstackobj(vm, -1, &slotObj);
            Object<Q> ret(slotObj, vm); // must addref before the pop!
            sq_pop(vm, 2);
            return ret;
        }
#else
        sq_get(vm, -2);
        sq_getstackobj(vm, -1, &slotObj);
        Object<Q> ret(slotObj, vm); // must addref before the pop!
        sq_pop(vm, 2);
        return ret;
#endif
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Casts the object to a certain C++ type
    ///
    /// \tparam T Type to cast to
    ///
    /// \return A copy of the value of the Object with the given type
    ///
    /// \remarks
    /// This function MUST have its Error handled if it occurred.
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    template <class T>
    T Cast() const {
        sq_pushobject(vm, GetObject());
        T ret = Var<T, Q>(vm, -1).value;
        sq_pop(vm, 1);
        return ret;
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Allows Object to be used like C++ arrays with the [] operator
    ///
    /// \param slot The slot key
    ///
    /// \tparam T Type of the slot key (usually doesnt need to be defined explicitly)
    ///
    /// \return An Object representing the value of the slot (can be a null object if nothing was found)
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    template <class T>
    inline Object<Q> operator[](T slot)
    {
        return GetSlot(slot);
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Returns the size of the Object
    ///
    /// \return Size of Object
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    SQInteger GetSize() const {
        sq_pushobject(vm, GetObject());
        SQInteger ret = sq_getsize(vm, -1);
        sq_pop(vm, 1);
        return ret;
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Iterator for going over the slots in the object using Object::Next
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    struct iterator
    {
        /// @cond DEV
        friend class Object;
        /// @endcond

        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Default constructor (null)
        ///
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        iterator()
        {
            Index = 0;
            sq_resetobject(&Key);
            sq_resetobject(&Value);
            Key._type = OT_NULL;
            Value._type = OT_NULL;
        }

        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Returns the string value of the key the iterator is on if possible
        ///
        /// \return String or NULL
        ///
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        const SQChar* getName() { return sq_objtostring(&Key); }

        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Gets the Squirrel object for the key the iterator is on
        ///
        /// \return HSQOBJECT representing a key
        ///
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        HSQOBJECT<Q> getKey() { return Key; }

        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Gets the Squirrel object for the value the iterator is on
        ///
        /// \return HSQOBJECT representing a value
        ///
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        HSQOBJECT<Q> getValue() { return Value; }
    private:

        HSQOBJECT<Q> Key;
        HSQOBJECT<Q> Value;
        SQInteger Index;
    };

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Used to go through all the slots in an Object (same limitations as sq_next)
    ///
    /// \param iter An iterator being used for going through the slots
    ///
    /// \return Whether there is a next slot
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    bool Next(iterator& iter) const
    {
        sq_pushobject(vm,obj);
        sq_pushinteger(vm,iter.Index);
        if(SQ_SUCCEEDED(sq_next(vm,-2)))
        {
            sq_getstackobj(vm,-1,&iter.Value);
            sq_getstackobj(vm,-2,&iter.Key);
            sq_getinteger(vm,-3,&iter.Index);
            sq_pop(vm,4);
            return true;
        }
        else
        {
            sq_pop(vm,2);
            return false;
        }
    }

protected:
/// @cond DEV

    // Bind a function and it's associated Squirrel closure to the object
    inline void BindFunc(const SQChar* name, void* method, size_t methodSize, SQFUNCTION<Q> func, bool staticVar = false) {
        sq_pushobject(vm, GetObject());
        sq_pushstring(vm, name, -1);

        SQUserPointer methodPtr = sq_newuserdata(vm, static_cast<SQUnsignedInteger>(methodSize));
        memcpy(methodPtr, method, methodSize);

        sq_newclosure(vm, func, 1);
        sq_newslot(vm, -3, staticVar);
        sq_pop(vm,1); // pop table
    }

    inline void BindFunc(const SQInteger index, void* method, size_t methodSize, SQFUNCTION<Q> func, bool staticVar = false) {
        sq_pushobject(vm, GetObject());
        sq_pushinteger(vm, index);

        SQUserPointer methodPtr = sq_newuserdata(vm, static_cast<SQUnsignedInteger>(methodSize));
        memcpy(methodPtr, method, methodSize);

        sq_newclosure(vm, func, 1);
        sq_newslot(vm, -3, staticVar);
        sq_pop(vm,1); // pop table
    }


    // Bind a function and it's associated Squirrel closure to the object
    inline void BindOverload(const SQChar* name, void* method, size_t methodSize, SQFUNCTION<Q> func, SQFUNCTION<Q> overload, int argCount, bool staticVar = false) {
        string overloadName = SqOverloadName::Get(name, argCount);

        sq_pushobject(vm, GetObject());

        // Bind overload handler
        sq_pushstring(vm, name, -1);
        sq_pushstring(vm, name, -1); // function name is passed as a free variable
        sq_newclosure(vm, overload, 1);
        sq_newslot(vm, -3, staticVar);

        // Bind overloaded function
        sq_pushstring(vm, overloadName.c_str(), -1);
        SQUserPointer methodPtr = sq_newuserdata(vm, static_cast<SQUnsignedInteger>(methodSize));
        memcpy(methodPtr, method, methodSize);
        sq_newclosure(vm, func, 1);
        sq_newslot(vm, -3, staticVar);

        sq_pop(vm,1); // pop table
    }

    // Set the value of a variable on the object. Changes to values set this way are not reciprocated
    template<class V>
    inline void BindValue(const SQChar* name, const V& val, bool staticVar = false) {
        sq_pushobject(vm, GetObject());
        sq_pushstring(vm, name, -1);
        PushVar(vm, val);
        sq_newslot(vm, -3, staticVar);
        sq_pop(vm,1); // pop table
    }
    template<class V>
    inline void BindValue(const SQInteger index, const V& val, bool staticVar = false) {
        sq_pushobject(vm, GetObject());
        sq_pushinteger(vm, index);
        PushVar(vm, val);
        sq_newslot(vm, -3, staticVar);
        sq_pop(vm,1); // pop table
    }

    template<>
    inline void BindValue<int>(const SQChar* name, const int & val, bool staticVar /*= false*/) {
        sq_pushobject(vm, GetObject());
        sq_pushstring(vm, name, -1);
        PushVar<int>(vm, val);
        sq_newslot(vm, -3, staticVar);
        sq_pop(vm, 1); // pop table
    }

    // Set the value of an instance on the object. Changes to values set this way are reciprocated back to the source instance
    template<class V>
    inline void BindInstance(const SQChar* name, V* val, bool staticVar = false) {
        sq_pushobject(vm, GetObject());
        sq_pushstring(vm, name, -1);
        PushVar(vm, val);
        sq_newslot(vm, -3, staticVar);
        sq_pop(vm,1); // pop table
    }
    template<class V>
    inline void BindInstance(const SQInteger index, V* val, bool staticVar = false) {
        sq_pushobject(vm, GetObject());
        sq_pushinteger(vm, index);
        PushVar(vm, val);
        sq_newslot(vm, -3, staticVar);
        sq_pop(vm,1); // pop table
    }

/// @endcond
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Used to get and push Object instances to and from the stack as references (Object is always a reference)
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template<Squirk Q>
struct Var<Object<Q>, Q> {

    Object<Q> value; ///< The actual value of get operations

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Attempts to get the value off the stack at idx as an Object
    ///
    /// \param vm  Target VM
    /// \param idx Index trying to be read
    ///
    /// \remarks
    /// This function MUST have its Error handled if it occurred.
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    Var(HSQUIRRELVM<Q> vm, SQInteger idx) {
        HSQOBJECT<Q> sqValue;
        sq_getstackobj(vm, idx, &sqValue);
        value = Object<Q>(sqValue, vm);
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Called by Sqrat::PushVar to put an Object on the stack
    ///
    /// \param vm    Target VM
    /// \param value Value to push on to the VM's stack
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static void push(HSQUIRRELVM<Q> vm, const Object<Q>& value) {
        sq_pushobject(vm, value.GetObject());
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Used to get and push Object instances to and from the stack as references (Object is always a reference)
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template<Squirk Q>
struct Var<Object<Q>&, Q> : Var<Object<Q>, Q> {Var(HSQUIRRELVM<Q> vm, SQInteger idx) : Var<Object<Q>, Q>(vm, idx) {}};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Used to get and push Object instances to and from the stack as references (Object is always a reference)
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template<Squirk Q>
struct Var<const Object<Q>&, Q> : Var<Object<Q>, Q> {Var(HSQUIRRELVM<Q> vm, SQInteger idx) : Var<Object<Q>, Q>(vm, idx) {}};

}

#endif
