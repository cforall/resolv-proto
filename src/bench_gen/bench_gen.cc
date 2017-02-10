#include <cstdlib>
#include <cstddef>
#include <iostream>

#include "gen_specs.h"
#include "name_gen.h"
#include "rand.h"

#include "data/option.h"

int main(int argc, char** argv) {
    unsigned n = 1500;
    unsigned m = 750;
    unsigned high = 120;
    option<unsigned> seed;
    switch (argc) {
        case 5:
            seed = std::atoi(argv[4]);
        case 4:
            high = std::atoi(argv[3]);
        case 3:
            m = std::atoi(argv[2]);
        case 2:
            n = std::atoi(argv[1]);
        case 1:
            break;
    }

    auto random_engine = seed ? new_random_engine( *seed ) : new_random_engine();
    auto range = LongTailRange<PartitionRange<UniformRandomGenerator>>{ n, m, random_engine, 2, high };

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
