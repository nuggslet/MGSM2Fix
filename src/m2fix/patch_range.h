#pragma once
#include <cstdint>

struct PatchRange {
    unsigned title, disk;
    uint64_t start, end;
    bool matches(unsigned currentTitle, unsigned currentDisk, uint64_t offset) const {
        return title == currentTitle && disk == currentDisk && offset >= start && offset < end;
    }
};
