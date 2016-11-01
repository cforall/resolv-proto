#pragma once

#include <algorithm>
#include <utility>
#include <vector>

#include "combos.h"
#include "merge.h"

/// Iteratively re-calculates cost on index change.
/// Interface matches default_on_index_change; `T`, `K`, `Extract` & `Q` are interpreted as in 
/// `for_each_cost_combo`.
template<typename T, 
         typename K = T, 
		 typename Extract = identity<T>, 
		 typename Q = std::vector<T> >
struct cost_on_index_change {
	K k;
	Extract cost;

	void init( const std::vector<Q>& qs ) {
		k = cost( qs[0][0] );
		for ( unsigned i = 1; i < qs.size(); ++i ) {
			k += cost( qs[i][0] );
		}
	}

	void recost( const std::vector<Q>& qs, unsigned i, unsigned j, unsigned new_j ) {
		k = k + cost( qs[i][new_j] ) - cost( qs[i][j] );
	}

	void inc( const std::vector<Q>& qs, unsigned i, unsigned j ) { recost( qs, i, j, j+1 ); }

	void reset( const std::vector<Q>& qs, unsigned i, unsigned j ) { recost( qs, i, j, 0 ); }
};

/// Given a vector of random-access queues of `T`, for each combination of  
/// elements from the input queues, call a function on the queue elements.
///
/// @param T	        The type of the queue elements.
/// @param F	        The function to apply to the combinations; given a  
/// 			        reference to `queues`, a std::vector<unsigned> holding 
///                     the indices, and a `K` representing the cost of the 
/// 					combination.
/// @param K			The cost type [default T]
/// @param Extract		Functor to extract `K` from `T` [default identity<T>]
/// @param Q	        The type of the queues of `T`
/// @param Valid		Combination validator.
/// 					Has functions matching the signatures of always_valid<Q>.
///
/// @param qs	        Queues to take combinations from
/// @param f	        Function to call on valid combinations
/// @param valid		Functor to validate combinations
template<typename T, typename K = T, typename Extract = identity<T>,
		 typename Q, typename F, typename Valid>
void for_each_cost_combo( const std::vector<Q>& qs, F f, Valid& valid ) {
	cost_on_index_change<T, K, Extract, Q> cost;
	auto f2 = [&f, &valid, &cost]( const std::vector<Q>& qs, const Indices& inds ) {
		if ( valid( qs, inds ) ) { f( qs, inds, cost.k ); }
	};

	for_each_combo<T>( qs, f2, cost );
}

/// `for_each_cost_combo` specialized to use default (always valid) validator.
template<typename T, typename K = T, typename Extract = identity<T>, 
		 typename Q, typename F>
inline void for_each_cost_combo( const std::vector<Q>& qs, F f ) {
	always_valid<Q> valid;
	return for_each_cost_combo<T, K, Extract>( qs, f, valid );
}

/// Eagerly applies an n-way merge to produce an unsorted output list.
/// Parameters have same meaning as `for_each_cost_combo`.
template<typename T, typename K = T, typename Extract = identity<T>, 
         typename Q, typename Valid>
auto unsorted_eager_merge( const std::vector<Q>& qs, Valid& valid ) 
		-> std::vector< std::pair< K, std::vector<T> > > {
	std::vector< std::pair< K, std::vector<T> > > v;  // output queue
	
	auto f = [&v]( const std::vector<Q>& qs, const Indices& inds, const K& k ) {
		// build combination
		std::vector<T> ts;
		ts.reserve( qs.size() );
		for ( unsigned i = 0; i < qs.size(); ++i ) {
			ts.push_back( qs[i][ inds[i] ] );
		}
		v.emplace_back( K{ k }, std::move(ts) );
	};
	
	for_each_cost_combo<T, K, Extract>( qs, f, valid );

	return v;
}

/// `unsorted_eager_merge` specialized to default (always valid) validator.
template<typename T, typename K = T, typename Extract = identity<T>, 
		 typename Q>
inline auto unsorted_eager_merge( const std::vector<Q>& qs )
		-> std::vector< std::pair< K, std::vector<T> > > {
	always_valid<Q> valid;
	return unsorted_eager_merge<T, K, Extract>( qs, valid );
}

/// Eagerly applies an n-way merge to produce a sorted output list.
/// Parameters have same meaning as `for_each_cost_combo`.
template<typename T, typename K = T, typename Extract = identity<T>,
		 typename Q, typename Valid>
inline auto eager_merge( const std::vector<Q>& qs, Valid& valid )
		-> std::vector< std::pair< K, std::vector<T> > > {
	auto list = unsorted_eager_merge<T, K, Extract>( qs, valid );
	std::sort( list.begin(), list.end(),
	           []( const std::pair< K, std::vector<T> >& a,
			       const std::pair< K, std::vector<T> >& b ) {
					   return a.first < b.first;
				   } );
	return list;
}

/// `eager_merge` specialized to default (always valid) validator.
template<typename T, typename K = T, typename Extract = identity<T>, 
		 typename Q>
inline auto eager_merge( const std::vector<Q>& qs )
		-> std::vector< std::pair< K, std::vector<T> > > {
	always_valid<Q> valid;
	return eager_merge<T, K, Extract>( qs, valid );
}
