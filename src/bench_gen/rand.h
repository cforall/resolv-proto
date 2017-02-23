#pragma once

#include <algorithm>
#include <cassert>
#include <initializer_list>
#include <random>
#include <vector>

#include "data/mem.h"

/// Default random engine for project
using def_random_engine = std::mt19937;

/// Constructs a new random engine seeded from the system random device
def_random_engine new_random_engine() { return def_random_engine{ std::random_device{}() }; }

/// Constructs a new random engine from the given seed
def_random_engine new_random_engine( unsigned seed ) { return def_random_engine{ seed }; }

/// Superclass for types generating an unsigned int from their operator()
class Generator {
public:
    virtual ~Generator() {}

    /// Gets the next value from the generator
    virtual unsigned operator() () = 0;

    /// Gets the minimum value produced by this generator
    virtual unsigned min() const = 0;

    /// Gets the maximum value produced by this generator
    virtual unsigned max() const = 0;
};

/// List of generators
using GeneratorList = std::vector<unique_ptr<Generator>>;

/// Constant function; always returns copy of its constructor parameter
struct ConstantGenerator : public Generator {
    const unsigned x;  ///< Result to return

    ConstantGenerator( unsigned x ) : x(x) {}
    
    unsigned operator() () final { return x; }

    unsigned min() const final { return x; }

    unsigned max() const final { return x; }
};

/// Generates random values from the given engine [default def_random_engine]
/// according to the given distribution (on unsigned)
template<template<typename> class Dist, typename Engine = def_random_engine>
class RandomGenerator : public Generator {
    Engine& e;         ///< Random engine
    Dist<unsigned> g;  ///< Distribution

public:
    using engine_type = Engine;

    /// Constructs a random number generator using the given engine and
    /// distribution construction parameters.
    template<typename... Args>
    RandomGenerator( Engine& e, Args&&... args ) : e(e), g(forward<Args>(args)...) {}

    unsigned operator() () final { return g(e); }

    unsigned min() const final { return g.min(); }

    unsigned max() const final { return g.max(); }

    const Dist<unsigned>& dist() const { return g; }
};

/// Wrapper for std::uniform_int_distribution
using UniformRandomGenerator = RandomGenerator<std::uniform_int_distribution>;

/// Wrapper for std::discrete_distribution
using DiscreteRandomGenerator = RandomGenerator<std::discrete_distribution>;

/// Wrapper for std::geometric_distribution
using GeometricRandomGenerator = RandomGenerator<std::geometric_distribution>;

/// Generates random values from multiple random distributions, choosing between them according 
/// to a discrete distribution
class StepwiseGenerator : public Generator {
    DiscreteRandomGenerator steps;  ///< Probabilities of each step
    GeneratorList gs;               ///< Step generators
    
public:
    StepwiseGenerator( DiscreteRandomGenerator::engine_type& e, std::initializer_list<double> ws,
                       GeneratorList&& gs ) : steps(e, ws), gs( move(gs) ) {
        assert(ws.size() > 1 && "Should be at least 2 steps");
        assert(this->gs.size() == ws.size() && "Should be same number of weights as generators");
    }

    template<typename WIter>
    StepwiseGenerator( DiscreteRandomGenerator::engine_type& e, WIter wbegin, WIter wend,
                       GeneratorList&& gs ) : steps(e, wbegin, wend), gs( move(gs) ) {
        assert(wend - wbegin > 1 && "Should be at least 2 steps");
        assert(this->gs.size() == wend - wbegin && "Should be same number of weights as generators");
    }

    unsigned operator() () final { return (*gs[ steps() ])(); }

    unsigned min() const final {
        unsigned m = gs[0]->min();
        for (unsigned i = 1; i < gs.size(); ++i) {
            m = std::min( m, gs[i]->min() );
        }
        return m;
    }

    unsigned max() const final {
        unsigned m = gs[0]->max();
        for (unsigned i = 1; i < gs.size(); ++i) {
            m = std::max( m, gs[i]->max() );
        }
        return m;
    }

    decltype( steps.dist().probabilities() ) probabilities() const { return steps.dist().probabilities(); }
    const GeneratorList& generators() const { return gs; }
};
