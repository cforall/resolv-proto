#pragma once

#include <cstdlib>
#include <iostream>
#include <fstream>

class Args {
    std::ifstream* in_;
    std::ofstream* out_;

public:
    Args( int argc, char** argv ) {
        in_ = nullptr;
        out_ = nullptr;

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

        std::cerr << "Usage: " << argv[0] << " [ infile [ outfile ] ]" << std::endl;
        std::exit(1);
    }

    ~Args() {
        if ( in_ ) { delete in_; }
        if ( out_ ) { delete out_; }
    }

    std::istream& in() { return in_ ? *in_ : std::cin; }
    std::ostream& out() { return out_ ? *out_ : std::cout; }
};
