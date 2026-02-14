#include "stdafx.h"
#include "warning_background_shuffle.hpp"

#include "m2utils.h"

void BackgroundShuffleWarning::Check()
{
	if (M2Utils::IsSteamOS()) {
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
		const char* message =
			"Warning:\n\n"
			"Having Windows wallpaper set to Slideshow / Windows Spotlight mode is known to cause stuttering while in DirectX games.\n"
	        "\n"
			"If you experience intermittent stuttering, change your wallpaper to a static picture in your personalization settings.";

		MessageBoxA(nullptr, message, "Performance Warning", MB_ICONWARNING | MB_OK);
	}
}
