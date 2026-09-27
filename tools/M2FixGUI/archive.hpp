#pragma once
#include "core_base.hpp"

// Archive MDF obfuscation uses reference MT19937 init_by_array, not
// std::seed_seq.
inline Bytes unmdf(Bytes b, const std::string &name) {
    need(b.size() >= 8 && memcmp(b.data(), "mdf\0", 4) == 0, "Invalid MDF");
    auto want = at(b, 4, 4);
    need(want < 128 * 1024 * 1024, "Oversized index");
    Hash md5(BCRYPT_MD5_ALGORITHM);
    auto key = "25G/xpvTbsb+6" + lower(name);
    md5.add(key.data(), key.size());
    auto d = md5.finish();
    uint32_t mt[624];
    mt[0] = 19650218;
    for (uint32_t i = 1; i < 624; ++i)
        mt[i] = 1812433253u * (mt[i - 1] ^ (mt[i - 1] >> 30)) + i;
    size_t i = 1, j = 0;
    for (size_t k = 624; k; --k) {
        mt[i] = (mt[i] ^ ((mt[i - 1] ^ (mt[i - 1] >> 30)) * 1664525u)) + (uint32_t)at(d, j * 4, 4) +
                (uint32_t)j;
        if (++i >= 624) {
            mt[0] = mt[623];
            i = 1;
        }
        j = (j + 1) % 4;
    }
    for (size_t k = 623; k; --k) {
        mt[i] = (mt[i] ^ ((mt[i - 1] ^ (mt[i - 1] >> 30)) * 1566083941u)) - (uint32_t)i;
        if (++i >= 624) {
            mt[0] = mt[623];
            i = 1;
        }
    }
    mt[0] = 0x80000000;
    for (i = 0; i < 624; ++i) {
        uint32_t y = (mt[i] & 0x80000000) | (mt[(i + 1) % 624] & 0x7fffffff);
        mt[i] = mt[(i + 397) % 624] ^ (y >> 1) ^ ((y & 1) ? 0x9908b0df : 0);
    }
    Bytes stream(64);
    for (i = 0; i < 16; ++i) {
        uint32_t y = mt[i];
        y ^= y >> 11;
        y ^= (y << 7) & 0x9d2c5680;
        y ^= (y << 15) & 0xefc60000;
        y ^= y >> 18;
        put(stream, 4 * i, y, 4);
    }
    for (i = 8; i < b.size(); ++i)
        b[i] ^= stream[(i - 8) % 64];
    Bytes out((size_t)want);
    mz_ulong n = (mz_ulong)out.size();
    need(mz_uncompress(out.data(), &n, b.data() + 8, (mz_ulong)b.size() - 8) == MZ_OK && n == want,
         "Steam index decompression failed");
    return out;
}
struct Psb {
    Bytes d;
    std::vector<uint64_t> offsets, tree, start, strings;
    size_t entry, strdata;
    uint64_t val(size_t p, size_t n) { return at(d, p, n); }
    std::vector<uint64_t> array(size_t &p) {
        size_t w = (size_t)val(p++, 1) - 12;
        auto count = val(p, w);
        p += w;
        size_t m = (size_t)val(p++, 1) - 12;
        need(m > 0 && m <= 8 && count <= d.size() / m, "Invalid PSB array");
        std::vector<uint64_t> r;
        for (size_t i = 0; i < count; ++i) {
            r.push_back(val(p, m));
            p += m;
        }
        return r;
    }
    Psb(Bytes b) : d(std::move(b)) {
        need(d.size() > 40 && memcmp(d.data(), "PSB\0", 4) == 0, "Invalid PSB");
        size_t p = (size_t)val(12, 4);
        offsets = array(p);
        tree = array(p);
        start = array(p);
        p = (size_t)val(16, 4);
        strings = array(p);
        strdata = (size_t)val(20, 4);
        entry = (size_t)val(36, 4);
    }
    std::string name(size_t n) {
        need(n < start.size() && start[n] < tree.size(), "PSB name");
        auto node = tree[start[n]];
        std::string r;
        for (size_t count = 0; node; ++count) {
            need(count < 1024 && node < tree.size(), "PSB trie");
            auto parent = tree[node];
            need(parent < offsets.size(), "PSB parent");
            r += (char)(node - offsets[parent]);
            node = parent;
        }
        std::reverse(r.begin(), r.end());
        return r;
    }
    Json value(size_t p, int depth = 0) {
        need(depth < 64, "PSB nesting");
        auto t = val(p++, 1);
        if (t <= 1)
            return nullptr;
        if (t == 2)
            return false;
        if (t == 3)
            return true;
        if (t >= 4 && t <= 12)
            return val(p, (size_t)t - 4);
        if (t >= 13 && t <= 20) {
            --p;
            return array(p);
        }
        if (t >= 21 && t <= 24) {
            auto n = val(p, (size_t)t - 20);
            need(n < strings.size(), "PSB string");
            auto s = strdata + strings[n];
            std::string r;
            while (val(s, 1))
                r += (char)val(s++, 1);
            return r;
        }
        if (t >= 25 && t <= 29)
            return nullptr;
        if (t == 30 || t == 31)
            return 0;
        if (t == 32) {
            auto list = array(p);
            Json r = Json::array();
            for (auto o : list)
                r.push_back(value(p + o, depth + 1));
            return r;
        }
        if (t == 33) {
            auto names = array(p), list = array(p);
            need(names.size() == list.size(), "PSB object");
            Json r = Json::object();
            for (size_t i = 0; i < names.size(); ++i)
                r[name(names[i])] = value(p + list[i], depth + 1);
            return r;
        }
        throw std::runtime_error("Unsupported PSB type");
    }
};
inline Json archiveIndex(const fs::path &blob) {
    auto p = blob;
    p.replace_extension(L".psb.m");
    Psb psb(unmdf(readFile(p), str(p.filename())));
    auto files = psb.value(psb.entry).at("file_info");
    Json result = Json::object();
    for (auto &[name, extent] : files.items())
        result[lower(name)] = extent;
    return result;
}
inline Bytes lz4(const Bytes &b, size_t p, size_t n, size_t size) {
    need(p + n <= b.size(), "Truncated LZ4");
    size_t end = p + n;
    Bytes out;
    out.reserve(size);
    while (p < end) {
        unsigned token = b[p++];
        size_t literal = token >> 4;
        auto extension = [&](size_t &length) {
            unsigned c;
            do {
                need(p < end, "Truncated LZ4 length");
                c = b[p++];
                length += c;
            } while (c == 255);
        };
        if (literal == 15)
            extension(literal);
        need(literal <= end - p && literal <= size - out.size(), "LZ4 literal bounds");
        out.insert(out.end(), b.begin() + p, b.begin() + p + literal);
        p += literal;
        if (p == end)
            break;
        need(p + 2 <= end, "LZ4 offset");
        size_t offset = (size_t)at(b, p, 2);
        p += 2;
        need(offset && offset <= out.size(), "LZ4 reference");
        size_t length = token & 15;
        if (length == 15)
            extension(length);
        length += 4;
        need(length <= size - out.size(), "LZ4 output bounds");
        for (size_t i = 0; i < length; ++i)
            out.push_back(out[out.size() - offset]);
    }
    need(out.size() == size, "LZ4 size mismatch");
    return out;
}
