#pragma once

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <fstream>

class Args {
public:
    /// Possible verbosity levels
    enum class Verbosity { Quiet, Filtered, Testing, Default, Verbose };
    /// Possible filter types
    enum class Filter { None, Invalid, Unambiguous, Resolvable };

private:
    int argc;
    char** argv;
    std::ifstream* in_;
    std::ofstream* out_;
    Verbosity verbosity_;
    Filter filter_;
    bool bench_;

private:
    /// Consumes an argument.
    void consume_arg() { --argc; ++argv; }

    /// Gets the next single-char flag and returns it; returns '\0' if no flag
    /// Removes found flags from argument count/array.
    char get_flag() {
        if ( argc <= 1 ) return '\0';          // terminate if too few args
        if ( argv[1][0] != '-' ) return '\0';  // terminate if first arg not an arg
        if ( argv[1][1] == '\0' ) return '\0'; // terminate if first arg has no flag

        char f = argv[1][1];
        if ( argv[1][2] == '\0' ) { // only flag in arg; consume arg
            consume_arg();
        } else {  // possibly more flags in arg; consume first flag
            ++argv[1];
            argv[1][0] = '-';
        }
        return f;
    }

    /// Checks a flag string starting with "--" against a value.
    bool flag_eq( const char* str ) const { return std::strcmp( argv[1] + 1, str ) == 0; }

    /// Checks for a flag with short form only
    bool is_flag( char f, char s ) { return f == s; }

    /// Checks for a flag with long form only
    bool is_flag( char f, const char* l ) {
        if ( f == '-' && flag_eq(l) ) {
            consume_arg();
            return true;
        }
        return false;
    }

    /// Checks for a flag with both long and short forms
    bool is_flag( char f, char s, const char* l ) { return is_flag(f, s) || is_flag(f, l); }

    void usage( char* name ) {
        std::cerr << "Usage: " << name << "[-vq | --test | --filter (invalid|unambiguous|resolvable)] [--bench] [ infile [ outfile ] ]" << std::endl;
        std::exit(1);
    }

public:
    Args( int c, char** v )
        : argc(c), argv(v), 
          in_(nullptr),
          out_(nullptr),
          verbosity_(Verbosity::Default),
          filter_(Filter::None),
          bench_(false)
    {
        char* name = argv[0];

        while ( char f = get_flag() ) {
            if ( is_flag( f, 'v', "verbose" ) ) {
                verbosity_ = Verbosity::Verbose;
                filter_ = Filter::None;
            } else if ( is_flag( f, 'q', "quiet" ) ) {
                verbosity_ = Verbosity::Quiet;
                filter_ = Filter::None;
            } else if ( is_flag( f, "test" ) ) {
                verbosity_ = Verbosity::Testing;
                filter_ = Filter::None;
            } else if ( is_flag( f, "bench" ) ) {
                bench_ = true;
            } else if ( argc > 2 && is_flag( f, "filter" ) ) {
                if ( std::strcmp(argv[1], "invalid") == 0 ) {
                    filter_ = Filter::Invalid;
                } else if ( std::strcmp(argv[1], "unambiguous") == 0 ) {
                    filter_ = Filter::Unambiguous;
                } else if ( std::strcmp(argv[1], "resolvable") == 0 ) {
                    filter_ = Filter::Resolvable;
                } else {
                    usage(name);
                }
                consume_arg();
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
    Verbosity verbosity() const { return verbosity_; }
    bool verbose() const { return verbosity_ == Verbosity::Verbose; }
    bool quiet() const { return verbosity_ == Verbosity::Quiet; }
    bool testing() const { return verbosity_ == Verbosity::Testing; }
    Filter filter() const { return filter_; }
    bool bench() const { return bench_; }
};
