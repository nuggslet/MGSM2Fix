#include "config.h"

#include "m2fix.h"
#include "m2config.h"

// std::string is compatible in Release configuration but it's not great to rely on API/ABI interop between two copies of the standard library.
// This enables Debug configuration and prevents damage that could be done to the game's object until our own copy (into our own std::string) is made.
template<typename T>
class M2InteropBasicString
{
private:
    static constexpr std::size_t BUFSIZE = 16 / sizeof(T);
    union {
        T *ptr;
        T buf[BUFSIZE] = { 0 };
    };
    std::size_t size;
    std::size_t capacity;

public:
    M2InteropBasicString(std::string_view str) {
        std::basic_string<T> s(str);
        std::memset(&buf[0], 0, sizeof(buf));
        size = s.size();
        capacity = s.capacity();
        if (capacity >= BUFSIZE) {
            ptr = new T[capacity];
            std::memset(ptr, 0, capacity * sizeof(T));
            std::memcpy(ptr, s.c_str(), size * sizeof(T));
        }
        else {
            std::memcpy(&buf[0], s.c_str(), size * sizeof(T));
        }
    }
    ~M2InteropBasicString() {
        // This is to detect which variant of the hook is used.
        // For `M2_GetCfgValue`, the string is not ours to free (not our allocator).
        if (M2Fix::GameInfo()->id != -1) return;
        if (capacity >= BUFSIZE) {
            delete[] ptr;
        }
    }
    constexpr const T *c_str() const noexcept {
        if (capacity >= BUFSIZE) {
            return ptr;
        }
        return &buf[0];
    }
};

#ifdef _WIN64
const char *Config::GetCfgValue(uintptr_t *ctx, const M2InteropString id)
#else
const char * __fastcall Config::GetCfgValue(uintptr_t *ctx, uintptr_t _EDX, const M2InteropString id, uintptr_t index)
#endif
{
    std::string key(id.c_str());

    // Game performs one look-up to check the key exists, and another to actually read the value.
    // Keep track of keys we've seen so we don't log the hit twice.
    static std::unordered_set<std::string> keys;
    if (keys.count(key) == 0) {
        spdlog::info("[M2] MWinResCfg::GetValue(\"{}\")", key);
        keys.insert(key);
    }

    if (key == "FULLSCREEN_MODE" || key == "BOOT_FULLSCREEN") {
        return M2Config::sFullscreenMode.c_str();
    }

    if (key == "FULLSCREEN_CURSOR" || key == "MOUSE_CURSOR") {
        return "0";
    }

    if (key == "WINDOW_VSYNC") {
        return "0";
    }

    if (key == "WINDOW_W" || key == "SCREEN_W" || key == "LAST_CLIENT_SIZE_X") {
        return M2Config::sExternalWidth.c_str();
    }

    if (key == "WINDOW_H" || key == "SCREEN_H" || key == "LAST_CLIENT_SIZE_Y") {
        return M2Config::sExternalHeight.c_str();
    }

#ifdef _WIN64
    return M2Hook::GetInstance().Invoke<const char *>(GetCfgValue, ctx, id);
#else
    return M2Hook::GetInstance().Invoke<const char *>(GetCfgValue, ctx, _EDX, id, index);
#endif
}

#ifdef _WIN64
int Config::GetCfgValueEx(uintptr_t *ctx, const M2InteropString *id)
#else
int __fastcall Config::GetCfgValueEx(uintptr_t *ctx, uintptr_t _EDX, const M2InteropString *id)
#endif
{
    std::string key(id->c_str());

    // Free previous allocations - the final request is not overridden so will have the effect of leaving everything freed.
    static std::unordered_set<M2InteropString *> strings;
    static std::unordered_set<std::byte *> buffers;

    for (M2InteropString *result : strings) {
        delete result;
    }
    for (std::byte *buffer : buffers) {
        delete[] buffer;
    }
    strings.clear();
    buffers.clear();

    // If we're here, the above (vastly preferable) hook cannot be done, due to game's optimisation level causing aggressive inlining.
    // This is a fallback approach where instead of returning `const char *` we return an out-of-bounds index into a `std::string` table.
    // Usual caveat around `std::string`: we actually use `M2InteropString` so that our Debug build can be compatible with a Release game.
    M2InteropString *result = nullptr;

    if (key == "FULLSCREEN_MODE" || key == "BOOT_FULLSCREEN") {
        result = new M2InteropString(M2Config::sFullscreenMode.c_str());
    }

    if (key == "FULLSCREEN_CURSOR" || key == "MOUSE_CURSOR") {
        result = new M2InteropString("0");
    }

    if (key == "WINDOW_VSYNC") {
        result = new M2InteropString("0");
    }

    if (key == "WINDOW_W" || key == "SCREEN_W" || key == "LAST_CLIENT_SIZE_X") {
        result = new M2InteropString(M2Config::sExternalWidth.c_str());
    }

    if (key == "WINDOW_H" || key == "SCREEN_H" || key == "LAST_CLIENT_SIZE_Y") {
        result = new M2InteropString(M2Config::sExternalHeight.c_str());
    }

    // Game performs one look-up to check the key exists, and another to actually read the value.
    // Keep track of keys we've seen so we don't log the hit twice.
    static std::unordered_set<std::string> keys;

    if (result) {
        strings.insert(result);

        typedef struct {
            M2InteropString *a;
            M2InteropString *b;
        } M2InteropTable;

        constexpr std::uintptr_t addend = sizeof(M2InteropString);
        constexpr std::uintptr_t stride = addend + (sizeof(std::uintptr_t) * 3);

        // Over-allocate so that we can align to the table stride, to allow returning a whole integer number.
        std::byte *buffer = new std::byte[sizeof(M2InteropTable) + (stride - 1)];
        std::uintptr_t address = reinterpret_cast<std::uintptr_t>(buffer);
        buffers.insert(buffer);

        // Align the allocated address to the stride, thanks to above over-allocation.
        std::uintptr_t offset = stride - ((address - (ctx[1] + addend)) % stride);
        if (offset == stride) offset = 0;
        address += offset;

        // Set up the table entry with our string value.
        M2InteropTable *table = reinterpret_cast<M2InteropTable *>(address);
        table->a = result;
        table->b = result;

        // Compute the actual return value - i.e. the table index.
        std::intptr_t distance = static_cast<std::intptr_t>(address) - (ctx[1] + addend);
        unsigned int index = static_cast<std::uintptr_t>(distance) / stride;
        if (static_cast<std::uintptr_t>(distance) % stride != 0) {
            // Should never happen.
            spdlog::error("[Config] MWinResCfg::GetValueEx produced an invalid index.");
        }
        else {
            if (keys.count(key) == 0) {
                spdlog::info("[Config] MWinResCfg::GetValueEx(\"{}\") -> {}", key, index);
                keys.insert(key);
            }
        }

        return index;
    }

    if (keys.count(key) == 0) {
        spdlog::info("[Config] MWinResCfg::GetValueEx(\"{}\")", key);
        keys.insert(key);
    }

#ifdef _WIN64
    return M2Hook::GetInstance().Invoke<int>(GetCfgValueEx, ctx, id);
#else
    return M2Hook::GetInstance().Invoke<int>(GetCfgValueEx, ctx, _EDX, id);
#endif
}

void Config::Load()
{
    switch (M2Fix::Game())
    {
        case M2FixGame::MGS1:
        case M2FixGame::Contra:
        case M2FixGame::Dracula:
        case M2FixGame::DraculaAdvance:
        case M2FixGame::Darius:
        case M2FixGame::Darius101:
        {
            bool ret = false;

            ret = M2Hook::GetInstance().Hook(
                "50 C6 01 00 E8 ?? ?? ?? FF 8B CF E8 ?? ?? ?? ?? 85 "
                "C0 78 1E 8B 57 04 8D 34 C0 8B 45 20 8D 0C 40",
                -0x48, GetCfgValue, "[Config-32A] MWinResCfg::GetValue"
            );
            if (!ret) {
                ret = M2Hook::GetInstance().Hook(
                    "51 8B 51 04 8B 41 08 2B C2 89 14 24 C1 F8 02 55",
                    0, GetCfgValueEx, "[Config-32] MWinResCfg::GetValueEx"
                );
            }

            break;
        }

        case M2FixGame::NightStrikers:
        {
            M2Hook::GetInstance().Hook(
                "8B 57 04 8D 34 C0 8B 45 20 8D 0C 40 8B 44 B2 18",
                -0x47, GetCfgValue, "[Config-32B] MWinResCfg::GetValue"
            );

            break;
        }

        case M2FixGame::MGSR:
        case M2FixGame::DraculaDominus:
        case M2FixGame::Ray:
        case M2FixGame::Gradius:
        {
            M2Hook::GetInstance().Hook(
                "48 33 C4 48 89 44 24 50 48 8B FA 48 8B D9 48 89 "
                "54 24 48 48 8D 4C 24 28 E8 ?? ?? ?? ?? 48 8B D0 "
                "48 8B CB E8 ?? ?? ?? ?? 85 C0 78 25 48 98 48 6B",
                -0x11, GetCfgValue, "[Config-64] MWinResCfg::GetValue"
            );

            break;
        }

        default: break;
    }
}
