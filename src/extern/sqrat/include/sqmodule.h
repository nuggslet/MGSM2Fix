//
// SqModule: API used to communicate with and register squirrel modules
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

#if !defined(_SQ_MODULE_H_)
#define _SQ_MODULE_H_

#include "squirrel.h"

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// @cond DEV
    /// Allows modules to interface with Squirrel's C api without linking to the squirrel library
    /// If new functions are added to the Squirrel API, they should be added here too
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    template <Squirk Q>
    struct sq_api {
        /*vm*/
        HSQUIRRELVM<Q>     (*open)(SQInteger initialstacksize);
        HSQUIRRELVM<Q>     (*newthread)(HSQUIRRELVM<Q> friendvm, SQInteger initialstacksize);
        void               (*seterrorhandler)(HSQUIRRELVM<Q> v);
        void               (*close)(HSQUIRRELVM<Q> v);
        void               (*setforeignptr)(HSQUIRRELVM<Q> v,SQUserPointer p);
        SQUserPointer      (*getforeignptr)(HSQUIRRELVM<Q> v);
#if SQUIRREL_VERSION_NUMBER >= 300
        void               (*setprintfunc)(HSQUIRRELVM<Q> v, SQPRINTFUNCTION<Q> printfunc, SQPRINTFUNCTION<Q>);
#else
        void               (*setprintfunc)(HSQUIRRELVM<Q> v, SQPRINTFUNCTION<Q> printfunc);
#endif
        SQPRINTFUNCTION<Q> (*getprintfunc)(HSQUIRRELVM<Q> v);
        SQRESULT           (*suspendvm)(HSQUIRRELVM<Q> v);
        SQRESULT           (*wakeupvm)(HSQUIRRELVM<Q> v,SQBool resumedret,SQBool retval,SQBool raiseerror,SQBool throwerror);
        SQInteger          (*getvmstate)(HSQUIRRELVM<Q> v);

        /*compiler*/
        SQRESULT           (*compile)(HSQUIRRELVM<Q> v,SQLEXREADFUNC read,SQUserPointer p,const SQChar *sourcename,SQBool raiseerror);
        SQRESULT           (*compilebuffer)(HSQUIRRELVM<Q> v,const SQChar *s,SQInteger size,const SQChar *sourcename,SQBool raiseerror);
        void               (*enabledebuginfo)(HSQUIRRELVM<Q> v, SQBool enable);
        void               (*notifyallexceptions)(HSQUIRRELVM<Q> v, SQBool enable);
        void               (*setcompilererrorhandler)(HSQUIRRELVM<Q> v,SQCOMPILERERROR<Q> f);

        /*stack operations*/
        void               (*push)(HSQUIRRELVM<Q> v,SQInteger idx);
        void               (*pop)(HSQUIRRELVM<Q> v,SQInteger nelemstopop);
        void               (*poptop)(HSQUIRRELVM<Q> v);
        void               (*remove)(HSQUIRRELVM<Q> v,SQInteger idx);
        SQInteger          (*gettop)(HSQUIRRELVM<Q> v);
        void               (*settop)(HSQUIRRELVM<Q> v,SQInteger newtop);
#if SQUIRREL_VERSION_NUMBER >= 300
        SQRESULT           (*reservestack)(HSQUIRRELVM<Q> v,SQInteger nsize);
#else
        void               (*reservestack)(HSQUIRRELVM<Q> v,SQInteger nsize);
#endif
        SQInteger          (*cmp)(HSQUIRRELVM<Q> v);
        void               (*move)(HSQUIRRELVM<Q> dest,HSQUIRRELVM<Q> src,SQInteger idx);

        /*object creation handling*/
        SQUserPointer      (*newuserdata)(HSQUIRRELVM<Q> v,SQUnsignedInteger size);
        void               (*newtable)(HSQUIRRELVM<Q> v);
        void               (*newarray)(HSQUIRRELVM<Q> v,SQInteger size);
        void               (*newclosure)(HSQUIRRELVM<Q> v,SQFUNCTION<Q> func,SQUnsignedInteger nfreevars);
        SQRESULT           (*setparamscheck)(HSQUIRRELVM<Q> v,SQInteger nparamscheck,const SQChar *typemask);
        SQRESULT           (*bindenv)(HSQUIRRELVM<Q> v,SQInteger idx);
        void               (*pushstring)(HSQUIRRELVM<Q> v,const SQChar *s,SQInteger len);
        void               (*pushfloat)(HSQUIRRELVM<Q> v,SQFloat f);
        void               (*pushinteger)(HSQUIRRELVM<Q> v,SQInteger n);
        void               (*pushbool)(HSQUIRRELVM<Q> v,SQBool b);
        void               (*pushuserpointer)(HSQUIRRELVM<Q> v,SQUserPointer p);
        void               (*pushnull)(HSQUIRRELVM<Q> v);
        SQObjectType       (*gettype)(HSQUIRRELVM<Q> v,SQInteger idx);
        SQInteger          (*getsize)(HSQUIRRELVM<Q> v,SQInteger idx);
        SQRESULT           (*getbase)(HSQUIRRELVM<Q> v,SQInteger idx);
        SQBool             (*instanceof)(HSQUIRRELVM<Q> v);
#if SQUIRREL_VERSION_NUMBER >= 300
        SQRESULT           (*tostring)(HSQUIRRELVM<Q> v,SQInteger idx);
#else
        void               (*tostring)(HSQUIRRELVM<Q> v,SQInteger idx);
#endif
        void               (*tobool)(HSQUIRRELVM<Q> v, SQInteger idx, SQBool *b);
        SQRESULT           (*getstring)(HSQUIRRELVM<Q> v,SQInteger idx,const SQChar **c);
        SQRESULT           (*getinteger)(HSQUIRRELVM<Q> v,SQInteger idx,SQInteger *i);
        SQRESULT           (*getfloat)(HSQUIRRELVM<Q> v,SQInteger idx,SQFloat *f);
        SQRESULT           (*getbool)(HSQUIRRELVM<Q> v,SQInteger idx,SQBool *b);
        SQRESULT           (*getthread)(HSQUIRRELVM<Q> v,SQInteger idx,HSQUIRRELVM<Q> *thread);
        SQRESULT           (*getuserpointer)(HSQUIRRELVM<Q> v,SQInteger idx,SQUserPointer *p);
        SQRESULT           (*getuserdata)(HSQUIRRELVM<Q> v,SQInteger idx,SQUserPointer *p,SQUserPointer *typetag);
        SQRESULT           (*settypetag)(HSQUIRRELVM<Q> v,SQInteger idx,SQUserPointer typetag);
        SQRESULT           (*gettypetag)(HSQUIRRELVM<Q> v,SQInteger idx,SQUserPointer *typetag);
        void               (*setreleasehook)(HSQUIRRELVM<Q> v,SQInteger idx,SQRELEASEHOOK hook);
        SQChar*            (*getscratchpad)(HSQUIRRELVM<Q> v,SQInteger minsize);
        SQRESULT           (*getclosureinfo)(HSQUIRRELVM<Q> v,SQInteger idx,SQUnsignedInteger *nparams,SQUnsignedInteger *nfreevars);
        SQRESULT           (*setnativeclosurename)(HSQUIRRELVM<Q> v,SQInteger idx,const SQChar *name);
        SQRESULT           (*setinstanceup)(HSQUIRRELVM<Q> v, SQInteger idx, SQUserPointer p);
        SQRESULT           (*getinstanceup)(HSQUIRRELVM<Q> v, SQInteger idx, SQUserPointer *p,SQUserPointer typetag);
        SQRESULT           (*setclassudsize)(HSQUIRRELVM<Q> v, SQInteger idx, SQInteger udsize);
        SQRESULT           (*newclass)(HSQUIRRELVM<Q> v,SQBool hasbase);
        SQRESULT           (*createinstance)(HSQUIRRELVM<Q> v,SQInteger idx);
        SQRESULT           (*setattributes)(HSQUIRRELVM<Q> v,SQInteger idx);
        SQRESULT           (*getattributes)(HSQUIRRELVM<Q> v,SQInteger idx);
        SQRESULT           (*getclass)(HSQUIRRELVM<Q> v,SQInteger idx);
        void               (*weakref)(HSQUIRRELVM<Q> v,SQInteger idx);
        SQRESULT           (*getdefaultdelegate)(HSQUIRRELVM<Q> v,SQObjectType t);

        /*object manipulation*/
        void               (*pushroottable)(HSQUIRRELVM<Q> v);
        void               (*pushregistrytable)(HSQUIRRELVM<Q> v);
        void               (*pushconsttable)(HSQUIRRELVM<Q> v);
        SQRESULT           (*setroottable)(HSQUIRRELVM<Q> v);
        SQRESULT           (*setconsttable)(HSQUIRRELVM<Q> v);
        SQRESULT           (*newslot)(HSQUIRRELVM<Q> v, SQInteger idx, SQBool bstatic);
        SQRESULT           (*deleteslot)(HSQUIRRELVM<Q> v,SQInteger idx,SQBool pushval);
        SQRESULT           (*set)(HSQUIRRELVM<Q> v,SQInteger idx);
        SQRESULT           (*get)(HSQUIRRELVM<Q> v,SQInteger idx);
        SQRESULT           (*rawget)(HSQUIRRELVM<Q> v,SQInteger idx);
        SQRESULT           (*rawset)(HSQUIRRELVM<Q> v,SQInteger idx);
        SQRESULT           (*rawdeleteslot)(HSQUIRRELVM<Q> v,SQInteger idx,SQBool pushval);
        SQRESULT           (*arrayappend)(HSQUIRRELVM<Q> v,SQInteger idx);
        SQRESULT           (*arraypop)(HSQUIRRELVM<Q> v,SQInteger idx,SQBool pushval);
        SQRESULT           (*arrayresize)(HSQUIRRELVM<Q> v,SQInteger idx,SQInteger newsize);
        SQRESULT           (*arrayreverse)(HSQUIRRELVM<Q> v,SQInteger idx);
        SQRESULT           (*arrayremove)(HSQUIRRELVM<Q> v,SQInteger idx,SQInteger itemidx);
        SQRESULT           (*arrayinsert)(HSQUIRRELVM<Q> v,SQInteger idx,SQInteger destpos);
        SQRESULT           (*setdelegate)(HSQUIRRELVM<Q> v,SQInteger idx);
        SQRESULT           (*getdelegate)(HSQUIRRELVM<Q> v,SQInteger idx);
        SQRESULT           (*clone)(HSQUIRRELVM<Q> v,SQInteger idx);
        SQRESULT           (*setfreevariable)(HSQUIRRELVM<Q> v,SQInteger idx,SQUnsignedInteger nval);
        SQRESULT           (*next)(HSQUIRRELVM<Q> v,SQInteger idx);
        SQRESULT           (*getweakrefval)(HSQUIRRELVM<Q> v,SQInteger idx);
        SQRESULT           (*clear)(HSQUIRRELVM<Q> v,SQInteger idx);

        /*calls*/
        SQRESULT           (*call)(HSQUIRRELVM<Q> v,SQInteger params,SQBool retval,SQBool raiseerror);
        SQRESULT           (*resume)(HSQUIRRELVM<Q> v,SQBool retval,SQBool raiseerror);
        const SQChar*      (*getlocal)(HSQUIRRELVM<Q> v,SQUnsignedInteger level,SQUnsignedInteger idx);
        const SQChar*      (*getfreevariable)(HSQUIRRELVM<Q> v,SQInteger idx,SQUnsignedInteger nval);
        SQRESULT           (*throwerror)(HSQUIRRELVM<Q> v,const SQChar *err);
        void               (*reseterror)(HSQUIRRELVM<Q> v);
        void               (*getlasterror)(HSQUIRRELVM<Q> v);

        /*raw object handling*/
        SQRESULT           (*getstackobj)(HSQUIRRELVM<Q> v,SQInteger idx,HSQOBJECT<Q> *po);
        void               (*pushobject)(HSQUIRRELVM<Q> v,HSQOBJECT<Q> obj);
        void               (*addref)(HSQUIRRELVM<Q> v,HSQOBJECT<Q> *po);
        SQBool             (*release)(HSQUIRRELVM<Q> v,HSQOBJECT<Q> *po);
        void               (*resetobject)(HSQOBJECT<Q> *po);
        const SQChar*      (*objtostring)(HSQOBJECT<Q> *o);
        SQBool             (*objtobool)(HSQOBJECT<Q> *o);
        SQInteger          (*objtointeger)(HSQOBJECT<Q> *o);
        SQFloat            (*objtofloat)(HSQOBJECT<Q> *o);
        SQRESULT           (*getobjtypetag)(HSQOBJECT<Q> *o,SQUserPointer * typetag);

        /*GC*/
        SQInteger          (*collectgarbage)(HSQUIRRELVM<Q> v);

        /*serialization*/
        SQRESULT           (*writeclosure)(HSQUIRRELVM<Q> vm,SQWRITEFUNC writef,SQUserPointer up);
        SQRESULT           (*readclosure)(HSQUIRRELVM<Q> vm,SQREADFUNC readf,SQUserPointer up);

        /*mem allocation*/
        void*              (*malloc)(SQUnsignedInteger size);
        void*              (*realloc)(void* p,SQUnsignedInteger oldsize,SQUnsignedInteger newsize);
        void               (*free)(void *p,SQUnsignedInteger size);

        /*debug*/
        SQRESULT           (*stackinfos)(HSQUIRRELVM<Q> v,SQInteger level,SQStackInfos *si);
        void               (*setdebughook)(HSQUIRRELVM<Q> v);
    };
    template <Squirk Q>
    using HSQAPI = struct sq_api<Q>*;
    /// @endcond

#endif /*_SQ_MODULE_H_*/
