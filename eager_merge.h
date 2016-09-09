#pragma once

#include <algorithm>
#include <utility>
#include <vector>

#include "nway_merge.h"

/// Eagerly applies an n-way merge to produce an unsorted output list
template<typename T, 
         typename K = T, 
		 typename Extract = identity<T>, 
		 typename Q = std::vector<T>,
		 typename Valid = always_valid<Q> >
inline auto unsorted_eager_merge( std::vector<Q>&& queues ) 
		-> std::vector< std::pair< K, std::vector<T> > > {
	// TODO Use simple iteration, rather than the queue abstraction of NWayMerge
	auto merge = UnsortedNWayMerge<T, K, Extract, Q, Valid>{ std::move(queues) };
	
	std::vector< std::pair< K, std::vector<T> > > v;
	while ( ! merge.empty() ) {
		v.push_back( merge.top() );
		merge.pop();
	}
	return v;
}

template<typename T, 
         typename K = T, 
		 typename Extract = identity<T>, 
		 typename Q = std::vector<T>,
		 typename Valid = always_valid<Q> >
inline auto unsorted_eager_merge( const std::vector<Q>& queues ) 
		-> std::vector< std::pair< K, std::vector<T> > > {
	return unsorted_eager_merge( std::vector<Q>{ queues } );
}

/// Eagerly applies an n-way merge to produce a sorted output list
template<typename T, 
         typename K = T, 
		 typename Extract = identity<T>, 
		 typename Q = std::vector<T>,
		 typename Valid = always_valid<Q> >
inline auto eager_merge( std::vector<Q>&& queues ) 
		-> std::vector< std::pair< K, std::vector<T> > > {
	auto list = unsorted_eager_merge( std::move(queues) );
	std::sort( list.begin(), list.end(),
	           []( const std::pair< K, std::vector<T> >& a,
			       const std::pair< K, std::vector<T> >& b ) {
					   return a.first < b.first;
				   } );
	return list;
}

template<typename T, 
         typename K = T, 
		 typename Extract = identity<T>, 
		 typename Q = std::vector<T>,
		 typename Valid = always_valid<Q> >
inline auto eager_merge( const std::vector<Q>& queues ) 
		-> std::vector< std::pair< K, std::vector<T> > > {
	return eager_merge( std::vector<Q>{ queues } );
}
