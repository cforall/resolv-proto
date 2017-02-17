#pragma once

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
};

/// List of generators
using GeneratorList = std::vector<unique_ptr<Generator>>;

/// Constant function; always returns copy of its constructor parameter
class ConstantGenerator : public Generator {
    unsigned x;  ///< Result to return
public:
    ConstantGenerator( unsigned x ) : x(x) {}
    
    unsigned operator() () final { return x; }
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
};

/// Wrapper for std::geometric_distribution
using GeometricRandomGenerator = RandomGenerator<std::geometric_distribution>;

/// Wrapper for std::uniform_int_distribution
using UniformRandomGenerator = RandomGenerator<std::uniform_int_distribution>;

/// Wrapper for std::discrete_distribution
using DiscreteRandomGenerator = RandomGenerator<std::discrete_distribution>;

/// Generates random values from multiple random distributions, choosing between them according 
/// to a discrete distribution
class StepwiseGenerator : public Generator {
    DiscreteRandomGenerator steps;  ///< Probabilities of each step
    GeneratorList gs;               ///< Step generators

public:
    StepwiseGenerator( DiscreteRandomGenerator::engine_type& e, std::initializer_list<double> ws,
                       GeneratorList&& gs ) : steps(e, ws), gs( move(gs) ) {
        assert(this->gs.size() == ws.size() && "Should be same number of weights as generators");
    }

    template<typename WIter>
    StepwiseGenerator( DiscreteRandomGenerator::engine_type& e, WIter wbegin, WIter wend,
                       GeneratorList&& gs ) : steps(e, wbegin, wend), gs( move(gs) ) {
        assert(this->gs.size() == wend - wbegin && "Should be same number of weights as generators");
    }

    unsigned operator() () final { return (*gs[ steps() ])(); }
};
