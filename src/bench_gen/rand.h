#pragma once

#include <random>
#include <utility>

/// Default random engine for project
using def_random_engine = std::mt19937;

/// Constructs a new random engine seeded from the system random device
def_random_engine new_random_engine() { return def_random_engine{ std::random_device{}() }; }

/// Constructs a new random engine from the given seed
def_random_engine new_random_engine( unsigned seed ) { return def_random_engine{ seed }; }

template<typename Result>
class Generator {
public:
    using result_type = Result;

    virtual ~Generator() {}

    /// Gets the next value from the generator
    virtual Result operator() () = 0;
};

/// Generates random values from the given engine [default def_random_engine]
/// according to the given distribution
template<typename Dist, typename Engine = def_random_engine>
class RandomGenerator : public Generator<typename Dist::result_type> {
    Engine& e;  ///< Random engine
    Dist g;     ///< Distribution

public:
    using result_type = typename Dist::result_type;
    using engine_type = Engine;

    /// Constructs a random number generator using the given engine and
    /// distribution construction parameters.
    template<typename... Args>
    RandomGenerator( Engine& e, Args&&... args ) : e(e), g(std::forward<Args>(args)...) {}

    /// Gets the next random number from the generator
    result_type operator() () final { return g(e); }
};

/// Wrapper for std::geometric_distribution<unsigned>
using GeometricRandomGenerator = RandomGenerator<std::geometric_distribution<unsigned>>;

/// Wrapper for std::uniform_int_distribution<unsigned>
using UniformRandomGenerator = RandomGenerator<std::uniform_int_distribution<unsigned>>;

/// Wrapper for std::discrete_distribution<unsigned>
using DiscreteRandomGenerator = RandomGenerator<std::discrete_distribution<unsigned>>;

