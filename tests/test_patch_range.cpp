#include "../src/m2fix/patch_range.h"
#include <stdexcept>
#include <iostream>
int main() {
    PatchRange range{981, 1, 100, 200};
    if (!range.matches(981, 1, 100) || !range.matches(981, 1, 199) ||
        range.matches(99, 1, 150) || range.matches(981, 0, 150) ||
        range.matches(981, 1, 200))
        throw std::runtime_error("Title/disc/range boundary mismatch");
    std::cout << "Patch range tests passed\n";
}
