#pragma once

// Copyright (c) 2015 University of Waterloo
//
// The contents of this file are covered under the licence agreement in 
// the file "LICENCE" distributed with this repository.

#include <utility>
#include <vector>

#include "merge.h"

#include "data/range.h"

/// Default (do nothing) "on index change" behaviour for `for_each_combo`
template<typename Q>
struct default_on_index_change {
    
    /// Called before first combination, queues in `qs` guaranteed to be all non-empty.
    void init( const std::vector<Q>& qs ) {}

    /// Called before incrementing the counter for queue `i` of `qs`, currently valued `j`
    void inc( const std::vector<Q>& qs, unsigned i, unsigned j ) {}

    /// Called before resetting the counter for queue `i` of `qs` (currently valued `j`) to zero.
    void reset( const std::vector<Q>& qs, unsigned i, unsigned j ) {}
};

/// Given a vector of random-access queues of `T`, for each combination of elements from the 
/// input queues, call a function on the queue elements.
///
/// @param T	        The type of the queue elements.
/// @param Q	        The type of the queues of `T`
/// @param F	        The function to apply to the combinations;   
/// 			        given a reference to `qs` and a std::vector<unsigned> holding the indices.
/// @param OnChange     The behaviour to apply on index change.
///                     Has functions matching the signatures of default_on_index_change<Q>. 
///
/// @param qs	        The queues to take combinations from
/// @param f	        A function of type `F` to call
/// @param on_change    Behaviour to apply on index change.
template<typename T, typename Q, typename F, typename OnChange>
void for_each_combo( const std::vector<Q>& qs, F f, OnChange& on_change ) {
	unsigned n = qs.size();  // number of queues to merge

    for ( auto& q : qs ) if ( q.empty() ) return;
    Indices inds( n, 0 );  // indices into merging queues

    on_change.init( qs );

    while (true) {
        f( qs, inds );  // run loop iteration

        // iterate to next index
        unsigned i = n-1;
        while (true) {
            unsigned j = inds[i];

            // increment index if can be done without rollover
            if ( j+1 < qs[i].size() ) {
                on_change.inc( qs, i, j );
                inds[i] += 1;
                break;
            }

            // seen all combinations if about to roll over zero-queue
            if ( i == 0 ) return;

            // otherwise roll over to next queue
            on_change.reset( qs, i, j );
            inds[i] = 0;
            --i;
        }
    }
}

/// `for_each_combo` specialized to use default (no-op) OnChange. 
template<typename T, typename Q, typename F>
inline void for_each_combo( const std::vector<Q>& qs, F f ) {
    default_on_index_change<Q> on_change;
    for_each_combo<T>( qs, std::forward(f), on_change );
}

/// Wraps a vector with an added default first element.
/// Provided functions are sufficient for use in for_each_combo and friends.
///
/// @param T    Element type of the vector
/// @param Q    Underlying vector type 
template<typename T, typename Q = std::vector<T>>
class defaulted_vector {
    const T& head;  ///< First (default) element
    const Q& tail;  ///< Remainder of vector

public:
    using size_type = typename Q::size_type;

    defaulted_vector( const T& head, const Q& tail ) : head(head), tail(tail) {}

    const T& operator[] ( size_type i ) const { return ( i == 0 ) ? head : tail[i-1]; }

    size_type size() const { return tail.size() + 1; }

    bool empty() const { return false; }
};

/// Wraps a range with an added default first element.
/// Provided functions are sufficient for use in for_each_combo and friends.
///
/// @param T    Element type of the range
/// @param I    Underlying range iterator type
template<typename T, typename I>
class defaulted_range {
    const T& head;  ///< First (default) element
    range<I> tail;  ///< Remainder of range

public:
    using size_type = typename range<I>::size_type;

    defaulted_range( const T& head, range<I>&& tail ) : head(head), tail( std::move(tail) ) {}

    const T& operator[] ( size_type i ) const { return ( i == 0 ) ? head : tail[i-1]; }

    size_type size() const { return tail.size() + 1; }

    bool empty() const { return false; }
};
