#pragma once

#include <vector>

#include "rand.h"

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
    unsigned n_decls;
    unique_ptr<Generator> n_overloads;
    unique_ptr<Generator> n_parms;
    unique_ptr<Generator> n_rets;

    GenSpecs(Args&& args) 
    : random_engine( args.seed ? new_random_engine( *args.seed ) : new_random_engine() ) {
        n_decls = args.n;

        std::vector<double> over_dists;
        double percent_left = 1.0;
        double d = (double)args.m / args.n;
        over_dists.push_back( d );
        percent_left -= d;
        over_dists.push_back( d );
        GeneratorList over_gens;
        over_gens.push_back( make_unique<UniformRandomGenerator>( random_engine, 2, args.high ) );
        over_gens.push_back( make_unique<ConstantGenerator>( 1 ) );
        n_overloads = make_unique<StepwiseGenerator>( random_engine, over_dists.begin(), over_dists.end(), move(over_gens) );

        auto parm_dists = { 0.05, 0.35, 0.55, 0.03, 0.01, 0.01 };
        n_parms = make_unique<DiscreteRandomGenerator>( random_engine, parm_dists );
        
        auto ret_dists = { 0.15, 0.85 };
        n_rets = make_unique<DiscreteRandomGenerator>( random_engine, ret_dists );
    }
};
