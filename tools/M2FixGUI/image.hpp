#pragma once
#include "core_base.hpp"

inline Bytes logical(const Mapping &disc, uint64_t base, uint64_t lba, size_t size) {
    Bytes r;
    need(size < 16 * 1024 * 1024, "ISO extent too large");
    for (size_t i = 0; i < size; i += 2048) {
        auto b = disc.slice(base + (lba + i / 2048) * 2352 + 24, std::min<size_t>(2048, size - i));
        r.insert(r.end(), b.begin(), b.end());
    }
    return r;
}
inline std::pair<uint64_t, size_t> exeExtent(const Mapping &disc, uint64_t base,
                                             const std::string &wanted) {
    auto pvd = logical(disc, base, 16, 2048);
    need(memcmp(pvd.data(), "\1CD001", 6) == 0, "Not a raw Integral image");
    std::function<std::pair<uint64_t, size_t>(uint64_t, size_t, std::string, int)> walk;
    walk = [&](uint64_t lba, size_t size, std::string dir,
               int depth) -> std::pair<uint64_t, size_t> {
        need(depth < 8, "ISO directory depth");
        auto b = logical(disc, base, lba, size);
        for (size_t i = 0; i < b.size();) {
            size_t len = b[i];
            if (!len) {
                i = (i / 2048 + 1) * 2048;
                continue;
            }
            need(len >= 34 && i + len <= b.size(), "ISO directory record");
            size_t count = b[i + 32];
            need(count <= len - 33, "ISO filename");
            std::string name((char *)b.data() + i + 33, count);
            auto block = at(b, i + 2, 4);
            size_t bytes = (size_t)at(b, i + 10, 4);
            if (!(count == 1 && (name[0] == 0 || name[0] == 1))) {
                std::string full = dir + "/" + name;
                if (lower(full) == lower(wanted))
                    return {block, bytes};
                if (b[i + 25] & 2) {
                    auto r = walk(block, bytes, full, depth + 1);
                    if (r.second)
                        return r;
                }
            }
            i += len;
        }
        return {0, 0};
    };
    auto r = walk(at(pvd, 158, 4), (size_t)at(pvd, 166, 4), "", 0);
    need(r.second != 0, "Integral executable not found");
    return r;
}
struct DiscSpec {
    std::string id, exe, folder;
    uint64_t base;
};
inline const std::vector<DiscSpec> specs = {
    {"1", "/MGS/SLPM_862.47;1", "mods/INTEGRAL/INTEGRAL/0", 0},
    {"2", "/MGS/SLPM_862.48;1", "mods/INTEGRAL/INTEGRAL/1", 0x2ae54800},
    {"vr", "/MGS/SLPM_862.49;1", "mods/INTEGRAL/VR-DISK", 0x57592000}};
inline void parity(Bytes &s) {
    need(s.size() == 2352 && s[15] == 2 && !(s[18] & 32), "Patched sector is not Mode 2 Form 1");
    static std::array<unsigned char, 256> f{}, b{};
    static std::array<uint32_t, 256> edc{};
    static bool init = false;
    if (!init) {
        for (unsigned i = 0; i < 256; ++i) {
            unsigned j = (i << 1) ^ ((i & 128) ? 0x11d : 0);
            f[i] = (unsigned char)j;
            b[i ^ (j & 255)] = (unsigned char)i;
            uint32_t c = i;
            for (int k = 0; k < 8; ++k)
                c = (c >> 1) ^ ((c & 1) ? 0xd8018001 : 0);
            edc[i] = c;
        }
        init = true;
    }
    uint32_t crc = 0;
    for (int i = 16; i < 2072; ++i)
        crc = (crc >> 8) ^ edc[(crc ^ s[i]) & 255];
    put(s, 2072, crc, 4);
    auto header = at(s, 12, 4);
    put(s, 12, 0, 4);
    auto ecc = [&](int major, int minor, int mult, int inc, int dest) {
        for (int m = 0; m < major; ++m) {
            int index = (m >> 1) * mult + (m & 1);
            unsigned char a = 0, c = 0;
            for (int n = 0; n < minor; ++n) {
                auto v = s[12 + index];
                index = (index + inc) % (major * minor);
                a ^= v;
                c ^= v;
                a = f[a];
            }
            a = b[f[a] ^ c];
            s[dest + m] = a;
            s[dest + m + major] = a ^ c;
        }
    };
    ecc(86, 24, 2, 86, 2076);
    ecc(52, 43, 86, 88, 2248);
    put(s, 12, header, 4);
}
inline std::map<uint64_t, Bytes> language(const Mapping &image, const DiscSpec &spec) {
    auto [lba, size] = exeExtent(image, 0, spec.exe);
    auto exe = logical(image, 0, lba, size);
    std::vector<size_t> hits;
    for (size_t p = 2048; p + 72 < exe.size(); p += 4) {
        auto w = [&](int i) { return (uint32_t)at(exe, p + i * 4, 4); };
        if (w(0) == 0x27bdffe8 && w(1) == 0xafbf0010 && (w(2) >> 26) == 3 && !w(3) &&
            (w(4) >> 26) == 3 && !w(5) && (w(6) >> 26) == 3 && !w(7) && w(8) == 0x24040067 &&
            w(9) == 0x3c058002 && (w(10) >> 26) == 3 && (w(11) >> 26) == 9 && (w(12) >> 26) == 3 &&
            w(13) == 0x2021 && w(14) == 0x8fbf0010 && !w(15) && w(16) == 0x03e00008 &&
            w(17) == 0x27bd0018)
            hits.push_back(p);
    }
    need(hits.size() == 1, "English startup function mismatch");
    size_t p = hits[0];
    auto w = [&](int i) { return (uint32_t)at(exe, p + i * 4, 4); };
    auto target = [&](uint32_t word) { return 0x80000000u | ((word & 0x03ffffff) << 2); };
    size_t init = 2048 + target(w(4)) - 0x80010000;
    uint32_t config = 0;
    bool c = false, g = false;
    for (int i = 0; i < 16; ++i) {
        auto v = (uint32_t)at(exe, init + 4 * i, 4);
        if ((v >> 16) == 0x3c10 && (at(exe, init + 4 * i + 4, 4) >> 16) == 0x2610)
            config = ((v & 65535) << 16) + (int16_t)at(exe, init + 4 * i + 4, 2) + 4;
        c |= v == 0x86120004;
        g |= v == 0x86110002;
    }
    need(c && g && config && (config & 65535) < 0x8000, "English configuration location mismatch");
    uint32_t words[] = {w(0),
                        w(1),
                        w(2),
                        0,
                        0x24030100,
                        0x3c020000 | (config >> 16),
                        w(4),
                        0xa4430000 | (config & 65535),
                        w(6),
                        0,
                        w(8),
                        w(9),
                        w(10),
                        w(11),
                        w(14),
                        w(17),
                        0x08000000 | (w(12) & 0x03ffffff),
                        0x2021};
    std::map<uint64_t, Bytes> r;
    for (int i = 0; i < 18; ++i) {
        size_t off = p + 4 * i;
        Bytes v(4);
        put(v, 0, words[i], 4);
        r[(lba + off / 2048) * 2352 + 24 + off % 2048] = v;
    }
    return r;
}
inline void records(const Bytes &b, const std::function<void(uint64_t, size_t, size_t)> &fn) {
    need(b.size() >= 60 && memcmp(b.data(), "PPF30\2", 6) == 0 && !b[58], "Invalid PPF recipe");
    size_t p = 60 + (b[57] ? 1024 : 0);
    while (p < b.size()) {
        auto off = at(b, p, 8);
        size_t n = (size_t)at(b, p + 8, 1);
        p += 9;
        need(n && p + n <= b.size(), "Truncated PPF record");
        fn(off, p, n);
        p += n;
    }
}
