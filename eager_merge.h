#pragma once

#include <algorithm>
#include <utility>
#include <vector>

#include "merge.h"

/// Eagerly applies an n-way merge to produce an unsorted output list
template<typename T, 
         typename K = T, 
		 typename Extract = identity<T>, 
		 typename Q = std::vector<T>,
		 typename Valid = always_valid<Q> >
inline auto unsorted_eager_merge( const std::vector<Q>& queues ) 
		-> std::vector< std::pair< K, std::vector<T> > > {
	unsigned n = queues.size();  // number of queues to merge

	std::vector< std::pair< K, std::vector<T> > > v;  // output queue

	for ( auto& q : queues ) if ( q.empty() ) return v;
	std::vector< unsigned > inds( n, 0 );  // indicies into merging queues

	Extract cost;  // cost function
	Valid valid;   // validity checking function
	K k = cost( queues[0][0] ); // initial cost
	for ( unsigned i = 1; i < n; ++i ) {
		k += cost( queues[i][0] );
	}

	while (true) {
		// Insert valid values into output queue
		if ( valid( queues, inds ) ) {
			std::vector<T> ts;
			ts.reserve( n );
			for ( unsigned i = 0; i < n; ++i ) {
				ts.push_back( queues[i][ inds[i] ] );
			}
			v.emplace_back( K{ k }, std::move(ts) );
		}

		// Iterate to next index
		unsigned i = n-1;
		while(true) {
			unsigned j = inds[i];
			
			// increment index if can be done without rollover
			if ( j+1 < queues[i].size() ) {
				k = k + cost( queues[i][j+1] ) - cost( queues[i][j] );
				inds[i] += 1;
				break;
			}

			// seen all combinations if about to roll over 0-queue
			if ( i == 0 ) return v; // done
			
			// otherwise roll over to next queue
			k = k + cost( queues[i][0] ) - cost( queues[i][j] );
			inds[i] = 0;
			--i;
		}
	}
}

/// Eagerly applies an n-way merge to produce a sorted output list
template<typename T, 
         typename K = T, 
		 typename Extract = identity<T>, 
		 typename Q = std::vector<T>,
		 typename Valid = always_valid<Q> >
inline auto eager_merge( const std::vector<Q>& queues ) 
		-> std::vector< std::pair< K, std::vector<T> > > {
	auto list = unsorted_eager_merge( std::move(queues) );
	std::sort( list.begin(), list.end(),
	           []( const std::pair< K, std::vector<T> >& a,
			       const std::pair< K, std::vector<T> >& b ) {
					   return a.first < b.first;
				   } );
	return list;
}
