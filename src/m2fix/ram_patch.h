#pragma once
#include <map>
#include <vector>
#include <cstddef>

struct Ketchup_RamPatch {
    unsigned int address;
    std::vector<unsigned char> data;
};

inline void NormalizeRamPatches(std::vector<Ketchup_RamPatch> &patches) {
    std::map<unsigned int, unsigned char> finalBytes;
    for (const auto &patch : patches)
        for (size_t i = 0; i < patch.data.size(); ++i) finalBytes[patch.address + i] = patch.data[i];
    patches.clear();
    for (const auto &[address, value] : finalBytes) {
        if (patches.empty() || patches.back().address + patches.back().data.size() != address)
            patches.push_back({address, {}});
        patches.back().data.push_back(value);
    }
}

template<class Read, class Write>
size_t RepairRamPatches(const std::vector<Ketchup_RamPatch> &patches, Read read, Write write) {
    size_t changed = 0;
    for (const auto &patch : patches) for (size_t i = 0; i < patch.data.size(); ++i) {
        const auto address = patch.address + i;
        if ((read(address) & 0xFF) == patch.data[i]) continue;
        write(address, patch.data[i]);
        ++changed;
    }
    return changed;
}
