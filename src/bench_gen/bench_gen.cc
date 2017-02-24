#include <algorithm>
#include <array>
#include <cassert>
#include <cstdlib>
#include <cstddef>
#include <initializer_list>
#include <iostream>
#include <set>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include "gen_args.h"
#include "name_gen.h"
#include "random_partitioner.h"
#include "weighted_partition.h"

#include "ast/type.h"
#include "data/option.h"

/// Generates a type
struct TypeGen {
    enum { None, Conc, Named, Poly } kind;
    unsigned i;

    TypeGen() : kind(None), i(0) {}
    TypeGen( unsigned i ) : kind(None), i(i) {}

    /// Generates a ConcType with the given index
    static TypeGen conc(unsigned i) {
        TypeGen t(i);
        t.kind = Conc;
        return t;
    }

    /// Generates a NamedType with the given index
    static TypeGen named(unsigned i) {
        TypeGen t(i);
        t.kind = Named;
        return t;
    }

    /// Generates a PolyType with the given index
    static TypeGen poly(unsigned i) {
        TypeGen t(i);
        t.kind = Poly;
        return t;
    }

    /// Generates a new type of the underlying kind
    Type* get() const {
        switch (kind) {
        case None: return nullptr;
        case Conc: return new ConcType{ (int)i };
        case Named: return new NamedType{ NameGen::get( i ) };
        case Poly: return new PolyType{ NameGen::get_cap( i ) };
        }
    }

    bool operator== ( const TypeGen& o ) const {
        return kind == o.kind && i == o.i;
    }
    bool operator!= ( const TypeGen& o ) const { return !(*this == o); }
    
    bool operator< ( const TypeGen& o ) const {
        return kind < o.kind || (kind == o.kind && i < o.i);
    }
    bool operator> ( const TypeGen& o ) const { return o < *this; }
    bool operator<= ( const TypeGen& o ) const { return !(o < *this); }
    bool operator>= ( const TypeGen& o ) const { return !(*this < o); }
};

/// List of type generators
using GenList = std::vector<TypeGen>;

/// Normalizes a vector of TypeGen such that all poly-types are in increasing order
void normalize_poly( GenList& v ) {
    std::unordered_map<unsigned, unsigned> polys;
    for ( TypeGen& t : v ) {
        if ( t.kind != TypeGen::Poly ) continue;

        auto it = polys.find( t.i );  // lookup replacement index
        if ( it == polys.end() ) {    // assign consecutively if none found
            it = polys.emplace_hint(it, t.i, polys.size() );
        }
        t.i = it->second;              // replace index
    }
}

template<typename T>
void comment_print( const char* name, const T& x ) {
    std::cout << "// " << arg_print( name, x ) << std::endl;
}

template<>
void comment_print<Generator>( const char* name, const Generator& x ) {
    std::cout << "// " << arg_print( name, x ) 
        << " [" << x.min() << "," << x.max() << "]" << std::endl;
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
        // generate number of overloads with a given name
        unsigned n_with_name = min( a.n_overloads()(), a.n_decls() - n_decls );
        std::string name = NameGen::get( i_name++ );

        // decide on number of paramters, return types & type variables according 
        // to a plausible distribution
        unsigned i_with_name = 0;
        for ( auto parm_overloads 
                : weighted_partition( a.n_parms(), n_with_name ) ) {
            unsigned n_parms = parm_overloads.first;
            
            for ( auto ret_overloads 
                    : weighted_partition( a.n_rets(), parm_overloads.second ) ) {
                unsigned n_rets = ret_overloads.first;

                std::set<GenList> with_same_arity;  // used to ensure no duplicate functions

                for ( auto poly_overloads
                        : weighted_partition( a.n_poly_types(), ret_overloads.second ) ) {
                    unsigned n_poly = poly_overloads.first;
                    unsigned n_same_kind = poly_overloads.second;

                    GenList parms_and_rets;
                    parms_and_rets.reserve( n_parms + n_rets );

                    for (unsigned i_same_kind = 0; i_same_kind < n_same_kind; ++i_same_kind) {
                        do {
                            parms_and_rets.clear();

                            // partition the parameter and return lists between types
                            unsigned last_poly = 0;
                            unsigned last_basic = 0;
                            unsigned last_struct = 0;
                            if ( n_poly > 0 ) {
                                partition.get( n_parms + n_rets, { &last_poly, &last_basic, &last_struct } );
                            } else {
                                partition.get( n_parms + n_rets, { &last_basic, &last_struct } );
                            }

                            // generate parameter and return lists to fit the partitioning
                            unsigned i = 0;

                            // polymorphic parameters/returns
                            unsigned i_poly = 0;  // number of polymorphic types
                            if ( last_poly > 0 ) {
                                parms_and_rets.push_back( TypeGen::poly( i_poly++ ) );
                                for (++i; i < last_poly; ++i) {
                                    if ( i_poly < n_poly && a.is_new_type()() ) {
                                        // select new poly type
                                        parms_and_rets.push_back( TypeGen::poly( i_poly++ ) );
                                    } else {
                                        // repeat existing poly type
                                        parms_and_rets.push_back( TypeGen::poly( random( a.engine(), i_poly-1 ) ) );
                                    }
                                }
                            }

                            // basic parameters/returns
                            std::vector<unsigned> basics;  // exisitng set of basic types
                            for (; i < last_basic; ++i) {
                                if ( basics.empty() || a.is_new_type()() ) {
                                    // select new basic type
                                    unsigned new_basic;
                                    do {
                                        new_basic = a.get_basic()();
                                    } while ( std::find( basics.begin(), basics.end(), new_basic ) != basics.end() );
                                    basics.push_back( new_basic );
                                    parms_and_rets.push_back( TypeGen::conc( new_basic ) );
                                } else {
                                    // repeat existing basic type
                                    parms_and_rets.push_back( TypeGen::conc( basics[ random( a.engine(), basics.size()-1 ) ] ) );
                                }
                            }

                            // struct parameters/returns
                            std::vector<unsigned> structs;  // exisitng set of struct types
                            for (; i < last_struct; ++i) {
                                if ( structs.empty() || a.is_new_type()() ) {
                                    // select new struct type
                                    unsigned new_struct;
                                    do {
                                        new_struct = a.get_struct()();
                                    } while ( std::find( structs.begin(), structs.end(), new_struct ) != structs.end() );
                                    structs.push_back( new_struct );
                                    parms_and_rets.push_back( TypeGen::named( new_struct ) );
                                } else {
                                    // repeat existing struct type
                                    parms_and_rets.push_back( TypeGen::named( structs[ random( a.engine(), structs.size()-1 ) ] ) );
                                }
                            }

                            // randomize list order
                            std::shuffle( parms_and_rets.begin(), parms_and_rets.end(), a.engine() );
                            normalize_poly( parms_and_rets );

                        } while ( ! with_same_arity.insert( parms_and_rets ).second ); // retry if a duplicate

                        // generate list of parameters and returns
                        List<Type> parms( n_parms );
                        List<Type> rets( n_rets );
                        for (unsigned i = 0; i < n_parms; ++i) {
                            parms[i] = parms_and_rets[i].get();
                        }
                        for (unsigned i = 0; i < n_rets; ++i) {
                            rets[i] = parms_and_rets[n_parms + i].get();
                        }

                        // print decl
                        for (unsigned ret = 0; ret < n_rets; ++ret) {
                            std::cout << *rets[ret] << " ";
                        }

                        std::cout << name;
                        if ( n_with_name > 1 ) {
                            std::cout << "-o" << i_with_name;
                        }

                        // print parameters
                        for (unsigned parm = 0; parm < n_parms; ++parm) {
                            std::cout << " " << *parms[parm];
                        }

                        std::cout << std::endl;

                        ++n_decls;
                        ++i_with_name;
                    }
                }
            }
        }
    }

    collect();
}
