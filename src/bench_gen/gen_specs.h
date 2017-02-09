#pragma once

#include <algorithm>

#include "data/mem.h"
#include "data/vector_map.h"

/// Abstracts a (possibly randomly-generated) range of values
struct Range {
    virtual ~Range() = default;

    /// Number of elements in the range
    virtual unsigned size() = 0;

    /// Value at index i (must be < n)
    virtual unsigned at(unsigned i) = 0;
};

/// Produces a range of values following the histogram for a given random distribution
/// Dist's operator() should return a new unsigned
template<typename Dist>
class HistRange : public Range {
    unsigned n;                ///< Number of values generated
    VectorMap<unsigned> hist;  ///< Actual values generated

public:
    template<typename... Args>
    HistRange(unsigned n, Args&&... args) : n(n), hist() {
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

    unsigned size() final { return n; }

    unsigned at(unsigned i) final {
        using entry = VectorMap<unsigned>::value_type;
        // get element in range
        auto it = std::lower_bound( hist.begin(), hist.end(), i, 
            []( const entry& a, const unsigned& b ) {
                return a.second < b;
            } );
        // return difference from start
        return it - hist.begin();
    }
};

struct GenSpecs {
    unique_ptr<Range> decl_names;
};
