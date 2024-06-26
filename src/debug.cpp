#include "stdafx.h"

using namespace std;

typedef struct {
    bool hooked;
    SQChar src[MAX_PATH];
    SQInteger line;
} M2FixData;

template <Squirk T>
void TraceParameter(stringstream &trace, SQObjectPtr<T> obj, int level)
{
    switch (obj._type) {
    case OT_BOOL:
        trace << _integer(obj) ? "true" : "false";
        break;
    case OT_INTEGER:
        trace << _integer(obj);
        break;
    case OT_FLOAT:
        trace << _float(obj);
        break;
    case OT_STRING:
    {
        string str(_string(obj)->_val, _string(obj)->_len);
        str.erase(remove(str.begin(), str.end(), '\n'), str.cend());
        trace << "\"" << str.c_str() << "\"";
        break;
    }
    case OT_ARRAY:
    {
        int i = 0;
        SQObjectPtr<T> ent;
        trace << "[";
        if (level >= 1) {
            while (_array(obj)->Get(i++, ent)) {
                TraceParameter<T>(trace, ent, level - 1);
                if (i != _array(obj)->_values.size()) trace << ", ";
            }
        }
        trace << "]";
        break;
    }
    case OT_TABLE:
    {
        SQObjectPtr<T> i = SQObjectPtr<T>(SQInteger(0));
        SQObjectPtr<T> key, value;
        trace << "{";
        if (level >= 1) {
            while (_table(obj)->Next(false, i, key, value) > 0) {
                TraceParameter<T>(trace, key, level - 1);
                trace << ": ";
                TraceParameter<T>(trace, value, level - 1);
                i = SQObjectPtr<T>(_integer(i) + 1);
                if (_table(obj)->Next(false, i, key, value) > 0) trace << ", ";
            }
        }
        trace << "}";
        break;
    }
    case OT_CLASS:
        trace << hex << "C" << (uintptr_t)_class(obj)->_typetag << dec;
        if (_class(obj)->_base) {
            trace << "::";
            TraceParameter<T>(trace, _class(obj)->_base, level);
        }
        break;
    case OT_INSTANCE:
        trace << hex << "I" << (uintptr_t)_instance(obj)->_userpointer << dec;
        if (_instance(obj)->_class) {
            trace << "?";
            TraceParameter<T>(trace, _instance(obj)->_class, level);
        }
        break;
    case OT_CLOSURE:
    {
        SQFunctionProto<T> *proto = _funcproto(_closure(obj)->_function);
        if (proto && sq_isstring(proto->_name)) {
            trace << _stringval(proto->_name);
        }
        trace << "()";
        break;
    }
    case OT_NATIVECLOSURE:
        if (sq_isstring(_nativeclosure(obj)->_name))
            trace << _stringval(_nativeclosure(obj)->_name);
        trace << hex << "{" << "0x" << _nativeclosure(obj)->_function << "}()" << dec;
        break;
    case OT_USERDATA:
        trace << "(udat *) " << _userdata(obj);
        break;
    case OT_USERPOINTER:
        trace << "(uptr *) " << _userpointer(obj);
        break;
    case OT_GENERATOR:
        trace << "G" << _generator(obj);
        break;
    case OT_THREAD:
        trace << "T" << _thread(obj);
        break;
    case OT_WEAKREF:
        trace << "&" << _weakref(obj);
        break;
    case OT_FUNCPROTO:
        trace << "def " << _funcproto(obj);
        break;
    case OT_NULL:
        trace << "null";
        break;
    default:
        trace << hex << "OT_" << obj._type << "<" << obj._unVal.raw << ">" << dec;
        break;
    }
}

template void TraceParameter(stringstream &trace, SQObjectPtr<Squirk::Standard> obj, int level);
template void TraceParameter(stringstream &trace, SQObjectPtr<Squirk::AlignObject> obj, int level);

template <Squirk T>
void TraceNext(stringstream &trace, HSQUIRRELVM<T> v)
{
    sq_pushnull(v);
    SQRESULT res = sq_next(v, -2);
    while (SQ_SUCCEEDED(res))
    {
        HSQOBJECT<T> key; sq_getstackobj(v, -2, &key);
        HSQOBJECT<T> value; sq_getstackobj(v, -1, &value);

        TraceParameter<T>(trace, key, 1);
        trace << ": ";

        if (sq_isinstance(value)) {
            trace << "{";
            TraceNext<T>(trace, v);
            trace << "}";
        }
        else {
            TraceParameter<T>(trace, value, 1);
        }

        sq_pop(v, 2);

        res = sq_next(v, -2);
        if (SQ_SUCCEEDED(res)) trace << ", ";
    }

    sq_pop(v, 1);
}

template void TraceNext(stringstream &trace, HSQUIRRELVM<Squirk::Standard> v);
template void TraceNext(stringstream &trace, HSQUIRRELVM<Squirk::AlignObject> v);

template <Squirk T>
void TraceNative(HSQUIRRELVM<T> v, SQFUNCTION<T> func, SQNativeClosure<T> *closure, const SQChar *name)
{
    stringstream trace;

    M2FixData* data = (M2FixData *) sq_getforeignptr(v);
    if (data && data->src[0] != 0) {
        trace << data->src << ":" << data->line << " -> ";
        data->src[0] = 0;
    }

    if (name) trace << name;
    trace << hex << "{" << "0x" << func << "}(" << dec;
    trace << hex << "0x" << v << dec;

    if (closure) {
        extern int iNativeLevel;
        if (iNativeLevel >= 2) {
            for (SQInteger i = 0; i < closure->_typecheck.size(); i++) {
                if (i == 0) trace << ", ";
                SQObjectPtr obj = v->_stack._vals[v->_stackbase + i];
                TraceParameter(trace, obj, iNativeLevel - 2);
                if ((i + 1) < closure->_typecheck.size()) trace << ", ";
            }
        }
    }

    trace << ")";

    string tracestring = trace.str();
    LOG_F(INFO, "Squirrel: CallNative: %s", tracestring.c_str());
}

template void TraceNative(HSQUIRRELVM<Squirk::Standard> v, SQFUNCTION<Squirk::Standard> func, SQNativeClosure<Squirk::Standard> *closure, const SQChar *name);
template void TraceNative(HSQUIRRELVM<Squirk::AlignObject> v, SQFUNCTION<Squirk::AlignObject> func, SQNativeClosure<Squirk::AlignObject> *closure, const SQChar *name);

template <Squirk T>
void Trace(HSQUIRRELVM<T> v)
{
    SQUserPointer up;
    SQInteger event_type, line;
    const SQChar *src = NULL, *func = NULL;
    sq_getinteger(v, 2, &event_type);
    if (sq_gettype(v, 3) == OT_STRING) sq_getstring(v, 3, &src);
    sq_getinteger(v, 4, &line);
    if (sq_gettype(v, 5) == OT_STRING) sq_getstring(v, 5, &func);

    auto &my = v->_callsstack[v->_callsstacksize - 1];
    auto &ci = v->_callsstack[v->_callsstacksize - 2];

    if (event_type == _SC('c') || event_type == _SC('r')) {
        SQClosure<T> *closure = _closure(ci._closure);
        if (closure) {
            SQFunctionProto<T> *proto = _funcproto(closure->_function);
            if (proto && sq_isstring(proto->_name)) {
                stringstream trace;

                trace << (event_type == _SC('c') ? "Call" : "Return") << ": " << src << ":" << line;
                trace << " " << (event_type == _SC('c') ? "->" : "<-") << " " << func;

                trace << "(";
                extern int iLevel;
                if (iLevel >= 2) {
                    for (SQInteger i = 0; i < proto->_nparameters; i++) {
                        SQObjectPtr obj = v->_stack._vals[v->_stackbase - my._prevstkbase + i];
                        TraceParameter(trace, obj, iLevel - 2);
                        if ((i + 1) < proto->_nparameters) trace << ", ";
                    }
                }
                trace << ")";

                if (event_type == _SC('r')) {
                    SQInteger i = ci._target; // what the fuck?
                    if (ci._ip[-1].op == _OP_RETURN) i = ci._ip[-1]._arg1;
                    if (ci._ip[-1].op == _OP_YIELD)  i = ci._ip[-1]._arg1;

                    SQObjectPtr obj = v->_stack._vals[v->_stackbase - my._prevstkbase + i];
                    trace << " -> ";
                    TraceParameter(trace, obj, iLevel - 2);
                }

                string tracestring = trace.str();
                LOG_F(INFO, "Squirrel: %s", tracestring.c_str());
            }
        }
    }
    else if (event_type == _SC('l')) {
        LOG_F(INFO, "Squirrel: Line: %s:%d", src, line);
    }
}

template void Trace(HSQUIRRELVM<Squirk::Standard> v);
template void Trace(HSQUIRRELVM<Squirk::AlignObject> v);
