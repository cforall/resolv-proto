#pragma once

#include <cstring>
#include <fstream>
#include <iostream>
#include <string>

#include "rand.h"

#include "data/mem.h"
#include "data/option.h"
#include "driver/parser_common.h"

/// Arguments to benchmark generator
class Args {
    option<def_random_engine> _engine;   ///< Underlying random engine, empty if uninitialized
    option<unsigned> _seed;              ///< Random seed engine is built off
    option<unsigned> _n_decls;           ///< Number of declarations to generate
    unique_ptr<Generator> _n_overloads;  ///< Number of overloads to generate for each function
    unique_ptr<Generator> _n_parms;      ///< Number of function parameters
    unique_ptr<Generator> _n_rets;       ///< Number of function return types

    /// Gets random engine, initializing from seed if needed.
    /// First use will fix a single engine
    def_random_engine& engine() {
        if ( ! _engine ) {
            _engine = _seed ? new_random_engine( *_seed ) : new_random_engine();
        }
        return *_engine;
    }

public:
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

private:
    /// Checks if `name` matches the provided flag
    static bool is_flag( const char* flag, char* name ) {
        return std::strcmp( flag, name ) == 0;
    }

    /// Assigns the unsigned from `line` into `val` if present, prints an error otherwise
    static void read_unsigned( char* name, char* line, option<unsigned>& val, 
                               bool do_override = true ) {
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

    /// Parses the given name and flag into the argument set
    void parse_flag(char* name, char* line) {
        if ( is_flag( "n_decls", name ) ) {
            read_unsigned( name, line, _n_decls );
        } else if ( is_flag( "seed", name ) ) {
            read_unsigned( name, line, _seed );
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
