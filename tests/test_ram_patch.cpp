#include "../src/m2fix/ram_patch.h"
#include <stdexcept>
#include <iostream>
int main() {
    std::vector<Ketchup_RamPatch> patches{{100, {1, 2, 3}}, {101, {4}}};
    NormalizeRamPatches(patches);
    std::map<unsigned, unsigned char> ram{{100, 1}, {101, 0}, {102, 3}};
    auto read = [&](unsigned at) { return ram.at(at); };
    auto write = [&](unsigned at, unsigned char v) { ram.at(at) = v; };
    if (RepairRamPatches(patches, read, write) != 1 || ram.at(101) != 4)
        throw std::runtime_error("Interior corruption or last-writer precedence failed");
    if (RepairRamPatches(patches, read, write) != 0)
        throw std::runtime_error("Intact RAM must not be rewritten");
    std::cout << "RAM patch tests passed\n";
}
