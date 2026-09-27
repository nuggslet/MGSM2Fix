#pragma once
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "vendor/miniz/miniz.h"
#include <algorithm>
#include <array>
#include <bcrypt.h>
#include <compressapi.h>
#include <filesystem>
#include <fstream>
#include <functional>
#include <map>
#include <memory>
#include <nlohmann/json.hpp>
#include <regex>
#include <set>
#include <sstream>
#include <vector>
namespace fs = std::filesystem;
using Bytes = std::vector<unsigned char>;
using Json = nlohmann::json;
inline void need(bool ok, const std::string &message) {
    if (!ok)
        throw std::runtime_error(message);
}
inline std::wstring wide(const std::string &s, UINT cp = CP_UTF8) {
    if (s.empty())
        return {};
    int n = MultiByteToWideChar(cp, 0, s.data(), (int)s.size(), nullptr, 0);
    std::wstring r(n, 0);
    MultiByteToWideChar(cp, 0, s.data(), (int)s.size(), r.data(), n);
    return r;
}
inline std::string utf8(const std::wstring &s, UINT cp = CP_UTF8) {
    if (s.empty())
        return {};
    int n = WideCharToMultiByte(cp, 0, s.data(), (int)s.size(), nullptr, 0, nullptr, nullptr);
    std::string r(n, 0);
    WideCharToMultiByte(cp, 0, s.data(), (int)s.size(), r.data(), n, nullptr, nullptr);
    return r;
}
inline std::string str(const fs::path &p) { return utf8(p.wstring()); }
inline std::string lower(std::string s) {
    for (auto &c : s)
        c = (char)tolower((unsigned char)c);
    return s;
}
inline std::string trim(std::string s) {
    auto a = s.find_first_not_of(" \t\r\n");
    if (a == s.npos)
        return {};
    return s.substr(a, s.find_last_not_of(" \t\r\n") - a + 1);
}
inline bool ends(const std::string &s, const std::string &t) {
    return s.size() >= t.size() && s.compare(s.size() - t.size(), t.size(), t) == 0;
}
inline Bytes readFile(const fs::path &p) {
    std::ifstream f(p, std::ios::binary);
    need(bool(f), "Cannot open " + str(p));
    auto n = fs::file_size(p);
    need(n < 256ull * 1024 * 1024, "File too large: " + str(p));
    Bytes b((size_t)n);
    need(bool(f.read((char *)b.data(), b.size())), "Cannot read " + str(p));
    return b;
}
inline std::string textFile(const fs::path &p) {
    auto b = readFile(p);
    return {b.begin(), b.end()};
}
inline void writeFile(const fs::path &p, const Bytes &b) {
    fs::create_directories(p.parent_path());
    std::ofstream f(p, std::ios::binary);
    need(bool(f.write((char *)b.data(), b.size())), "Cannot write " + str(p));
    f.close();
    need(bool(f), "Cannot finish " + str(p));
}
inline void writeText(const fs::path &p, const std::string &s) {
    writeFile(p, Bytes(s.begin(), s.end()));
}
inline Bytes resource(int id) {
    auto r = FindResourceW(nullptr, MAKEINTRESOURCEW(id), RT_RCDATA);
    need(r != nullptr, "Missing embedded resource");
    auto n = SizeofResource(nullptr, r);
    auto p = (unsigned char *)LockResource(LoadResource(nullptr, r));
    return Bytes(p, p + n);
}
inline uint64_t integer(const unsigned char *b, size_t n) {
    need(n <= 8, "Integer width");
    uint64_t v = 0;
    for (size_t i = 0; i < n; ++i)
        v |= uint64_t(b[i]) << (8 * i);
    return v;
}
inline uint64_t at(const Bytes &b, size_t p, size_t n) {
    need(p <= b.size() && n <= b.size() - p, "Truncated data");
    return integer(b.data() + p, n);
}
inline void put(Bytes &b, size_t p, uint64_t v, size_t n) {
    need(p + n <= b.size(), "Write bounds");
    for (size_t i = 0; i < n; ++i)
        b[p + i] = (unsigned char)(v >> (8 * i));
}
struct Hash {
    BCRYPT_ALG_HANDLE alg = nullptr;
    BCRYPT_HASH_HANDLE h = nullptr;
    Bytes object;
    DWORD size = 0;
    Hash(LPCWSTR name = BCRYPT_SHA256_ALGORITHM) {
        need(BCryptOpenAlgorithmProvider(&alg, name, nullptr, 0) >= 0, "Hash provider");
        DWORD n = 0, len = 0;
        BCryptGetProperty(alg, BCRYPT_OBJECT_LENGTH, (PUCHAR)&len, 4, &n, 0);
        BCryptGetProperty(alg, BCRYPT_HASH_LENGTH, (PUCHAR)&size, 4, &n, 0);
        object.resize(len);
        need(BCryptCreateHash(alg, &h, object.data(), len, nullptr, 0, 0) >= 0, "Hash creation");
    }
    ~Hash() {
        if (h)
            BCryptDestroyHash(h);
        if (alg)
            BCryptCloseAlgorithmProvider(alg, 0);
    }
    void add(const void *p, size_t n) {
        while (n) {
            ULONG k = (ULONG)std::min<size_t>(n, 1 << 22);
            need(BCryptHashData(h, (PUCHAR)p, k, 0) >= 0, "Hash update");
            p = (const unsigned char *)p + k;
            n -= k;
        }
    }
    Bytes finish() {
        Bytes r(size);
        need(BCryptFinishHash(h, r.data(), size, 0) >= 0, "Hash finish");
        return r;
    }
};
inline std::string hex(const Bytes &b) {
    const char *h = "0123456789abcdef";
    std::string r;
    for (auto c : b) {
        r += h[c >> 4];
        r += h[c & 15];
    }
    return r;
}
inline std::string digest(const Bytes &b) {
    Hash h;
    h.add(b.data(), b.size());
    return hex(h.finish());
}
struct Mapping {
    HANDLE file = INVALID_HANDLE_VALUE, mapping = nullptr;
    unsigned char *data = nullptr;
    size_t size = 0;
    Mapping(const fs::path &p, bool writable = false) {
        file = CreateFileW(p.c_str(), writable ? GENERIC_READ | GENERIC_WRITE : GENERIC_READ,
                           FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
        need(file != INVALID_HANDLE_VALUE, "Cannot open " + str(p));
        LARGE_INTEGER n{};
        GetFileSizeEx(file, &n);
        size = (size_t)n.QuadPart;
        mapping = CreateFileMappingW(file, nullptr, writable ? PAGE_READWRITE : PAGE_READONLY, 0, 0,
                                     nullptr);
        if (mapping)
            data = (unsigned char *)MapViewOfFile(
                mapping, writable ? FILE_MAP_WRITE : FILE_MAP_READ, 0, 0, 0);
        if (!data) {
            if (mapping)
                CloseHandle(mapping);
            CloseHandle(file);
            throw std::runtime_error("Cannot map " + str(p));
        }
    }
    ~Mapping() {
        if (data)
            UnmapViewOfFile(data);
        if (mapping)
            CloseHandle(mapping);
        if (file != INVALID_HANDLE_VALUE)
            CloseHandle(file);
    }
    Mapping(const Mapping &) = delete;
    Bytes slice(uint64_t p, size_t n) const {
        need(p <= size && n <= size - p, "Truncated image");
        return {data + p, data + p + n};
    }
    std::string sha() const {
        Hash h;
        h.add(data, size);
        return hex(h.finish());
    }
    void flush() { need(FlushViewOfFile(data, 0) && FlushFileBuffers(file), "Cannot flush image"); }
};
struct Zip {
    mz_zip_archive z{};
    FILE *file = nullptr;
    bool writer = false;
    Zip(const Bytes &b) {
        need(mz_zip_reader_init_mem(&z, b.data(), b.size(), 0), "Invalid recipe ZIP");
    }
    Zip(const fs::path &p, bool out = false) : writer(out) {
        file = _wfopen(p.c_str(), out ? L"wb+" : L"rb");
        need(file != nullptr, "Cannot open ZIP: " + str(p));
        bool ok =
            out ? mz_zip_writer_init_cfile(&z, file, 0) : mz_zip_reader_init_cfile(&z, file, 0, 0);
        if (!ok) {
            fclose(file);
            file = nullptr;
            throw std::runtime_error("Invalid ZIP");
        }
    }
    ~Zip() {
        if (writer)
            mz_zip_writer_end(&z);
        else
            mz_zip_reader_end(&z);
        if (file)
            fclose(file);
    }
    Bytes read(const std::string &n) {
        size_t size = 0;
        void *p = mz_zip_reader_extract_file_to_heap(&z, n.c_str(), &size, 0);
        need(p != nullptr, "Missing ZIP entry: " + n);
        Bytes b((unsigned char *)p, (unsigned char *)p + size);
        mz_free(p);
        return b;
    }
    void add(const std::string &n, const Bytes &b) {
        need(mz_zip_writer_add_mem(&z, n.c_str(), b.data(), b.size(), MZ_BEST_COMPRESSION),
             "ZIP write failed");
    }
    void finish() { need(mz_zip_writer_finalize_archive(&z), "ZIP finish failed"); }
};
inline fs::path temporary(const fs::path &parent, const wchar_t *prefix) {
    for (int i = 0; i < 100; ++i) {
        auto p = parent / (std::wstring(prefix) + std::to_wstring(GetCurrentProcessId()) + L"-" +
                           std::to_wstring(GetTickCount64()) + L"-" + std::to_wstring(i));
        if (fs::create_directory(p))
            return p;
    }
    throw std::runtime_error("Cannot create temporary directory");
}
struct Temp {
    fs::path path;
    Temp(const fs::path &parent) : path(temporary(parent, L".m2fix-")) {}
    ~Temp() {
        std::error_code e;
        fs::remove_all(path, e);
    }
};
inline fs::path safePath(const fs::path &root, const std::string &name) {
    need(!name.empty() && name.find(':') == name.npos && name.find('\\') == name.npos,
         "Unsafe package path");
    fs::path rel = wide(name);
    need(!rel.is_absolute(), "Absolute package path");
    for (auto &p : rel)
        need(p != L"..", "Parent package path");
    auto base = fs::weakly_canonical(root), target = fs::weakly_canonical(root / rel);
    auto a = base.begin(), b = target.begin();
    for (; a != base.end(); ++a, ++b)
        need(b != target.end() && lower(str(*a)) == lower(str(*b)),
             "Package path escapes destination");
    return root / rel;
}
