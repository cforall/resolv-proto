// Copyright (c) 2015 University of Waterloo
//
// The contents of this file are covered under the licence agreement in 
// the file "LICENCE" distributed with this repository.

#include <algorithm>
#include <array>
#include <cstdlib>
#include <cstddef>
#include <initializer_list>
#include <iostream>
#include <map>
#include <set>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "gen_args.h"
#include "name_gen.h"
#include "random_partitioner.h"
#include "weighted_partition.h"

#include "ast/ast.h"
#include "ast/decl.h"
#include "ast/forall.h"
#include "ast/forall_substitutor.h"
#include "ast/mutator.h"
#include "ast/type.h"
#include "ast/type_mutator.h"
#include "data/cast.h"
#include "data/debug.h"
#include "data/guard.h"
#include "data/list.h"
#include "data/mem.h"
#include "data/option.h"
#include "resolver/env.h"
#include "resolver/env_substitutor.h"
#include "resolver/type_unifier.h"
#include "resolver/unify.h"

/// Generates a type
struct TypeGen {
    TypeKind kind;
    unsigned i;

    TypeGen() : kind(N_KINDS), i(0) {}
    TypeGen( TypeKind k, unsigned i ) : kind(k), i(i) {}

    /// Generates a ConcType with the given index
    static TypeGen conc(unsigned i) { return TypeGen{ Conc, i }; }

    /// Generates a NamedType with the given index
    static TypeGen named(unsigned i) { return TypeGen{ Compound, i }; }

    /// Generates a PolyType with the given index
    static TypeGen poly(unsigned i) { return TypeGen{ Poly, i }; }

    /// Generates a new type of the underlying kind
    const Type* get( unique_ptr<Forall>& forall ) const {
        switch (kind) {
        case Conc: return new ConcType{ (int)i };
        case Compound: return new NamedType{ NameGen::get_lower( i ) };
        case Poly: return forall ? forall->add( NameGen::get_cap( i ) ) : nullptr;
        case N_KINDS: return nullptr;
        default: unreachable("Invalid kind"); return nullptr;
        }
    }

    /// Generates a new type of the underlying kind; will fail to generate polytypes
    const Type* get() const {
        unique_ptr<Forall> forall{};
        return get( forall );
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

/// Partially-generated function declaration
struct DeclPack {
    std::string name;
    std::string tag;
    List<Type> parms;
    List<Type> rets;
    unique_ptr<Forall> forall;

    DeclPack(const std::string& name, const std::string& tag, List<Type>&& parms, 
            List<Type>&& rets, unique_ptr<Forall>&& forall )
            : name(name), tag(tag), parms(move(parms)), rets(move(rets)), forall(move(forall)) {}
    
    FuncDecl* unpack() && {
        return new FuncDecl{ name, tag, move(parms), move(rets), move(forall) };
    }
};

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
    auto random_in( const C& c ) -> decltype( c[0] ) {
        return ::random_in( a.engine(), c );
    }

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
        comment_print( "p_ret_overload", a.p_ret_overload() );
        comment_print( "max_tries", a.max_tries() );
        comment_print( "p_new_type", a.p_new_type() );
        comment_print( "get_basic", a.get_basic() );
        comment_print( "with_struct_params", a.with_struct_params() );
        comment_print( "get_struct", a.get_struct() );
        comment_print( "get_nested_struct", a.get_struct() );
        comment_print( "p_nested_at_root", a.p_nested_at_root() );
        comment_print( "p_nested_deeper", a.p_nested_deeper() );
        comment_print( "p_unsafe_offset", a.p_unsafe_offset() );
        comment_print( "basic_offset", a.basic_offset() );
        comment_print( "p_with_kind", a.p_with_kind() );
        comment_print( "n_assns", a.n_assns() );
        comment_print( "p_all_poly_assn_param", a.p_all_poly_assn_param() );
        comment_print( "p_all_poly_assn_return", a.p_all_poly_assn_return() );
        comment_print( "p_poly_assn_nested", a.p_poly_assn_nested() );
    }

    /// gets a type from a chunk of a gen list
    const Type* get_type( GenList::iterator& it, unique_ptr<Forall>& forall ) {
        TypeGen& t = *it;
        ++it;
        
        if ( t.kind != Compound ) return t.get( forall );
        
        unsigned n_params = a.get_n_params( t.i );
        List<Type> params;
        params.reserve( n_params );
        for ( unsigned i = 0; i < n_params; ++i ) { params.push_back( get_type(it, forall) ); }

        return new NamedType{ NameGen::get_lower( t.i ), move(params) };
    }

    /// Generate a type
    void generate_type( GenList& out, unsigned& i_poly, std::vector<unsigned>& basics, 
            std::vector<GenList>& structs, bool nested = false ) {
        switch( (TypeKind)a.get_kind()() ) {
        case Conc: {
            if ( basics.empty() || coin_flip( a.p_new_type() ) ) {
                // select new basic type
                unsigned new_basic;
                do {
                    new_basic = a.get_basic()();
                } while ( std::find( basics.begin(), basics.end(), new_basic ) != basics.end() );
                basics.push_back( new_basic );
                out.push_back( TypeGen::conc( new_basic ) );
            } else {
                // repeat existing basic type
                out.push_back( TypeGen::conc( random_in( basics ) ) );
            }
            break;
        } case Compound: {
            if ( structs.empty() || coin_flip( a.p_new_type() ) ) {
                // generate new struct type
                GenList nout;
                while(true) {
                    unsigned new_struct = nested ? a.get_nested_struct()() : a.get_struct()();
                    nout.push_back( TypeGen::named( new_struct ) );
                    unsigned n_params = a.get_n_params( new_struct );

                    if ( n_params == 0 ) {
                        // done if no params and none matching yet
                        if ( std::find( structs.begin(), structs.end(), nout ) != structs.end() )
                            continue;
                        else break;
                    }

                    // fill in parameters
                    unsigned ni_poly = i_poly;
                    std::vector<unsigned> nbasics = basics;
                    std::vector<GenList> nstructs = structs;
                    for ( unsigned p = 0; p < n_params; ++p ) {
                        generate_type( nout/*, n_poly*/, ni_poly, nbasics, nstructs, true );
                    }
                    
                    // retry if already used this struct
                    if ( std::find( structs.begin(), structs.end(), nout ) != structs.end() )
                        continue;
                    
                    // update parameters otherwise
                    i_poly = ni_poly;
                    basics.swap( nbasics );
                    structs.swap( nstructs );
                    break;
                }
                structs.push_back( nout );
                out.insert( out.end(), nout.begin(), nout.end() );
            } else {
                // repeat existing struct type
                const GenList& s = random_in( structs );
                out.insert( out.end(), s.begin(), s.end() );
            }
            break;
        } case Poly: {
            if ( i_poly == 0 || coin_flip( a.p_new_type() ) ) {
                // select new poly type
                out.push_back( TypeGen::poly( i_poly++ ) );
            } else {
                // repeat existing poly type
                out.push_back( TypeGen::poly( random( i_poly-1 ) ) );
            }
            break;
        } case N_KINDS: unreachable("Invalid type kind generated.");
        }
    }

    /// Generate a parameter and return lists
    void generate_parms_and_rets( GenList& parms, GenList& rets, unsigned n_parms, unsigned n_rets ) {
        
        unsigned i_poly = 0;
        std::vector<unsigned> basics;
        std::vector<GenList> structs;

        for ( unsigned i = 0; i < n_parms; ++i ) {
            generate_type( parms, i_poly, basics, structs );
        }

        for ( unsigned i = 0; i < n_rets; ++i ) {
            generate_type( rets, i_poly, basics, structs );
        }
    }

    /// Replace polymorphic variables in a type with those bound to a different Forall
    class ReplacePoly : public TypeMutator<ReplacePoly> {
        def_random_engine& engine;
        const Forall& forall;
    public:
        ReplacePoly( def_random_engine& engine, const Forall& forall ) 
            : engine(engine), forall(forall) {}

        using TypeMutator<ReplacePoly>::visit;

        bool visit( const PolyType* t, const Type*& r ) {
            r = ::random_in( engine, forall.variables() );
            return true;
        }
    };

    /// Replace type with (possibly nested) type bound to a different Forall
    class ReplaceWithPoly : public TypeMutator<ReplaceWithPoly> {
        def_random_engine& engine;
        double p_nested;
        const Forall& forall;
        Env& env;

        // replaces t by a random type variable, returning false and setting r to nullptr 
        // if that random type variable doesn't bind
        bool replace( const Type* t, const Type*& r ) {
            const PolyType* candidate = ::random_in( engine, forall.variables() );
            Cost dummy;
            if ( unify( candidate, t, dummy, env ) ) {
                r = candidate;
                return true;
            } else {
                r = nullptr;
                return false;
            }
        }
    public:
        ReplaceWithPoly(
            def_random_engine& engine, double p_nested, const Forall& forall, Env& env )
            : engine(engine), p_nested(p_nested), forall(forall), env(env) {}
        
        using TypeMutator<ReplaceWithPoly>::visit;

        bool visit( const PolyType* t, const Type*& r ) {
            r = ::random_in( engine, forall.variables() );
            return true;
        }

        bool visit( const ConcType* t, const Type*& r ) {
            return replace( t, r );
        }

        bool visit( const NamedType* t, const Type*& r ) {
            if ( t->params().empty() || ! ::coin_flip( engine, p_nested ) ) {
                return replace( t, r );
            }
            return TypeMutator<ReplaceWithPoly>::visit( t, r );
        }
    };

    /// Replace a polymorphic type with a random compatible concrete type
    class ReplaceWithConcrete : public TypeMutator<ReplaceWithConcrete> {
        BenchGenerator& gen;
        Env& env;
        bool nested;
    public:
        ReplaceWithConcrete( BenchGenerator& gen, Env& env ) : gen(gen), env(env), nested(false) {}

        using TypeMutator<ReplaceWithConcrete>::visit;

        bool visit( const PolyType* t, const Type*& r ) {
            // Find type in environment
            ClassRef tr = env.getClass( t );
            // replace with bound type if bound
            if ( tr ) {
                const Type* tb = tr.get_bound();
                if ( tb ) {
                    r = tb;
                    return visit( r, r );
                }
            }
            // if unbound generate random types until a non-polymorphic one is generated
            unique_ptr<Forall> dummy{};
            unsigned i_poly;
            do {
                i_poly = 0;
                GenList tys;
                std::vector<unsigned> basics;
                std::vector<GenList> structs;

                gen.generate_type( tys, i_poly, basics, structs, nested );
                auto it = tys.begin();
                r = gen.get_type( it, dummy );
            } while ( i_poly > 0 );
            // bind type in environment
            env.bindType( tr, r );
            return true;
        }

        bool visit( const NamedType* t, const Type*& r ) {
            // track nesting of type for replacement
            auto guard = swap_in_scope( nested, true );
            return TypeMutator<ReplaceWithConcrete>::visit( t, r );
        }
    };

    void print_decl( const FuncDecl& decl ) {
        const Type* rty = decl.returns();
        if ( typeof(rty) != typeof<VoidType>() ) {
            rty->write( std::cout, ASTNode::Print::InputStyle );
            std::cout << ' ';
        }

        std::cout << decl.name();
        if ( ! decl.tag().empty() ) {
            std::cout << "-" << decl.tag();
        }

        for (const Type* pty : decl.params()) {
            std::cout << ' ';
            pty->write( std::cout, ASTNode::Print::InputStyle );
        }

        const Forall* forall = decl.forall();
        if ( forall ) for ( const Decl* asn : forall->assertions() ) {
            if ( ! is<FuncDecl>( asn ) ) continue;
            std::cout << " | ";
            print_decl( *as<FuncDecl>(asn) );
        }
    }

    /// Generate and print all the declarations
    void generate_decls() {
        unsigned n_decls = 0;
        unsigned i_name = 0;
        std::vector<DeclPack> decls;

        // generate assertion-less function decls
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

                    // used to ensure no duplicate functions
                    std::multimap<GenList, GenList> with_same_arity;
                    unsigned n_same_kind = ret_overloads.second;


                    GenList parms, rets;
                    parms.reserve( n_parms );
                    rets.reserve( n_rets );
                    
                    for ( unsigned i_same_kind = 0; i_same_kind < n_same_kind; ++i_same_kind ) {
                        unsigned tries;
                        for (tries = 1; tries <= a.max_tries(); ++tries) {
                            generate_parms_and_rets( parms, rets, n_parms, n_rets );
                            
                            auto range = with_same_arity.equal_range( parms );
                            if ( range.first == range.second 
                                    || ( coin_flip( a.p_ret_overload() )
                                        && std::find_if( range.first, range.second, 
                                            [&rets]( const auto& pair ) { 
                                                return pair.second == rets;
                                            } ) == range.second ) ) {
                                // found a unique parameter list or willing to overload on 
                                // return list and no matching signature; record and break
                                with_same_arity.emplace_hint( range.second, parms, rets );
                                break;
                            }
                            
                            parms.clear();
                            rets.clear();
                        }
                        // quit if too many tries
                        if ( tries > a.max_tries() ) break;

                        // generate list of parameters and returns
                        List<Type> parm_ts( n_parms );
                        List<Type> ret_ts( n_rets );
                        unique_ptr<Forall> forall{ new Forall{} };
                        GenList::iterator it = parms.begin();
                        for (unsigned i = 0; i < n_parms; ++i) {
                            parm_ts[i] = get_type( it, forall );
                        }
                        it = rets.begin();
                        for (unsigned i = 0; i < n_rets; ++i) {
                            ret_ts[i] = get_type( it, forall );
                        }
                        parms.clear();
                        rets.clear();

                        // replace forall with null if empty
                        if ( forall->empty() ) { forall.reset(); }

                        std::string tag;
                        if ( n_with_name > 1 ) {
                            tag = "o" + std::to_string( i_with_name );
                        }

                        decls.emplace_back( 
                            name, tag, move(parm_ts), move(ret_ts), move(forall) );

                        ++n_decls;
                        ++i_with_name;
                    }
                }
            }
        }

        // generate assertions for function decls
        for ( unsigned i = 0; i < decls.size(); ++i ) {
            // skip monomorphic functions
            if ( ! decls[i].forall ) continue;

            // generate random number of assertions
            unsigned n_assns = a.n_assns()();
            unsigned tries = 1;
            unsigned max_tries = a.max_tries() * 2;
            std::unordered_set<unsigned> decls_used;
            decls_used.insert( i );
            Env env{};
            for ( unsigned i_assn = 0; i_assn < n_assns && tries <= max_tries; ) {
                // look for a declaration not already used in this assertion list or base
                unsigned base_i = 0;
                while( tries <= max_tries ) {
                    base_i = random( decls.size() - 1 );
                    if ( decls_used.insert( base_i ).second ) break;
                    ++tries;
                }
                // fail if run out of tries
                if ( tries > max_tries ) break;

                const DeclPack& assn_base = decls[base_i];

                List<Type> aparms;
                List<Type> arets;
                ReplacePoly replace_poly{ a.engine(), *decls[i].forall };
                ReplaceWithPoly replace_with_poly{ 
                    a.engine(), a.p_poly_assn_nested(), *decls[i].forall, env };

                if ( ! assn_base.parms.empty() ) {
                    const Type* rty = replace_with_poly( assn_base.parms[0] );
                    if ( rty == nullptr ) {
                        ++tries;
                        goto retry_assn;
                    }
                    aparms.push_back( rty );
                    if ( coin_flip( a.p_all_poly_assn_param() ) ) {
                        for ( unsigned j = 1; j < assn_base.parms.size(); ++j ) {
                            rty = replace_with_poly( assn_base.parms[j] );
                            if ( rty == nullptr ) {
                                ++tries;
                                goto retry_assn;
                            }
                            aparms.push_back( rty );
                        }
                    } else {
                        for ( unsigned j = 1; j < assn_base.parms.size(); ++j ) {
                            aparms.push_back( replace_poly( assn_base.parms[j] ) );
                        }
                    }
                }

                if ( assn_base.parms.empty() || coin_flip( a.p_all_poly_assn_return() ) ) {
                    for ( unsigned j = 0; j < assn_base.rets.size(); ++j ) {
                        const Type* rty = replace_with_poly( assn_base.rets[j] );
                        if ( rty == nullptr ) {
                            ++tries;
                            goto retry_assn;
                        }
                        arets.push_back( rty );
                    }
                } else {
                    for ( unsigned j = 0; j < assn_base.rets.size(); ++j ) {
                        arets.push_back( replace_poly( assn_base.rets[j] ) );
                    }
                }

                decls[i].forall->addAssertion( 
                    new FuncDecl{ assn_base.name, move(aparms), move(arets) } );
                ++i_assn;
            retry_assn:; }
        }

        // finish declarations.
        for ( unsigned i = 0; i < decls.size(); ++i ) {
            FuncDecl* decl = move(decls[i]).unpack();

            print_decl( *decl );
            std::cout << std::endl;

            const Type* rty = decl->returns();
            funcs.push_back( decl );
            if ( rty->size() == 1 ) {
                auto rid = typeof( rty );
                if ( rid == typeof<ConcType>() ) {
                    basic_funcs.push_back( decl );
                } else if ( rid == typeof<NamedType>() ) {
                    struct_funcs[ as<NamedType>( rty )->name() ].push_back( decl );
                } else if ( rid == typeof<PolyType>() ) {
                    poly_funcs.push_back( decl );
                }
            }
        }
    }

    static constexpr const Type* toplevel = (const Type*)0x1;

    /// Generates an expression with type matching `ty`, at top level if `ty == toplevel`.
    /// Returns return type of the expression
    const Type* generate_expr( const Type* ty = nullptr ) {
        unique_ptr<Forall> forall;       ///< Local polymorphic variables
        Env env{};                       ///< Environment to track consistent bindings
        const FuncDecl* decl = nullptr;  ///< Function to call
        const Type* rtype;               ///< Return type of this function

        // get function declaration from functions with appropriate return type
        if ( ty == nullptr || ty == toplevel ) {
            // find random function, not void-returning unless at top level
            do {
                decl = random_in( funcs );
            } while ( ty == nullptr && decl->returns()->size() == 0 );
            forall = Forall::from( decl->forall() );
            // correct return type if possibly polymorphic
            rtype = forall ? ForallSubstitutor{ forall.get() }( decl->returns() ) : decl->returns();
        } else {
            // get set of compatible declarations
            auto tid = typeof(ty);
            List<FuncDecl>& conc_funcs =
                tid == typeof<ConcType>() ? basic_funcs :
                tid == typeof<NamedType>() ? struct_funcs[ as<NamedType>(ty)->name() ] :
                (unreachable("invalid expression type"), *(List<FuncDecl>*)0);
            unsigned n_funcs = conc_funcs.size() + poly_funcs.size();
            
            if ( n_funcs == 0 ) {  // leaf expression if nothing with compatible type
                ReplaceWithConcrete{ *this, env }.mutate( ty );
                std::cout << *ty;
                return ty;
            }

            // choose declaration from compatible
            unsigned i = random( n_funcs - 1 );
            if ( i < conc_funcs.size() ) {
                unsigned tries;
                for ( tries = 1; tries <= a.max_tries(); ++tries ) {
                    decl = conc_funcs[ i ];
                    forall = Forall::from( decl->forall() );
                    rtype = forall ? 
                        ForallSubstitutor{ forall.get() }( decl->returns() ) : decl->returns();
                    if ( tid == typeof<NamedType>() ) {
                        Cost::Element dummy = 0;
                        TypeUnifier unifier{ env, dummy };
                        rtype = unifier( ty, rtype );
                        if ( ! rtype ) {
                            i = random( conc_funcs.size() - 1 );
                            continue;
                        }
                        env = unifier.env;
                    }
                    break;
                }
                if ( tries > a.max_tries() ) {
                    // leaf expression if can't find compatible function
                    ReplaceWithConcrete{ *this, env }.mutate( ty );
                    std::cout << *ty;
                    return ty;
                }
            } else {
                decl = poly_funcs[ i - conc_funcs.size() ];
                forall = Forall::from( decl->forall() );
                // bind target type to new poly-type for this expression
                const PolyType* prtype = 
                    forall->get( as<PolyType>(decl->returns())->name() );
                ClassRef pr = env.getClass( prtype );
                env.bindType( pr, ty );
                rtype = ty;
            }
        }

        // print call expr
        std::cout << decl->name() << "(";
        for (const Type* pty : decl->params()) {
            std::cout << ' ';

            Cost::Element dummy;
            CostedSubstitutor sub{ env, dummy };
            sub.mutate( pty );

            bool nested = // should there be a nested call, or a leaf expression?
                coin_flip( ty == toplevel ? a.p_nested_at_root() : a.p_nested_deeper() );
            
            if ( sub.is_poly() ) {
                if ( nested ) {
                    // generate compatible expression and bind result to polymorphic type
                    const Type* rty = generate_expr( is<PolyType>(pty) ? nullptr : pty );
                    Cost dummy;
                    unify( pty, rty, dummy, env );
                } else {
                    // generate random compatible concrete leaf expression
                    ReplaceWithConcrete{ *this, env }.mutate( pty );
                    std::cout << *pty;
                }
            } else {
                if ( nested ) {
                    // generate nested expression with concrete type
                    generate_expr( pty );
                } else {
                    // generate leaf expression matching concrete type
                    auto pid = typeof( pty );
                    if ( pid == typeof<ConcType>() ) {
                        // offset ConcType by random amount for leaf expression
                        int offset = a.basic_offset()();
                        if ( coin_flip( a.p_unsafe_offset() ) ) { offset *= -1; }

                        std::cout << ( as<ConcType>(pty)->id() + offset );
                    } else if ( pid == typeof<NamedType>() ) {
                        // exact match for concrete named type
                        std::cout << *as<NamedType>(pty);
                    } else unreachable("invalid concrete parameter type");
                }
            }
        }
        std::cout << " )";

        return env.replace( rtype );
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
