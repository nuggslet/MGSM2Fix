#pragma once

#include "sqinvoker.h"

template <Squirk Q>
using RSCFUNCTION = void (*)(Sqrat::Object<Q> resource, std::any data);

template <Squirk Q>
class SQResource : public SQInvoker<Q>
{
public:
	SQResource() : SQInvoker<Q>(_SC("Resource")) {
		HSQUIRRELVM<Q> v = Sqrat::DefaultVM<Q>::Get();
		SQObjectPtr<Q> obj = {}, ctor = {};
		obj = this->m_instance.GetObject();
		v->CreateClassInstance(_class(obj), obj, ctor);
		this->SetInstance(obj);
		this->Invoke<void>("constructor");
	}

	static auto & GetInstance() {
		static SQResource instance;
		return instance;
	}

	void Load(std::string path) {
		this->Invoke<void>(__func__, path);
	}

	void Unload() {
		this->Invoke<void>(__func__);
	}

	void Load(std::string path, RSCFUNCTION<Q> function, std::any data = {}) {
		ResourceTable.insert({ path, { function, data } });
		if (ResourceTable.size() == 1) Load(path);
	}

	bool GetLoading() {
		return this->Invoke<bool>(__func__);
	}

	Sqrat::Object<Q> Find(std::string path) {
		return this->Invoke<Sqrat::Object<Q>>(__func__, path);
	}

	void Poll() {
		if (GetLoading()) return;

		for (auto it = ResourceTable.begin(); it != ResourceTable.end(); ++it)
		{
			std::string path = it->first;
			RSCFUNCTION<Q> function = it->second.first;
			std::any data = it->second.second;

			auto obj = Find(path);
			if (obj.IsNull()) continue;

			it = ResourceTable.erase(it);
			function(obj, data);

			Unload();

			for (auto it = ResourceTable.begin(); it != ResourceTable.end(); ++it)
			{
				std::string path = it->first;
				Load(path);
				break;
			}

			break;
		}


	}

private:
	std::unordered_multimap<std::string, std::pair<RSCFUNCTION<Q>, std::any>> ResourceTable;
};

template SQResource<Squirk::Standard>;
template SQResource<Squirk::AlignObject>;
template SQResource<Squirk::StandardShared>;
template SQResource<Squirk::AlignObjectShared>;
