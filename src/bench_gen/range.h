#pragma once

// Copyright (c) 2015 University of Waterloo
//
// The contents of this file are covered under the licence agreement in 
// the file "LICENCE" distributed with this repository.

#include <algorithm>
#include <vector>

#include "rand.h"

#include "data/mem.h"
#include "data/vector_map.h"

/// Abstracts a (possibly randomly-generated) range of values
class Range {
protected:
    unsigned n;  ///< Number of values in the range

    Range(unsigned n) : n(n) {}

public:
    virtual ~Range() = default;

    /// Number of elements in the range
    unsigned size() const { return n; }

    /// Value at index i (must be < n)
    virtual unsigned at(unsigned i) const = 0;

    /// Maximum value in range
    virtual unsigned max() const = 0;
};

/// Produces the same value repeatedly
class ConstantRange : public Range {
    unsigned c;   ///< Actual value generated

public:
    ConstantRange(unsigned n, unsigned c) : Range(n), c(c) {}

    unsigned at(unsigned) const final { return c; }

    unsigned max() const final { return c; }
};

/// Produces a range of values following the histogram for a given random distribution
/// Dist's operator() should return a new unsigned
template<typename Dist>
class HistRange : public Range {
    VectorMap<unsigned> hist;  ///< Actual values generated

public:
    template<typename... Args>
    HistRange(unsigned n, Args&&... args) : Range(n), hist() {
        Dist r{ forward<Args>(args)... };  // distribution generator
        // Generate histogram
        for ( unsigned i = 0; i < n; ++i ) {
            ++hist.at( r() );
        }
        // Replace histogram with prefix sum
        unsigned sum = 0;
        for ( auto& entry : hist ) {
            entry.second += sum;
            sum = entry.second;
        }
    }

    unsigned at(unsigned i) const final {
        using entry = VectorMap<unsigned>::const_value_type;
        // get element in range
        auto it = std::lower_bound( hist.begin(), hist.end(), i, 
            []( const entry& a, const unsigned& b ) {
                return a.second < b;
            } );
        // return difference from start
        return it - hist.begin();
    }

    unsigned max() const final { return hist.size() - 1; }
};

/// Produces a range of values with partition sizes drawn from a given random distribution
/// Dist's operator() should return a new unsigned
template<typename Dist>
class PartitionRange : public Range {
    std::vector<unsigned> hist;

public:
    template<typename... Args>
    PartitionRange(unsigned n, Args&&... args) : Range(n), hist() {
        Dist r{ forward<Args>(args)... };  // partition generator
        // Generate upper bounds of partitions
        unsigned sum;
        for ( sum = r(); sum < n; sum += r() ) {
            hist.push_back( sum );
        }
        n = sum;  // reset range upper bound
    }

    unsigned at(unsigned i) const final {
        auto it = std::lower_bound( hist.begin(), hist.end(), i );
        return it - hist.begin();
    }

    unsigned max() const final { return hist.size() - 1; }
};

/// First set of `m` values are drawn from range R, remainder are a one-to-one function
template<typename R>
class LongTailRange : public Range {
    R head;      ///< Underlying range

public:
    /// Builds a range of size `n` from the underlying range of size `m` (with other args Args),
    /// remainder c
    template<typename... Args>
    LongTailRange(unsigned n, unsigned m, Args&&... args)
        : Range(n), head(m, forward<Args>(args)...) {}
    
    unsigned at(unsigned i) const final {
        return i < head.size() ? head.at(i) : i - head.size() + head.max();
    }

    unsigned max() const final { return n - head.size() + head.max() + 1; }
};

/// Range suitable for generating declaration names
class DeclNameRange : public LongTailRange<PartitionRange<UniformRandomGenerator>> {
public:
    using Super = LongTailRange<PartitionRange<UniformRandomGenerator>>;

    DeclNameRange(unsigned n, unsigned m, unsigned high, UniformRandomGenerator::engine_type& engine)
        : Super( n, m, engine, 2, high ) {}
};
