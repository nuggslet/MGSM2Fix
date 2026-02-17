#include "m2fix.h"
#include "sqhook.h"

template <Squirk Q>
void SQHook<Q>::TraceParameter(std::stringstream &trace, SQObjectPtr<Q> obj, int level)
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
            std::string str(_string(obj)->_val, _string(obj)->_len);
            str.erase(remove(str.begin(), str.end(), '\n'), str.cend());
            trace << "\"" << str << "\"";
            break;
        }
        case OT_ARRAY:
        {
            int i = 0;
            SQObjectPtr<Q> ent;
            trace << "[";
            if (level >= 1) {
                while (_array(obj)->Get(i++, ent)) {
                    TraceParameter(trace, ent, level - 1);
                    if (i != _array(obj)->_values.size()) trace << ", ";
                }
            }
            trace << "]";
            break;
        }
        case OT_TABLE:
        {
            SQObjectPtr<Q> i = SQObjectPtr<Q>(SQInteger(0));
            SQObjectPtr<Q> key, value;
            trace << "{";
            if (level >= 1) {
                while (_table(obj)->Next(false, i, key, value) > 0) {
                    TraceParameter(trace, key, level - 1);
                    trace << ": ";
                    TraceParameter(trace, value, level - 1);
                    i = SQObjectPtr<Q>(_integer(i) + 1);
                    if (_table(obj)->Next(false, i, key, value) > 0) trace << ", ";
                }
            }
            trace << "}";
            break;
        }
        case OT_CLASS:
        {
            std::string classname;
            Sqrat::RootTable root = Sqrat::RootTable<Q>();
            for (auto &name : ClassNames) {
                Sqrat::Object<Q> object = root.GetSlot(name.c_str());
                if (object.GetType() != OT_CLASS) continue;
                if (!_instance(obj)->InstanceOf(_class(object.GetObject()))) continue;
                classname = name;
                break;
            }

            if (!classname.empty()) {
                trace << classname << "{";
            }
            trace << std::hex << "C" << (uintptr_t)_class(obj)->_typetag << std::dec;
            if (!classname.empty()) {
                trace << "}";
            }

            if (_class(obj)->_base) {
                trace << "::";
                TraceParameter(trace, _class(obj)->_base, level);
            }
            break;
        }
        case OT_INSTANCE:
            trace << std::hex << "I" << (uintptr_t)_instance(obj)->_userpointer << std::dec;
            if (_instance(obj)->_class) {
                trace << "?";
                TraceParameter(trace, _instance(obj)->_class, level);
            }
            break;
        case OT_CLOSURE:
        {
            SQFunctionProto<Q> *proto = _funcproto(_closure(obj)->_function);
            if (proto && sq_isstring(proto->_name)) {
                trace << _stringval(proto->_name);
            }
            trace << "()";
            break;
        }
        case OT_NATIVECLOSURE:
            if (sq_isstring(_nativeclosure(obj)->_name))
                trace << _stringval(_nativeclosure(obj)->_name);
            trace << std::hex << "{" << "0x" << _nativeclosure(obj)->_function << "}()" << std::dec;
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
            trace << std::hex << "OT_" << obj._type << "<" << obj._unVal.raw << ">" << std::dec;
            break;
    }
}

template void SQHook<Squirk::Standard>::TraceParameter(std::stringstream &trace, SQObjectPtr<Squirk::Standard> obj, int level);
template void SQHook<Squirk::AlignObject>::TraceParameter(std::stringstream &trace, SQObjectPtr<Squirk::AlignObject> obj, int level);
template void SQHook<Squirk::StandardShared>::TraceParameter(std::stringstream &trace, SQObjectPtr<Squirk::StandardShared> obj, int level);
template void SQHook<Squirk::AlignObjectShared>::TraceParameter(std::stringstream &trace, SQObjectPtr<Squirk::AlignObjectShared> obj, int level);

template <Squirk Q>
void SQHook<Q>::TraceNext(std::stringstream &trace, HSQUIRRELVM<Q> v)
{
    sq_pushnull(v);
    SQRESULT res = sq_next(v, -2);
    while (SQ_SUCCEEDED(res))
    {
        HSQOBJECT<Q> key; sq_getstackobj(v, -2, &key);
        HSQOBJECT<Q> value; sq_getstackobj(v, -1, &value);

        TraceParameter(trace, key, 1);
        trace << ": ";

        if (sq_isinstance(value)) {
            trace << "{";
            TraceNext(trace, v);
            trace << "}";
        }
        else {
            TraceParameter(trace, value, 1);
        }

        sq_pop(v, 2);

        res = sq_next(v, -2);
        if (SQ_SUCCEEDED(res)) trace << ", ";
    }

    sq_pop(v, 1);
}

template void SQHook<Squirk::Standard>::TraceNext(std::stringstream &trace, HSQUIRRELVM<Squirk::Standard> v);
template void SQHook<Squirk::AlignObject>::TraceNext(std::stringstream &trace, HSQUIRRELVM<Squirk::AlignObject> v);
template void SQHook<Squirk::StandardShared>::TraceNext(std::stringstream &trace, HSQUIRRELVM<Squirk::StandardShared> v);
template void SQHook<Squirk::AlignObjectShared>::TraceNext(std::stringstream &trace, HSQUIRRELVM<Squirk::AlignObjectShared> v);

template <Squirk Q>
void SQHook<Q>::TraceNative(HSQUIRRELVM<Q> v, SQFUNCTION<Q> func, SQNativeClosure<Q> *closure, const SQChar *name)
{
    std::stringstream trace;

    M2FixData<Q> *data = EnsureFixData(v);
    if (data && !data->src.empty()) {
        trace << data->src << ":" << data->line << " -> ";
        data->src.clear();
    }

    if (name) trace << name;
    trace << std::hex << "{" << "0x" << func << "}(" << std::dec;
    trace << std::hex << "0x" << v << std::dec;

    if (closure) {
        if (M2Config::iNativeLevel >= 2) {
            for (SQUnsignedInteger i = 0; i < closure->_typecheck.size(); i++) {
                if (i == 0) trace << ", ";
                SQObjectPtr obj = v->_stack._vals[v->_stackbase + i];
                TraceParameter(trace, obj, M2Config::iNativeLevel - 2);
                if ((i + 1) < closure->_typecheck.size()) trace << ", ";
            }
        }
    }

    trace << ")";

    spdlog::info("[SQ] CallNative: {}", trace.str());
}

template void SQHook<Squirk::Standard>::TraceNative(HSQUIRRELVM<Squirk::Standard> v, SQFUNCTION<Squirk::Standard> func, SQNativeClosure<Squirk::Standard> *closure, const SQChar *name);
template void SQHook<Squirk::AlignObject>::TraceNative(HSQUIRRELVM<Squirk::AlignObject> v, SQFUNCTION<Squirk::AlignObject> func, SQNativeClosure<Squirk::AlignObject> *closure, const SQChar *name);
template void SQHook<Squirk::StandardShared>::TraceNative(HSQUIRRELVM<Squirk::StandardShared> v, SQFUNCTION<Squirk::StandardShared> func, SQNativeClosure<Squirk::StandardShared> *closure, const SQChar *name);
template void SQHook<Squirk::AlignObjectShared>::TraceNative(HSQUIRRELVM<Squirk::AlignObjectShared> v, SQFUNCTION<Squirk::AlignObjectShared> func, SQNativeClosure<Squirk::AlignObjectShared> *closure, const SQChar *name);

template <Squirk Q>
void SQHook<Q>::Trace(HSQUIRRELVM<Q> v)
{
    SQInteger event_type, line;
    const SQChar *src = NULL, *func = NULL;
    sq_getinteger(v, 2, &event_type);
    if (sq_gettype(v, 3) == OT_STRING) sq_getstring(v, 3, &src);
    sq_getinteger(v, 4, &line);
    if (sq_gettype(v, 5) == OT_STRING) sq_getstring(v, 5, &func);

    auto &my = v->_callsstack[v->_callsstacksize - 1];
    auto &ci = v->_callsstack[v->_callsstacksize - 2];

    if (event_type == _SC('c') || event_type == _SC('r')) {
        SQClosure<Q> *closure = _closure(ci._closure);
        if (closure) {
            SQFunctionProto<Q> *proto = _funcproto(closure->_function);
            if (proto && sq_isstring(proto->_name)) {
                std::stringstream trace;

                trace << (event_type == _SC('c') ? "Call" : "Return") << ": " << src << ":" << line;
                trace << " " << (event_type == _SC('c') ? "->" : "<-") << " " << func;

                trace << "(";
                if (M2Config::iLevel >= 2) {
                    for (SQInteger i = 0; i < proto->_nparameters; i++) {
                        SQObjectPtr<Q> obj = v->_stack._vals[v->_stackbase - my._prevstkbase + i];
                        TraceParameter(trace, obj, M2Config::iLevel - 2);
                        if ((i + 1) < proto->_nparameters) trace << ", ";
                    }
                }
                trace << ")";

                if (event_type == _SC('r')) {
                    SQInteger i = ci._target; // what the fuck?
                    if (ci._ip[-1].op == _OP_RETURN) i = ci._ip[-1]._arg1;
                    if (ci._ip[-1].op == _OP_YIELD)  i = ci._ip[-1]._arg1;

                    SQObjectPtr<Q> obj = v->_stack._vals[v->_stackbase - my._prevstkbase + i];
                    trace << " -> ";
                    TraceParameter(trace, obj, M2Config::iLevel - 2);
                }

                spdlog::info("[SQ] {}", trace.str());
            }
        }
    }
    else if (event_type == _SC('l')) {
        spdlog::info("[SQ] Line: {}:{}", src, line);
    }
}

template void SQHook<Squirk::Standard>::Trace(HSQUIRRELVM<Squirk::Standard> v);
template void SQHook<Squirk::AlignObject>::Trace(HSQUIRRELVM<Squirk::AlignObject> v);
template void SQHook<Squirk::StandardShared>::Trace(HSQUIRRELVM<Squirk::StandardShared> v);
template void SQHook<Squirk::AlignObjectShared>::Trace(HSQUIRRELVM<Squirk::AlignObjectShared> v);
