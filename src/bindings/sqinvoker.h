#pragma once

#include "sqpcheader.h"
#include "sqvm.h"
#include "sqrat.h"

#include <cctype>
#include <string>

template <Squirk Q>
class SQInvoker
{
public:
	SQInvoker(const SQChar *name)
	{
		Sqrat::RootTable root = Sqrat::RootTable<Q>();
		m_instance = root.GetSlot(name);
	}

	SQInvoker(const HSQOBJECT<Q> &obj)
	{
		m_instance = obj;
	}

	inline HSQOBJECT<Q> GetInstance()
	{
		return m_instance;
	}

	inline void SetInstance(HSQOBJECT<Q> instance)
	{
		m_instance = instance;
	}

protected:
	inline Sqrat::Function<Q> Function(const char *function)
	{
		std::string name(function);
		name[0] = tolower(name[0]);
		return m_instance.GetFunction(name.c_str());
	}

	template<typename Return, typename ... Args>
	inline Return Invoke(Sqrat::Function<Q> function, Args ... args)
	{
		constexpr std::size_t count = sizeof...(Args);
		if constexpr (std::is_void_v<Return>) {
			if (function.IsNull()) return;
			if constexpr (count == 0) return function.Execute();
			else return function.Execute<Args ...>(args ...);
		} else {
			if (function.IsNull()) return {};
			Sqrat::SharedPtr<Return> ret;
			if constexpr (count == 0) ret = function.Evaluate<Return>();
			else ret = function.Evaluate<Return, Args ...>(args ...);
			if (!ret) return {};
			return *ret;
		}
	}

	template<typename Return, typename ... Args>
	inline Return Invoke(const char *function, Args ... args)
	{
		return Invoke<Return>(Function(function), args ...);
	}

protected:
	Sqrat::Table<Q> m_instance;
};

template SQInvoker<Squirk::Standard>;
template SQInvoker<Squirk::AlignObject>;
template SQInvoker<Squirk::StandardShared>;
template SQInvoker<Squirk::AlignObjectShared>;
