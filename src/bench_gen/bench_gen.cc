#include <cstdlib>
#include <cstddef>
#include <iostream>

#include "gen_args.h"
#include "name_gen.h"

#include "data/option.h"

int main(int argc, char** argv) {
    Args a{ argc, argv };

    std::cout << "// n_decls: " << a.n_decls() << std::endl;
    if ( a.seed() ) std::cout << "// seed: " << *a.seed() << std::endl;

    unsigned n_decls = 0;
    unsigned i_name = 0;
    while ( n_decls < a.n_decls() ) {
        // generate overloads with a given name
        unsigned n_with_name = a.n_overloads()();
        std::string name = NameGen::get( i_name++ );
        for (unsigned i_with_name = 0; i_with_name < n_with_name; ++i_with_name ) {
            unsigned n_rets = a.n_rets()();
            unsigned n_parms = a.n_parms()();

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
            if ( ++n_decls == a.n_decls() ) break;
        }
    }
}
