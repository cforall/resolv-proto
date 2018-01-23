#pragma once

#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "rand.h"

#include "data/cast.h"
#include "data/mem.h"
#include "data/option.h"
#include "data/vector_map.h"
#include "driver/parser_common.h"

/// Kinds of types
enum TypeKind { Conc, Compound, Poly, N_KINDS };

/// Arguments to benchmark generator
class Args {
    option<def_random_engine> _engine;         ///< Underlying random engine, empty if uninitialized
    option<unsigned> _seed;                    ///< Random seed engine is built off
    option<unsigned> _n_decls;                 ///< Number of declarations to generate
    option<unsigned> _n_exprs;                 ///< Number of top-level expressions to generate
    unique_ptr<Generator> _n_overloads;        ///< Number of overloads to generate for each function
    unique_ptr<Generator> _n_parms;            ///< Number of function parameters
    unique_ptr<Generator> _n_rets;             ///< Number of function return types
    unique_ptr<Generator> _n_poly_types;       ///< Number of polymorphic type parameters
    option<unsigned> _max_tries_for_unique;    ///< Maximum number of tries for unique parameter list
    option<double> _p_new_type;                ///< Probability of a parameter/return type being new
    unique_ptr<Generator> _get_basic;          ///< Generates an index for a basic type
    std::vector<unsigned> _with_struct_params; ///< [i] is the last struct index with i parameters
    unique_ptr<Generator> _get_struct;         ///< Generates an index for a struct type
    unique_ptr<Generator> _get_nested_struct;  ///< Generates an index for a nested struct type
    option<double> _p_nested_at_root;          ///< Probability that a parameter of a root-level expression will be nested
    option<double> _p_nested_deeper;           ///< Probability that a parameter of a nested expression will be more deeply nested
    option<double> _p_unsafe_offset;           ///< Probability that a basic type will match unsafely.
    unique_ptr<Generator> _basic_offset;       ///< Magnitude of basic type offset.
    std::vector<double> _p_with_kind;          ///< Probabilities of drawing type of particular kind
    unique_ptr<Generator> _get_kind;           ///< Gets a type kind
    unique_ptr<Generator> _n_assns;            ///< Number of assertions for a given function
    option<double> _p_all_poly_assn_param;     ///< Probability that all parameters of an assertion will be polymorphic
    option<double> _p_all_poly_assn_return;    ///< Probability that all returns of an assertion will be polymorphic
    option<double> _p_poly_assn_nested;        ///< Probability that a generic type in an assertion will exist, rather than being replaced by a nested poly-type

public:
    /// Gets random engine, initializing from seed if needed.
    /// First use will fix a single engine
    def_random_engine& engine() {
        if ( ! _engine ) {
            _engine = _seed ? new_random_engine( *_seed ) : new_random_engine();
        }
        return *_engine;
    }

    const option<unsigned>& seed() { return _seed; }

    unsigned n_decls() { return _n_decls.value_or( 1500 ); }

    unsigned n_exprs() { return _n_exprs.value_or( 750 ); }

    Generator& n_overloads() {
        if ( ! _n_overloads ) {
            auto over_dists = { 0.5, 0.5 };
            GeneratorList over_gens(2);
            over_gens[0] = make_unique<UniformRandomGenerator>( engine(), 2, 120 );
            over_gens[1] = make_unique<ConstantGenerator>( 1 );
            _n_overloads = 
                make_unique<WeightedGenerator>( engine(), over_dists, move(over_gens) );
        }
        return *_n_overloads;
    }

    Generator& n_parms() {
        if ( ! _n_parms ) {
            auto parm_dists = { 0.05, 0.35, 0.55, 0.03, 0.01, 0.01 };
            _n_parms = make_unique<DiscreteRandomGenerator>( engine(), parm_dists );
        }
        return *_n_parms;
    }

    Generator& n_rets() {
        if ( ! _n_rets ) {
            auto ret_dists = { 0.15, 0.85 };
            _n_rets = make_unique<DiscreteRandomGenerator>( engine(), ret_dists );
        }
        return *_n_rets;
    }

    Generator& n_poly_types() {
        if ( ! _n_poly_types ) {
            auto poly_dists = { 0.9, 0.1 };
            _n_poly_types = make_unique<DiscreteRandomGenerator>( engine(), poly_dists );
        }
        return *_n_poly_types;
    }

    unsigned max_tries_for_unique() { return _max_tries_for_unique.value_or( 20 ); }

    double p_new_type() { return _p_new_type.value_or( 0.5 ); }

    Generator& get_basic() {
        if ( ! _get_basic ) {
            auto basic_dists = { 0.375, 0.25, 0.375 };
            GeneratorList basic_gens(3);
            basic_gens[0] = make_unique<UniformRandomGenerator>( engine(), 0, 2 );
            basic_gens[1] = make_unique<ConstantGenerator>( 3 );
            basic_gens[2] = make_unique<UniformRandomGenerator>( engine(), 4, 7 );
            _get_basic =
                make_unique<WeightedGenerator>( engine(), basic_dists, move(basic_gens) );
        }
        return *_get_basic;
    }

    std::vector<unsigned>& with_struct_params() {
        if ( _with_struct_params.empty() ) {
            _with_struct_params.reserve( 3 );
            _with_struct_params.push_back( 12 );
            _with_struct_params.push_back( 21 );
            _with_struct_params.push_back( 25 );
        }
        return _with_struct_params;
    }

    /// gets the number of struct parameters for s
    unsigned get_n_params( unsigned s ) {
        auto& ps = with_struct_params();
        for ( unsigned i = ps.size(); i > 0; --i ) {
            if ( s > ps[i-1] ) return i;
        }
        return 0;
    }

    Generator& get_struct() {
        if ( ! _get_struct ) {
            auto& ps = with_struct_params();
            if( ps.size() == 1 ) {
                _get_struct = make_unique<UniformRandomGenerator>( engine(), 0, ps[0] );
            } else {
                std::vector<double> struct_dists;
                GeneratorList struct_gens;
                struct_dists.reserve( ps.size() + 2 );
                struct_gens.reserve( ps.size() + 2 );

                struct_dists.push_back( 0.45 );
                struct_gens.emplace_back( new UniformRandomGenerator{ engine(), 0, ps[0] } );

                if ( ps[1] - ps[0] <= 2 ) {
                    struct_dists.push_back( 0.54 );
                    struct_gens.emplace_back( 
                        new UniformRandomGenerator{ engine(), ps[0] + 1, ps[1] } );
                } else {
                    struct_dists.push_back( 0.28 );
                    struct_gens.emplace_back( new ConstantGenerator{ ps[0] + 1 } );

                    struct_dists.push_back( 0.24 );
                    struct_gens.emplace_back( new ConstantGenerator{ ps[0] + 2 } );

                    struct_dists.push_back( 0.02 );
                    struct_gens.emplace_back( 
                        new UniformRandomGenerator{ engine(), ps[0] + 3, ps[1] } );
                }

                for ( unsigned i = 2; i < ps.size(); ++i ) {
                    struct_dists.push_back( 0.01 );
                    struct_gens.emplace_back(
                        new UniformRandomGenerator{ engine(), ps[i-1] + 1, ps[i] } );
                }

                _get_struct = make_unique<WeightedGenerator>( 
                    engine(), struct_dists.begin(), struct_dists.end(), move(struct_gens) );
            }
        }
        return *_get_struct;
    }

    Generator& get_nested_struct() {
        if ( ! _get_nested_struct ) {
            auto& ps = with_struct_params();
            if( ps.size() == 1 ) {
                _get_nested_struct = make_unique<UniformRandomGenerator>( engine(), 0, ps[0] );
            } else {
                std::vector<double> struct_dists;
                GeneratorList struct_gens;
                struct_dists.reserve( ps.size() + 2 );
                struct_gens.reserve( ps.size() + 2 );

                struct_dists.push_back( 0.945 );
                struct_gens.emplace_back( new UniformRandomGenerator{ engine(), 0, ps[0] } );

                if ( ps[1] - ps[0] <= 2 ) {
                    struct_dists.push_back( 0.054 );
                    struct_gens.emplace_back( 
                        new UniformRandomGenerator{ engine(), ps[0] + 1, ps[1] } );
                } else {
                    struct_dists.push_back( 0.028 );
                    struct_gens.emplace_back( new ConstantGenerator{ ps[0] + 1 } );

                    struct_dists.push_back( 0.024 );
                    struct_gens.emplace_back( new ConstantGenerator{ ps[0] + 2 } );

                    struct_dists.push_back( 0.002 );
                    struct_gens.emplace_back( 
                        new UniformRandomGenerator{ engine(), ps[0] + 3, ps[1] } );
                }

                for ( unsigned i = 2; i < ps.size(); ++i ) {
                    struct_dists.push_back( 0.001 );
                    struct_gens.emplace_back(
                        new UniformRandomGenerator{ engine(), ps[i-1] + 1, ps[i] } );
                }

                _get_nested_struct = make_unique<WeightedGenerator>( 
                    engine(), struct_dists.begin(), struct_dists.end(), move(struct_gens) );
            }
        }
        return *_get_nested_struct;
    }

    double p_nested_at_root() { return _p_nested_at_root.value_or( 0.06 ); }

    double p_nested_deeper() { return _p_nested_deeper.value_or( 0.3 ); }

    double p_unsafe_offset() { return _p_unsafe_offset.value_or( 0.25 ); }

    Generator& basic_offset() {
        if ( ! _basic_offset ) {
            _basic_offset = make_unique<GeometricRandomGenerator>( engine(), 0.25 );
        }
        return *_basic_offset;
    }

    std::vector<double>&  p_with_kind() {
        if ( _p_with_kind.empty() ) {
            _p_with_kind.push_back( 0.54 );
            _p_with_kind.push_back( 0.36 );
            _p_with_kind.push_back( 0.10 );
        } else while ( _p_with_kind.size() < N_KINDS ) {
            _p_with_kind.push_back( 0.0 );
        }
        return _p_with_kind;
    }

    Generator& get_kind() {
        if ( ! _get_kind ) {
            auto& ps = p_with_kind();
            _get_kind = make_unique<DiscreteRandomGenerator>( engine(), ps.begin(), ps.end() );
        }
        return *_get_kind;
    }

    Generator& n_assns() {
        if ( ! _n_assns ) {
            auto assn_dists = { 0.65, 0.25, 0.10 };
            GeneratorList assn_gens(3);
            assn_gens[0] = make_unique<ConstantGenerator>( 0 );
            assn_gens[1] = make_unique<UniformRandomGenerator>( engine(), 1, 10 );
            assn_gens[2] = make_unique<UniformRandomGenerator>( engine(), 11, 35 );
            _n_assns =
                make_unique<WeightedGenerator>( engine(), assn_dists, move(assn_gens) );
        }
        return *_n_assns;
    }

    double p_all_poly_assn_param() { return _p_all_poly_assn_param.value_or( 0.78 ); }

    double p_all_poly_assn_return() { return _p_all_poly_assn_return.value_or( 0.34 ); }

    double p_poly_assn_nested() { return _p_poly_assn_nested.value_or( 0.95 ); }

private:
    /// Checks if `name` matches the provided flag
    static bool is_flag( const char* flag, char* name ) {
        return std::strcmp( flag, name ) == 0;
    }

    /// Assigns the unsigned from `line` into `val` if present, prints an error otherwise.
    /// Will not overwrite val.
    static void read_unsigned( char* name, char* line, option<unsigned>& val ) {
        char* orig = line;
        unsigned ret;
        if ( parse_unsigned( line, ret ) 
                && ( match_whitespace( line ) || true ) 
                && is_empty( line ) ) {
            if ( ! val ) val = ret;
        } else {
            std::cerr << "ERROR: Expected unsigned int for " << name << ", got `" 
                      << orig << "'" << std::endl;
        }
    }

    /// Assigns the unsigned array from `line` into `val` if present, prints an error otherwise.
    /// Will not overwrite val.
    static void read_unsigned_array( char* name, char* line, std::vector<unsigned>& val ) {
        char* orig = line;
        std::vector<unsigned> ret;
        unsigned r;
        while ( parse_unsigned( line, r ) ) {
            ret.push_back( r );
            match_whitespace( line );
        }
        if ( ! ret.empty() && is_empty( line ) ) {
            if ( val.empty() ) val = move(ret);
        } else {
            std::cerr << "ERROR: Expected unsigned int array for " << name << ", got `"
                      << orig << "'" << std::endl;
        }
    }

    /// Assigns the decimal value from `line` into `val` if present, prints an error otherwise.
    /// Will not overwrite val.
    static void read_float( char* name, char* line, option<double>& val ) {
        char* orig = line;
        double ret;
        if ( parse_float( line, ret ) 
                && ( match_whitespace( line ) || true ) 
                && is_empty( line ) ) {
            if ( ! val ) val = ret;
        } else {
            std::cerr << "ERROR: Expected floating point for " << name << ", got `" 
                      << orig << "'" << std::endl;
        }
    }

    /// Assigns the decimal array from `line` into `val` if present, prints an error otherwise.
    /// Will not overwrite val.
    static void read_float_array( char* name, char* line, std::vector<double>& val ) {
        char* orig = line;
        std::vector<double> ret;
        double r;
        while ( parse_float( line, r ) ) {
            ret.push_back( r );
            match_whitespace( line );
        }
        if ( ! ret.empty() && is_empty( line ) ) {
            if ( val.empty() ) val = move(ret);
        } else {
            std::cerr << "ERROR: Expected floating point array for " << name << ", got `"
                      << orig << "'" << std::endl;
        }
    }

    /// Parses a generator, returning true, storing the result into `val`, and incrementing 
    /// `token` if so. `token` must not be null.
    bool parse_generator( char*& token, unique_ptr<Generator>& val ) {
        char *end = token;
        
        if ( match_string( end, "constant" ) ) {
            unsigned c;

            match_whitespace( end );
            if ( ! parse_unsigned( end, c ) ) return false;

            val = make_unique<ConstantGenerator>( c );
        } else if ( match_string( end, "uniform" ) ) {
            unsigned a, b;

            match_whitespace( end );
            if ( ! parse_unsigned( end, a ) ) return false;
            match_whitespace( end );
            if ( ! parse_unsigned( end, b ) ) return false;

            val = make_unique<UniformRandomGenerator>( engine(), a, b );
        } else if ( match_string( end, "discrete" ) ) {
            std::vector<double> ws;

            double w;
            match_whitespace( end );
            while ( parse_float( end, w ) ) {
                ws.push_back( w );
                match_whitespace( end );
            }
            if ( ws.empty() ) return false;
            
            val = make_unique<DiscreteRandomGenerator>( engine(), ws.begin(), ws.end() );
        } else if ( match_string( end, "geometric" ) ) {
            double p;

            match_whitespace( end );
            if ( ! parse_float( end, p ) ) return false;

            val = make_unique<GeometricRandomGenerator>( engine(), p );
        } else if ( match_string( end, "weighted" ) ) {
            std::vector<double> ws;
            GeneratorList gs;
            double w;
            unique_ptr<Generator> g;

            do {
                match_whitespace( end );
                if ( ! parse_float( end, w ) ) return false;
                match_whitespace( end );
                if ( ! parse_generator( end, g ) ) return false;

                ws.push_back( w );
                gs.push_back( move(g) );

                match_whitespace( end );
                if ( *end != ',' ) break;
                ++end;
            } while ( true );
            if ( ws.size() < 2 ) return false;

            val = make_unique<WeightedGenerator>( engine(), ws.begin(), ws.end(), move(gs) );
        } else return false;

        token = end;
        return true;
    }

    /// Assigns the generator described by `line` into `val` if present, prints an error 
    /// otherwise. Will not overwrite val.
    void read_generator( char* name, char* line, unique_ptr<Generator>& val ) {
        char* orig = line;
        unique_ptr<Generator> ret;
        if ( parse_generator( line, ret )
                && ( match_whitespace( line ) || true )
                && is_empty( line ) ) {
            if ( ! val ) val = move(ret);
        } else {
            std::cerr << "ERROR: Expected generator for " << name << ", got `" 
                      << orig << "'" << std::endl;
        }
        
    }

    /// Parses the given name and flag into the argument set
    void parse_flag(char* name, char* line) {
        if ( is_flag( "n_decls", name ) ) {
            read_unsigned( name, line, _n_decls );
        } else if ( is_flag( "n_exprs", name ) ) {
            read_unsigned( name, line, _n_exprs );
        } else if ( is_flag( "seed", name ) ) {
            read_unsigned( name, line, _seed );
        } else if ( is_flag( "n_overloads", name ) ) {
            read_generator( name, line, _n_overloads );
        } else if ( is_flag( "n_parms", name ) ) {
            read_generator( name, line, _n_parms );
        } else if ( is_flag( "n_rets", name ) ) {
            read_generator( name, line, _n_rets );
        } else if ( is_flag( "n_poly_types", name ) ) {
            read_generator( name, line, _n_poly_types );
        } else if ( is_flag( "max_tries_for_unique", name ) ) {
            read_unsigned( name, line, _max_tries_for_unique );
        } else if ( is_flag( "p_new_type", name ) ) {
            read_float( name, line, _p_new_type );
        } else if ( is_flag( "get_basic", name ) ) {
            read_generator( name, line, _get_basic );
        } else if ( is_flag( "with_struct_params", name ) ) {
            read_unsigned_array( name, line, _with_struct_params );
        } else if ( is_flag( "get_struct", name ) ) {
            read_generator( name, line, _get_struct );
        } else if ( is_flag( "get_nested_struct", name ) ) {
            read_generator( name, line, _get_struct );
        } else if ( is_flag( "p_nested_at_root", name ) ) {
            read_float( name, line, _p_nested_at_root );
        } else if ( is_flag( "p_nested_deeper", name ) ) {
            read_float( name, line, _p_nested_deeper );
        } else if ( is_flag( "p_unsafe_offset", name ) ) {
            read_float( name, line, _p_unsafe_offset );
        } else if ( is_flag( "basic_offset", name ) ) {
            read_generator( name, line, _basic_offset );
        } else if ( is_flag( "p_with_kind", name ) ) {
            read_float_array( name, line, _p_with_kind );
        } else if ( is_flag( "n_assns", name ) ) {
            read_generator( name, line, _n_assns );
        } else if ( is_flag( "p_all_poly_assn_param", name ) ) {
            read_float( name, line, _p_all_poly_assn_param );
        } else if ( is_flag( "p_all_poly_assn_return", name ) ) {
            read_float( name, line, _p_all_poly_assn_return );
        } else if ( is_flag( "p_poly_assn_nested", name ) ) {
            read_float( name, line, _p_poly_assn_nested );
        } else {
            std::cerr << "ERROR: Unknown argument `" << name << "'" << std::endl;
        }
    }

    /// Skips over all lowercase-ASCII + '_' chars at token
    bool match_name(char*& token) {
        if ( ( 'a' <= *token && *token <= 'z' ) || '_' == *token ) ++token;
        else return false;

        while ( ( 'a' <= *token && *token <= 'z' ) || '_' == *token ) ++token;

        return true;
    }

public:
    Args(int argc, char** argv) {
        int i;
        for (i = 1; i + 1 < argc; i += 2) {
            if ( argv[i][0] == '-' ) {
                parse_flag( argv[i] + 1, argv[i+1] );
            } else {
                std::cerr << "ERROR: Cannot parse [" << i << "]`" << argv[i] << "'" << std::endl;
                return;
            }
        }

        if ( i < argc ) {
            // parse input file
            std::ifstream file{ argv[i] };
            if ( ! file ) {
                std::cerr << "ERROR: Cannot open input file `" << argv[i] << "'" << std::endl;
                return;
            }

            std::string orig_line;
            while ( std::getline( file, orig_line ) ) {
                unique_ptr<char[]> line = make_unique<char[]>( orig_line.size() + 1 );
                std::strcpy( line.get(), orig_line.c_str() );
                char* s = line.get();
                
                // skip empty lines
                match_whitespace( s );
                if ( is_empty( s ) ) continue;

                // find name
                char *name = s;
                if ( ! match_name( s ) ) goto err;

                // find ':' delimiter (with optional preceding whitespace)
                switch ( *s ) {
                case ' ': case '\t':
                    *s = '\0';
                    ++s;
                    match_whitespace( s );
                    if ( ':' != *s ) goto err;
                    break;
                case ':':
                    *s = '\0';
                    break;
                default:
                    goto err;
                }

                // skip leading whitespace on line
                ++s;
                match_whitespace( s );

                parse_flag( name, s );
                
                continue;
                err: std::cerr << "ERROR: Cannot parse `" << orig_line << "'" << std::endl;
            }
        }
    }
};

namespace {
    template<typename T>
    struct ArgTraits {};

    template<>
    struct ArgTraits<unsigned> {
        static void print(std::ostream& out, unsigned x ) { out << x; }
    };

    template<>
    struct ArgTraits<std::vector<unsigned>> {
        static void print(std::ostream& out, const std::vector<unsigned>& v ) {
            if ( v.empty() ) return;
            out << v[0];
            for ( unsigned i = 1; i < v.size(); ++i ) { out << " " << v[i]; }
        }
    };

    template<>
    struct ArgTraits<double> {
        static void print(std::ostream& out, double p ) { out << p; }
    };

    template<>
    struct ArgTraits<std::vector<double>> {
        static void print(std::ostream& out, const std::vector<double>& v ) {
            if ( v.empty() ) return;
            out << v[0];
            for ( unsigned i = 1; i < v.size(); ++i ) { out << " " << v[i]; }
        }
    };

    template<>
    struct ArgTraits<Generator> {
        static void print(std::ostream& out, const Generator& g ) {
            auto gid = typeof( &g );

            if ( gid == typeof<ConstantGenerator>() ) {
                const auto& cg = *as<ConstantGenerator>( &g );
                out << "constant " << cg.x;
            } else if ( gid == typeof<UniformRandomGenerator>() ) {
                const auto& ug = *as<UniformRandomGenerator>( &g );
                out << "uniform " << ug.dist().a() << " " << ug.dist().b();
            } else if ( gid == typeof<DiscreteRandomGenerator>() ) {
                const auto& dg = *as<DiscreteRandomGenerator>( &g );
                out << "discrete";
                for ( double w : dg.dist().probabilities() ) {
                    out << " " << w;
                }
            } else if ( gid == typeof<GeometricRandomGenerator>() ) {
                const auto& dg = *as<GeometricRandomGenerator>( &g );
                out << "geometric " << dg.dist().p();
            } else if ( gid == typeof<WeightedGenerator>() ) {
                const auto& sg = *as<WeightedGenerator>( &g );
                out << "weighted ";

                auto ws = sg.probabilities();
                const auto& gs = sg.generators();
                out << ws[0] << " ";
                print( out, *gs[0] );
                for (unsigned i = 1; i < gs.size(); ++i) {
                    out << ", " << ws[i] << " ";
                    print( out, *gs[i] );
                }
            } else {
                assert(false && "unknown generator type");
            }
        }
    };

    template<typename T>
    struct ArgPrint {
        const char* name;
        const T& x;

        ArgPrint( const char* name, const T& x ) : name(name), x(x) {}
    };
}

template<typename T>
ArgPrint<T> arg_print( const char* name, const T& x ) { return ArgPrint<T>{ name, x }; }

template<typename T>
std::ostream& operator<< ( std::ostream& out, ArgPrint<T>&& p ) {
    out << p.name << ": ";
    ArgTraits<T>::print( out, p.x );
    return out;
}

