#pragma once

#include <cstdlib>
#include <iostream>
#include <fstream>

class Args {
    std::ifstream* in_;
    std::ofstream* out_;
    bool verbose_;

    bool is_flag( char* arg, char flag ) {
        return arg[0] == '-' && arg[1] == flag && arg[2] == '\0';
    }

public:
    Args( int argc, char** argv )
        : in_(nullptr),
          out_(nullptr),
          verbose_(false)
    {
        if ( argc > 1 && is_flag( argv[1], 'v' ) ) {
            verbose_ = true;
            --argc; ++argv;
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
        }

        std::cerr << "Usage: " << argv[0] << "[-v] [ infile [ outfile ] ]" << std::endl;
        std::exit(1);
    }

    ~Args() {
        if ( in_ ) { delete in_; }
        if ( out_ ) { delete out_; }
    }

    std::istream& in() const { return in_ ? *in_ : std::cin; }
    std::ostream& out() const { return out_ ? *out_ : std::cout; }
    bool verbose() const { return verbose_; }
};
