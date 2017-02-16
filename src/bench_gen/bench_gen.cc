#include <cstdlib>
#include <cstddef>
#include <iostream>

#include "gen_specs.h"
#include "name_gen.h"
#include "rand.h"

#include "data/option.h"

int main(int argc, char** argv) {
    GenSpecs specs{ Args{ argc, argv } };
    
    unsigned over_ind = 0;
    unsigned last = 0;
    for (unsigned i = 0; i < specs.decl_names->size(); ++i) {
        unsigned crnt = specs.decl_names->at( i );
        if ( crnt != last ) {
            over_ind = 0;
        }

        unsigned n_rets = (*specs.n_rets)();
        unsigned n_parms = (*specs.n_parms)();

        // print parameters
        for (unsigned ret = 0; ret < n_rets; ++ret) {
            std::cout << "1 ";
        }

        // print name+tag
        std::cout << NameGen::get( last ) << "-o" << over_ind;

        // print returns
        for (unsigned parm = 0; parm < n_parms; ++parm) {
            std::cout << " 1";
        }

        std::cout << std::endl;
        
        ++over_ind;
        last = crnt;
    }
}
