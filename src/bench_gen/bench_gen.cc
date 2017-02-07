#include <cstddef>
#include <iostream>

#include "name_gen.h"

int main() {
    unsigned n = 2000;
    unsigned nc = 16;
    for (unsigned i = 0; i < n; i += nc) {
        std::cout << "[" << (i/nc) << "] " << i << ": " << NameGen::get( i );
        for (unsigned j = 1; j < nc; ++j) {
            std::cout << " " << (i + j) << ": " << NameGen::get( i + j );
        }
        std::cout << std::endl;
    }
}
