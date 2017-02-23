#include <algorithm>
#include <array>
#include <cassert>
#include <cstdlib>
#include <cstddef>
#include <initializer_list>
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

    Args a{ argc, argv };

    comment_print( "n_decls", a.n_decls() );
    if ( a.seed() ) comment_print( "seed", *a.seed() );
    comment_print( "n_overloads", a.n_overloads() );
    comment_print( "n_rets", a.n_rets() );
    comment_print( "n_parms", a.n_parms() );
    comment_print( "n_poly_types", a.n_poly_types() );

    RandomPartitioner partition( a.engine() );

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

            // generate list of parameters and returns
            List<Type> parms( n_parms );
            List<Type> rets( n_rets );

            unsigned last_poly = 0;
            unsigned last_basic = 0;
            unsigned last_struct = 0;

            if ( n_poly_types > 0 ) {
                partition.get( n_parms, { &last_poly, &last_basic, &last_struct } );
            } else {
                partition.get( n_parms, { &last_basic, &last_struct } );
            }

            unsigned parm;
            for ( parm = 0; parm < last_poly; ++parm ) {
                parms[parm] = new PolyType{ NameGen::get_cap( 0 ) };
            }
            for ( ; parm < last_basic; ++parm ) {
                parms[parm] = new ConcType{ 0 };
            }
            for ( ; parm < last_struct; ++parm ) {
                parms[parm] = new NamedType{ NameGen::get( 0 ) };
            }

            if ( n_poly_types > 0 ) {
                partition.get( n_rets, { &last_poly, &last_basic, &last_struct } );
            } else {
                partition.get( n_rets, { &last_basic, &last_struct } );
            }

            unsigned ret;
            for ( ret = 0; ret < last_poly; ++ret ) {
                rets[ret] = new PolyType{ NameGen::get_cap( 0 ) };
            }
            for ( ; ret < last_basic; ++ret ) {
                rets[ret] = new ConcType{ 0 };
            }
            for ( ; ret < last_struct; ++ret ) {
                rets[ret] = new NamedType{ NameGen::get( 0 ) };
            }

            // print returns
            for (unsigned ret = 0; ret < n_rets; ++ret) {
                std::cout << *rets[ret] << " ";
            }

            // print name+tag
            std::cout << name;
            if ( n_with_name > 1 ) { std::cout << "-o" << i_with_name; }

            // print parameters
            for (unsigned parm = 0; parm < n_parms; ++parm) {
                std::cout << " " << *parms[parm];
            }

            std::cout << std::endl;

            // break if out of declarations
            if ( ++n_decls == a.n_decls() ) break;
        }
    }

    collect();
}
