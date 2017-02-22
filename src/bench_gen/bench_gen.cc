#include <algorithm>
#include <cstdlib>
#include <cstddef>
#include <iostream>

#include "gen_args.h"
#include "name_gen.h"
#include "random_partitioner.h"

#include "ast/type.h"
#include "data/list.h"
#include "data/option.h"

template<typename T>
void comment_print( const char* name, const T& x ) {
    std::cout << "// " << arg_print( name, x ) << std::endl;
}

int main(int argc, char** argv) {
    using std::min;
    using std::max;
    using std::shuffle;

/*    Args a{ argc, argv };

    comment_print( "n_decls", a.n_decls() );
    if ( a.seed() ) comment_print( "seed", *a.seed() );
    comment_print( "n_overloads", a.n_overloads() );
    comment_print( "n_rets", a.n_rets() );
    comment_print( "n_parms", a.n_parms() );
    comment_print( "n_poly_types", a.n_poly_types() );

    unsigned n_decls = 0;
    unsigned i_name = 0;
    while ( n_decls < a.n_decls() ) {
        // generate overloads with a given name
        unsigned n_with_name = a.n_overloads()();
        std::string name = NameGen::get( i_name++ );
        for (unsigned i_with_name = 0; i_with_name < n_with_name; ++i_with_name ) {
            unsigned n_poly_types = a.n_poly_types()();
            unsigned n_rets = a.n_rets()();
            unsigned n_parms = a.n_parms()();
            // ensure that there are enough parameters/returns to handle all poly types
            if ( n_poly_types < n_parms + n_rets ) { n_parms = n_poly_types - n_rets; }

            // generate list of parameters and returns
            List<Type> parms( n_parms, nullptr );
            List<Type> rets( n_rets, nullptr );
            // at least one instance of each 
            unsigned poly_type;
            for (poly_type = 0; poly_type < min(n_poly_types, n_parms); ++poly_type) {
                parms[poly_type] = new PolyType{ NameGen::get_cap( poly_type ) };
            }
            for (; poly_type < n_poly_types; ++poly_type) {
                rets[poly_type - n_parms] = new PolyType{ NameGen::get_cap( poly_type ) };
            }

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

    collect();
*/
    for (unsigned n = 0; n <= 10; ++n) {
        std::cerr << "[" << n << "]";
        for (unsigned k = 0; k <= n; ++k) {
            std::cerr << " " << RandomPartitioner::C(n, k);
        }
        std::cerr << std::endl;
    }
}
