#pragma once

#include <random>
#include <utility>

/// Constructs a new random engine seeded from the system random device
std::mt19937 new_random_engine() { return std::mt19937{ std::random_device{}() }; }

/// Constructs a new random engine from the given seed
std::mt19937 new_random_engine( unsigned seed ) { return std::mt19937{ seed }; }

/// Generates random values from the given engine [default std::mt19937]
/// according to the given distribution
template<typename Dist, typename Engine = std::mt19937>
class RandomGenerator {
    Engine& e;  ///< Random engine
    Dist g;     ///< Distribution

public:
    using result_type = typename Dist::result_type;

    /// Constructs a random number generator using the given engine and
    /// distribution construction parameters.
    template<typename... Args>
    RandomGenerator( Engine& e, Args&&... args ) : e(e), g(std::forward<Args>(args)...) {}

    /// Gets the next random number from the generator
    result_type operator() () { return g(e); }
};

using GeometricRandomGenerator = RandomGenerator<std::geometric_distribution<unsigned>>;
