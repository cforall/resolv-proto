#pragma once

#include "rand.h"
#include "range.h"

#include "data/mem.h"
#include "data/option.h"

/// Arguments to benchmark generator
struct Args {
    unsigned n;
    unsigned m;
    unsigned high;
    option<unsigned> seed;

    Args(int argc, char** argv) 
    : n(1500), m(750), high(120), seed() {
        switch (argc) {
        case 5:
            seed = std::atoi(argv[4]);
        case 4:
            high = std::atoi(argv[3]);
        case 3:
            m = std::atoi(argv[2]);
        case 2:
            n = std::atoi(argv[1]);
        case 1:
            break;
        }
    }
};

/// Parameters to generate from
struct GenSpecs {
    def_random_engine random_engine;
    unique_ptr<Range> decl_names;
    unique_ptr<Generator<unsigned>> n_parms;
    unique_ptr<Generator<unsigned>> n_rets;

    GenSpecs(Args&& args) 
    : random_engine( args.seed ? new_random_engine( *args.seed ) : new_random_engine() ) {
        decl_names = make_unique<DeclNameRange>( args.n, args.m, args.high, random_engine );
        auto parm_dists = { 0.05, 0.35, 0.55, 0.03, 0.01, 0.01 };
        n_parms = make_unique<DiscreteRandomGenerator>( random_engine, move(parm_dists) );
        auto ret_dists = { 0.15, 0.85 };
        n_rets = make_unique<DiscreteRandomGenerator>( random_engine, move(ret_dists) );
    }
};
