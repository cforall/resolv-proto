#include <cstdlib>
#include <cstddef>
#include <iostream>

#include "gen_specs.h"
#include "name_gen.h"
#include "rand.h"

#include "data/option.h"

int main(int argc, char** argv) {
    GenSpecs specs{ Args{ argc, argv } };

    unsigned n_decls = 0;
    unsigned i_name = 0;
    while ( n_decls < specs.n_decls ) {
        // generate overloads with a given name
        unsigned n_with_name = (*specs.n_overloads)();
        std::string name = NameGen::get( i_name++ );
        for (unsigned i_with_name = 0; i_with_name < n_with_name; ++i_with_name ) {
            unsigned n_rets = (*specs.n_rets)();
            unsigned n_parms = (*specs.n_parms)();

            // print parameters
            for (unsigned ret = 0; ret < n_rets; ++ret) {
                std::cout << "1 ";
            }

            // print name+tag
            std::cout << name;
            if ( n_with_name > 1 ) { std::cout << "-o" << i_with_name; }

            // print returns
            for (unsigned parm = 0; parm < n_parms; ++parm) {
                std::cout << " 1";
            }

            std::cout << std::endl;

            // break if out of declarations
            if ( ++n_decls == specs.n_decls ) break;
        }
    }
}
