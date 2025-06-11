#pragma once

#include "sqrat.h"

template <Squirk Q>
class SQHelper
{
public:
	SQHelper() {}

	template <typename T>
	static std::vector<T> MakeVector(Sqrat::Array<Q> &array)
	{
		std::vector<T> vector(array.Length());
		array.GetArray(vector.data(), vector.size());
		return vector;
	}

	template <typename T>
	static std::vector<T> MakeVector(Sqrat::Object<Q> &object)
	{
		if (object.GetType() != OT_ARRAY) return {};
		auto & array = static_cast<Sqrat::Array<Q> &>(object);
		return MakeVector<T>(array);
	}

	template <typename T>
	static Sqrat::Array<Q> MakeArray(const std::vector<T> &vector)
	{
		Sqrat::Array<Q> array;
		for (auto & item : vector) array.Append(item);
		return array;
	}

	// DO NOT USE e,g. `char` / `unsigned char` here. Use `int` / `unsigned int`.
	static Sqrat::Object<Q> GetObject(SQInteger idx, HSQUIRRELVM<Q> v = Sqrat::DefaultVM<Q>::Get())
	{
		HSQOBJECT<Q> object;
		sq_getstackobj(v, idx, &object);
		return Sqrat::Object<Q>(object);
	}

	template <typename T>
	static T *AcquireForeignObject(HSQUIRRELVM<Q> v = Sqrat::DefaultVM<Q>::Get())
	{
		if (!sq_getforeignptr(v)) {
			sq_setforeignptr(v, new T {});
		}
		return reinterpret_cast<T *>(sq_getforeignptr(v));
	}
};

template SQHelper<Squirk::Standard>;
template SQHelper<Squirk::AlignObject>;
template SQHelper<Squirk::StandardShared>;
template SQHelper<Squirk::AlignObjectShared>;
