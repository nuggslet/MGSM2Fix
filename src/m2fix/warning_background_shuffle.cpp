#include "stdafx.h"
#include "warning_background_shuffle.hpp"

#include "m2fix.h"
#include "m2utils.h"
#include "m2config.h"

void BackgroundShuffleWarning::Check()
{
	if (M2Utils::IsSteamOS()) {
		return;
	}

	if (M2Config::bDisableWindowsSlideshowWarning) {
		return;
	}

	HKEY hKey;
	DWORD value = 0;
	DWORD size = sizeof(value);

	if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Wallpapers", 0, KEY_READ, &hKey) != ERROR_SUCCESS) {
		return; // key doesn't exist, bail
	}

	LSTATUS status = RegQueryValueExW(hKey, L"BackgroundType", nullptr, nullptr, reinterpret_cast<LPBYTE>(&value), &size);
	RegCloseKey(hKey);

	if (status == ERROR_SUCCESS && value > 1) {
		std::string title = fmt::format(
			"{}: Performance Warning",
			M2Fix::FixName()
		);

		std::string message = fmt::format(
			"Having Windows wallpaper set to Slideshow / Windows Spotlight mode is known to cause stuttering while in DirectX games."
	        "\n\n"
			"If you experience intermittent stuttering, change your wallpaper to a static picture in your personalization settings."
			"\n\n"
			"If you want to disable this warning, edit {}.",
			M2Fix::ConfigFile()
		);

		MessageBoxA(nullptr, message.c_str(), title.c_str(), MB_ICONWARNING | MB_OK);
	}
}
