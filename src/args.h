#pragma once

#include <cstdlib>
#include <iostream>
#include <fstream>

class Args {
    std::ifstream* in_;
    std::ofstream* out_;
    int verbosity_;

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

    void usage( char* name ) {
        std::cerr << "Usage: " << name << "[-vq] [ infile [ outfile ] ]" << std::endl;
        std::exit(1);
    }

public:
    Args( int argc, char** argv )
        : in_(nullptr),
          out_(nullptr),
          verbosity_(1)
    {
        char* name = argv[0];

        while ( char f = get_flag(argc, argv) ) {
            switch (f) {
            case 'v':
                verbosity_ = 2;
                break;
            case 'q':
                verbosity_ = 0;
                break;
            default:
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
    bool verbose() const { return verbosity_ == 2; }
    bool quiet() const { return verbosity_ == 0; }
};
