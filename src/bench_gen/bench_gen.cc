#include <cstdlib>
#include <cstddef>
#include <iostream>

#include "gen_specs.h"
#include "name_gen.h"
#include "rand.h"

int main(int argc, char** argv) {
    unsigned n = 1500;
    double p = 0.25;
    switch (argc) {
        case 3:
            p = std::atof(argv[2]);
        case 2:
            n = std::atoi(argv[1]);
        case 1:
            break;
    }

    auto random_engine = new_random_engine();
    auto range = HistRange<GeometricRandomGenerator>{ n, random_engine, p };

    unsigned n_last = 1;
    unsigned last = range.at( 0 );
    for (unsigned i = 1; i < n; ++i) {
        unsigned crnt = range.at( i );
        if ( crnt == last ) {
            ++n_last;
            continue;
        }

        std::cout << NameGen::get( last ) << " x" << n_last << std::endl;
        n_last = 1;
        last = crnt;
    }
    std::cout << NameGen::get( last ) << " x" << n_last << std::endl;
}
