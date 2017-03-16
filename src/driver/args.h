#pragma once

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <fstream>

class Args {
    std::ifstream* in_;
    std::ofstream* out_;
    enum class Verbosity { Quiet, Filtered, Default, Verbose } verbosity_;
public:
    enum class Filter { None, Invalid, Unambiguous, Resolvable } filter_;

private:
    /// Gets the next single-char flag and returns it; returns '\0' if no flag
    /// Removes found flags from argument count/array.
    char get_flag( int& argc, char**& argv ) {
        if ( argc <= 1 ) return '\0';          // terminate if too few args
        if ( argv[1][0] != '-' ) return '\0';  // terminate if first arg not an arg
        if ( argv[1][1] == '\0' ) return '\0'; // terminate if first arg has no flag

        char f = argv[1][1];
        if ( argv[1][2] == '\0' ) { // only flag in arg; consume arg
            --argc; ++argv;
        } else {  // possibly more flags in arg; consume first flag
            ++argv[1];
            argv[1][0] = '-';
        }
        return f;
    }

    /// Checks a flag string starting with "--" against a value.
    bool flag_eq( const char* flag, const char* str ) {
        return std::strcmp( flag + 1, str ) == 0;
    }

    void usage( char* name ) {
        std::cerr << "Usage: " << name << "[-vq] [--filter (invalid|unambiguous|resolvable)] [ infile [ outfile ] ]" << std::endl;
        std::exit(1);
    }

public:
    Args( int argc, char** argv )
        : in_(nullptr),
          out_(nullptr),
          verbosity_(Verbosity::Default),
          filter_(Filter::None)
    {
        char* name = argv[0];

        while ( char f = get_flag(argc, argv) ) {
            if ( f == 'v' || f == '-' && flag_eq(argv[1], "verbose") ) {
                verbosity_ = Verbosity::Verbose;
                filter_ = Filter::None;
            } else if ( f == 'q' || f == '-' && flag_eq(argv[1], "quiet") ) {
                verbosity_ = Verbosity::Quiet;
                filter_ = Filter::None;
            } else if ( f == '-' ) {
                if ( flag_eq(argv[1], "filter") && argc > 2 ) {
                    if ( std::strcmp(argv[2], "invalid") == 0 ) {
                        filter_ = Filter::Invalid;
                    } else if ( std::strcmp(argv[2], "unambiguous") == 0 ) {
                        filter_ = Filter::Unambiguous;
                    } else if ( std::strcmp(argv[2], "resolvable") == 0 ) {
                        filter_ = Filter::Resolvable;
                    } else {
                        usage(name);
                    }
                    verbosity_ = Verbosity::Filtered;
                    argc -= 2; argv += 2;
                } else {
                    usage(name);
                }
            } else {
                usage(name);
            }
        }

        switch ( argc ) {
        case 3:
            out_ = new std::ofstream( argv[2] );
            if ( out_->fail() ) break;
            // fallthrough
        case 2:
            in_ = new std::ifstream( argv[1] );
            if ( in_->fail() ) break;
            // fallthrough
        case 1:
            return;
        default:
            usage(name);
        }
    }

    ~Args() {
        if ( in_ ) { delete in_; }
        if ( out_ ) { delete out_; }
    }

    std::istream& in() const { return in_ ? *in_ : std::cin; }
    std::ostream& out() const { return out_ ? *out_ : std::cout; }
    bool verbose() const { return verbosity_ == Verbosity::Verbose; }
    bool quiet() const { return verbosity_ == Verbosity::Quiet; }
    Filter filter() const { return filter_; }
};
