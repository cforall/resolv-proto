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
#include "driver/parser_common.h"

/// Arguments to benchmark generator
class Args {
    option<def_random_engine> _engine;    ///< Underlying random engine, empty if uninitialized
    option<unsigned> _seed;               ///< Random seed engine is built off
    option<unsigned> _n_decls;            ///< Number of declarations to generate
    unique_ptr<Generator> _n_overloads;   ///< Number of overloads to generate for each function
    unique_ptr<Generator> _n_parms;       ///< Number of function parameters
    unique_ptr<Generator> _n_rets;        ///< Number of function return types
    unique_ptr<Generator> _n_poly_types;  ///< Number of polymorphic type parameters
    unique_ptr<Generator> _is_new_type;   ///< Returns a bool testing whether a type should be new
    unique_ptr<Generator> _get_basic;     ///< Generates an index for a basic type
    unique_ptr<Generator> _get_struct;    ///< Generates an index for a struct type

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

    Generator& n_overloads() {
        if ( ! _n_overloads ) {
            auto over_dists = { 0.5, 0.5 };
            GeneratorList over_gens(2);
            over_gens[0] = make_unique<UniformRandomGenerator>( engine(), 2, 120 );
            over_gens[1] = make_unique<ConstantGenerator>( 1 );
            _n_overloads = 
                make_unique<StepwiseGenerator>( engine(), over_dists, move(over_gens) );
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

    Generator& is_new_type() {
        if ( ! _is_new_type ) {
            _is_new_type = make_unique<UniformRandomGenerator>( engine(), 0, 1 );
        }
        return *_is_new_type;
    }

    Generator& get_basic() {
        if ( ! _get_basic ) {
            auto basic_dists = { 0.375, 0.25, 0.375 };
            GeneratorList basic_gens(3);
            basic_gens[0] = make_unique<UniformRandomGenerator>( engine(), 0, 2 );
            basic_gens[1] = make_unique<ConstantGenerator>( 3 );
            basic_gens[2] = make_unique<UniformRandomGenerator>( engine(), 4, 7 );
            _get_basic =
                make_unique<StepwiseGenerator>( engine(), basic_dists, move(basic_gens) );
        }
        return *_get_basic;
    }

    Generator& get_struct() {
        if ( ! _get_struct ) {
            _get_struct = make_unique<UniformRandomGenerator>( engine(), 0, 25 );
        }
        return *_get_struct;
    }

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
        } else if ( match_string( end, "stepwise" ) ) {
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

            val = make_unique<StepwiseGenerator>( engine(), ws.begin(), ws.end(), move(gs) );
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
        } else if ( is_flag( "is_new_type", name ) ) {
            read_generator( name, line, _is_new_type );
        } else if ( is_flag( "get_basic", name ) ) {
            read_generator( name, line, _get_basic );
        } else if ( is_flag( "get_struct", name ) ) {
            read_generator( name, line, _get_struct );
        } else {
            std::cerr << "ERROR: Unknown argument `" << name << "'" << std::endl;
        }
    }

    /// Skips over all lowercase-ASCII + '_' chars at token
    bool match_name(char*& token) {
        if ( 'a' <= *token && *token <= 'z' || '_' == *token ) ++token;
        else return false;

        while ( 'a' <= *token && *token <= 'z' || '_' == *token ) ++token;

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
            } else if ( gid == typeof<StepwiseGenerator>() ) {
                const auto& sg = *as<StepwiseGenerator>( &g );
                out << "stepwise ";

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

