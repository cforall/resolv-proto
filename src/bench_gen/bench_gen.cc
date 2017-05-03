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

#include "ast/decl.h"
#include "ast/type.h"
#include "data/cast.h"
#include "data/list.h"
#include "data/option.h"
#include "resolver/environment.h"

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
        case Named: return new NamedType{ NameGen::get_lower( i ) };
        case Poly: return new PolyType{ NameGen::get_cap( i ) };
        default: assert(false); return nullptr;
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

class BenchGenerator {
    /// Parsed arguments
    Args a;
    /// Random partitioner
    RandomPartitioner partition;

    /// All functions
    List<FuncDecl> funcs;
    /// Functions with basic return type
    List<FuncDecl> basic_funcs;
    /// Functions with polymorphic return type
    List<FuncDecl> poly_funcs;
    /// Functions with struct type, by struct name
    std::unordered_map<std::string, List<FuncDecl>> struct_funcs;

    unsigned random( unsigned e ) { return ::random( a.engine(), e ); }

    unsigned random( unsigned b, unsigned e ) { return ::random( a.engine(), b, e ); }

    template<typename C>
    auto random_in( const C& c ) -> decltype( c[0] ) { return ::random_in( a.engine(), c ); }

    bool coin_flip( double p = 0.5 ) { return ::coin_flip( a.engine(), p ); }

    /// Print all the arguments as comments
    void print_args() {
        comment_print( "n_decls", a.n_decls() );
        comment_print( "n_exprs", a.n_exprs() );
        if ( a.seed() ) comment_print( "seed", *a.seed() );
        comment_print( "n_overloads", a.n_overloads() );
        comment_print( "n_rets", a.n_rets() );
        comment_print( "n_parms", a.n_parms() );
        comment_print( "n_poly_types", a.n_poly_types() );
        comment_print( "max_tries_for_unique", a.max_tries_for_unique() );
        comment_print( "p_new_type", a.p_new_type() );
        comment_print( "get_basic", a.get_basic() );
        comment_print( "get_struct", a.get_struct() );
        comment_print( "p_nested_at_root", a.p_nested_at_root() );
        comment_print( "p_nested_deeper", a.p_nested_deeper() );
        comment_print( "p_unsafe_offset", a.p_unsafe_offset() );
        comment_print( "basic_offset", a.basic_offset() );
    }

    /// Generate a parameter+return list
    void generate_parms_and_rets( GenList& parms_and_rets, 
            unsigned n_parms, unsigned n_rets, unsigned n_poly ) {
        
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
                if ( i_poly < n_poly && coin_flip( a.p_new_type() ) ) {
                    // select new poly type
                    parms_and_rets.push_back( TypeGen::poly( i_poly++ ) );
                } else {
                    // repeat existing poly type
                    parms_and_rets.push_back( TypeGen::poly( random( i_poly-1 ) ) );
                }
            }
        }

        // basic parameters/returns
        std::vector<unsigned> basics;  // exisitng set of basic types
        for (; i < last_basic; ++i) {
            if ( basics.empty() || coin_flip( a.p_new_type() ) ) {
                // select new basic type
                unsigned new_basic;
                do {
                    new_basic = a.get_basic()();
                } while ( std::find( basics.begin(), basics.end(), new_basic ) != basics.end() );
                basics.push_back( new_basic );
                parms_and_rets.push_back( TypeGen::conc( new_basic ) );
            } else {
                // repeat existing basic type
                parms_and_rets.push_back( TypeGen::conc( random_in( basics ) ) );
            }
        }

        // struct parameters/returns
        std::vector<unsigned> structs;  // exisitng set of struct types
        for (; i < last_struct; ++i) {
            if ( structs.empty() || coin_flip( a.p_new_type() ) ) {
                // select new struct type
                unsigned new_struct;
                do {
                    new_struct = a.get_struct()();
                } while ( std::find( structs.begin(), structs.end(), new_struct ) != structs.end() );
                structs.push_back( new_struct );
                parms_and_rets.push_back( TypeGen::named( new_struct ) );
            } else {
                // repeat existing struct type
                parms_and_rets.push_back( TypeGen::named( random_in( structs ) ) );
            }
        }

        // randomize list order
        std::shuffle( parms_and_rets.begin(), parms_and_rets.end(), a.engine() );
        normalize_poly( parms_and_rets );
    }

    void print_decl( const FuncDecl& decl ) {
        const Type* rty = decl.returns();
        auto rtid = typeof(rty);
        if ( rtid == typeof<VoidType>() ) {
            // do nothing
        } else if ( rtid == typeof<TupleType>() ) {
            const TupleType* tty = as<TupleType>(rty);

            for (const Type* ity : tty->types()) {
                std::cout << *ity << " ";
            }
        } else {
            std::cout << *rty << " ";
        }

        std::cout << decl.name();
        if ( ! decl.tag().empty() ) {
            std::cout << "-" << decl.tag();
        }

        for (const Type* pty : decl.params()) {
            std::cout << " " << *pty;
        }

        std::cout << std::endl;
    }

    /// Generate and print all the declarations
    void generate_decls() {
        unsigned n_decls = 0;
        unsigned i_name = 0;

        while ( n_decls < a.n_decls() ) {
            // generate number of overloads with given name
            unsigned n_with_name = std::min( a.n_overloads()(), a.n_decls() - n_decls );
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
                            unsigned tries;
                            for (tries = 1; tries <= a.max_tries_for_unique(); ++tries) {
                                generate_parms_and_rets( parms_and_rets, n_parms, n_rets, n_poly );
                                // done if found a unique decl with this arity
                                if ( with_same_arity.insert( parms_and_rets ).second ) break;
                                parms_and_rets.clear();
                            }
                            // quit if too many tries
                            if ( tries > a.max_tries_for_unique() ) goto done_arity;

                            // generate list of parameters and returns
                            List<Type> parms( n_parms );
                            List<Type> rets( n_rets );
                            for (unsigned i = 0; i < n_parms; ++i) {
                                parms[i] = parms_and_rets[i].get();
                            }
                            for (unsigned i = 0; i < n_rets; ++i) {
                                rets[i] = parms_and_rets[n_parms + i].get();
                            }
                            parms_and_rets.clear();

                            std::string tag;
                            if ( n_with_name > 1 ) {
                                tag = "o" + std::to_string( i_with_name );
                            }

                            // store records of decl and print
                            auto decl = new FuncDecl{ name, tag, parms, rets };
                            print_decl( *decl );

                            funcs.push_back( decl );
                            if ( rets.size() == 1 ) {
                                auto rid = typeof( rets[0] );
                                if ( rid == typeof<ConcType>() ) {
                                    basic_funcs.push_back( decl );
                                } else if ( rid == typeof<NamedType>() ) {
                                    struct_funcs[ as<NamedType>( rets[0] )->name() ]
                                        .push_back( decl );
                                } else if ( rid == typeof<PolyType>() ) {
                                    poly_funcs.push_back( decl );
                                }
                            }

                            ++n_decls;
                            ++i_with_name;
                        }
                    }
                    done_arity:;
                }
            }
        }
    }

    static constexpr const Type* toplevel = (const Type*)0x1;

    /// Generates an expression with type matching `ty`, at top level if `ty == toplevel`.
    /// Returns return type of the expression
    const Type* generate_expr( const Type* ty = nullptr ) {
        cow_ptr<Environment> env;
        const FuncDecl* decl;

        // get function declaration from functions with appropriate return type
        if ( ty == nullptr || ty == toplevel ) {
            decl = random_in( funcs );
        } else {
            auto tid = typeof(ty);
            if ( tid == typeof<ConcType>() ) {
                unsigned n_funcs = basic_funcs.size() + poly_funcs.size();
                if ( n_funcs == 0 ) {  // leaf expression if nothing with compatible type
                    std::cout << *ty;
                    return ty;
                }

                unsigned i = random( n_funcs - 1 );
                if ( i < basic_funcs.size() ) { 
                    decl = basic_funcs[ i ];
                } else {
                    decl = poly_funcs[ i - basic_funcs.size() ];
                    bind( env, as<PolyType>(decl->returns()), ty );
                }
            } else if ( tid == typeof<NamedType>() ) {
                const auto& s_funcs = struct_funcs[ as<NamedType>(ty)->name() ];
                unsigned n_funcs = s_funcs.size() + poly_funcs.size();
                if ( n_funcs == 0 ) {  // leaf expression if nothing with compatible type
                    std::cout << *ty;
                    return ty;
                }

                unsigned i = random( n_funcs - 1 );
                if ( i < s_funcs.size() ) {
                    decl = s_funcs[ i ];
                } else {
                    decl = poly_funcs[ i - s_funcs.size() ];
                    bind( env, as<PolyType>(decl->returns()), ty );
                }
            } else assert(false && "invalid expression type");
        }

        // print call expr
        std::cout << decl->name() << "(";
        for (const Type* pty : decl->params()) {
            // replace parameter type according to environment
            pty = replace( env, pty );
            
            std::cout << " ";
            if ( coin_flip( ty == toplevel ? a.p_nested_at_root() : a.p_nested_deeper() ) ) {
                // nested call
                if ( const PolyType* ppty = as_safe<PolyType>(pty) ) {
                    // bind poly type to return from generation
                    const Type* rty = generate_expr();
                    if ( const PolyType* prty = as_safe<PolyType>(rty) ) {
                        bindClass( env, ppty, prty );
                    } else {
                        bind( env, ppty, rty );
                    }
                } else {
                    // generate according to concrete type
                    generate_expr( pty );
                }
            } else {
                // leaf expression
                auto pid = typeof( pty );
                if ( pid == typeof<ConcType>() ) {
                    // offset ConcType by random amount
                    int offset = a.basic_offset()();
                    if ( coin_flip( a.p_unsafe_offset() ) ) { offset *= -1; }

                    std::cout << ( as<ConcType>(pty)->id() + offset );
                } else if ( pid == typeof<NamedType>() ) {
                    // exact match for named type
                    std::cout << *as<NamedType>(pty);
                } else if ( pid == typeof<PolyType>() ) {
                    // generate random concrete type for PolyType
                    Type *aty = ( coin_flip() ? 
                        TypeGen::conc( a.get_basic()() ) : 
                        TypeGen::named( a.get_struct()() ) ).get();
                    bind( env, as<PolyType>(pty), aty );

                    std::cout << *aty;
                } else assert(false && "invalid parameter type");
            }

        }
        std::cout << " )";

        // substitute return type by environment prior to return
        return replace( env, decl->returns() );
    }

    void generate_exprs( bool is_toplevel = false ) {
        unsigned n_exprs = a.n_exprs();
        for(unsigned i_expr = 0; i_expr < n_exprs; ++i_expr) {
            generate_expr( toplevel );
            std::cout << std::endl;
        }
    }

public:
    BenchGenerator( int argc, char** argv ) 
        : a{ argc, argv }, partition{ a.engine() }, 
          funcs(), basic_funcs(), poly_funcs(), struct_funcs() {}

    void operator() () {
        print_args();
        generate_decls();
        std::cout << "\n%%\n" << std::endl;
        generate_exprs();
        collect();
    }
};

int main(int argc, char** argv) {
    BenchGenerator{ argc, argv }();
}
