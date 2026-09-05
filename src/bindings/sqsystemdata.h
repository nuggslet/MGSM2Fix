#pragma once

#include "sqinvoker.h"
#include "sqhelper.h"

class SystemDataSettingEtc {

};

template <Squirk Q>
class SQSystemData : public SQInvoker<Q>
{
public:
	enum class Index {
		SETTING_PAD,
		SETTING_SCREEN,
		SETTING_SOUND,
		SETTING_SAVEDATA,
		SETTING_ETC,
		SETTING_NETWORK,
		SETTING_GAME,
		BACKUP_FLAGS,
		WINDOW_INFO,
		PURCHASE_RECORD,
		WORK_MEDAL,
		WORK_TRIAL,
		SRAM_DATA,
		WORK_TITLE,
	};

	class SettingPad : public SQInvoker<Q>
	{
	public:
		SettingPad() : SQInvoker<Q>(_SC("g_systemdata")) {
			auto obj = this->Invoke<Sqrat::Object<Q>>("get_value", static_cast<int>(Index::SETTING_PAD));
			this->SetInstance(obj.GetObject());
		}

		static int GetPlaySide_MGS1() {
			return SettingPad().Invoke<int>("get_playside_mgs");
		}

		static void SetPlaySide_MGS1(int side) {
			return SettingPad().Invoke<void>("set_playside_mgs", side);
		}
	};

	class SettingScreen : public SQInvoker<Q>
	{
	public:
		enum class SizeAuto {
			NORMAL,
			ORIGINAL_ONLY_ONE,
			FIT,
			FULL,
			ORIGINAL,
			MANUAL,
			CUSTOM,
			FIT_WIDE,
		};

		SettingScreen() : SQInvoker<Q>(_SC("g_systemdata")) {
			auto obj = this->Invoke<Sqrat::Object<Q>>("get_value", static_cast<int>(Index::SETTING_SCREEN));
			this->SetInstance(obj.GetObject());
		}

		static SizeAuto GetSizeAuto() {
			return static_cast<SizeAuto>(SettingScreen().Invoke<int>("get_size_auto"));
		}

		static bool GetSmoothing() {
			return SettingScreen().Invoke<bool>("get_smoothing");
		}
	};

	class SettingETC : public SQInvoker<Q>
	{
	public:
		SettingETC() : SQInvoker<Q>(_SC("g_systemdata")) {
			auto obj = this->Invoke<Sqrat::Object<Q>>("get_value", static_cast<int>(Index::SETTING_ETC));
			this->SetInstance(obj.GetObject());
		}

		static std::string GetVersion() {
			return SettingETC().Invoke<std::string>("get_game_regionTag");
		}
	};

	SQSystemData() : SQInvoker<Q>(_SC("g_systemdata")) {}
};

template SQSystemData<Squirk::Standard>;
template SQSystemData<Squirk::AlignObject>;
template SQSystemData<Squirk::StandardShared>;
template SQSystemData<Squirk::AlignObjectShared>;
