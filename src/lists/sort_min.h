#pragma once

#include <algorithm>
#include <functional>
#include <iterator>
#include <utility>

/// Reorders the input range in-place so that the minimum-value elements are in front; returns 
/// the iterator representing the last minimum value element
template<typename Iter, typename Compare>
Iter sort_mins( Iter begin, Iter end, Compare& lt ) {
	Iter min_pos = begin;
	for ( Iter i = begin + 1; i != end; ++i ) {
		if ( lt( *i, *min_pos ) ) {
			// new minimum cost; swap into first position
			min_pos = begin;
			std::iter_swap( min_pos, i );
		} else if ( ! lt( *min_pos, *i ) ) {
			// duplicate minimum cost; swap into next minimum position
			++min_pos;
			std::iter_swap( min_pos, i );
		}
	}
	return min_pos;
}

template<typename Iter, typename Compare>
inline Iter sort_mins( Iter begin, Iter end, Compare&& lt ) {
	return sort_mins( begin, end, lt );
}

/// sort_mins defaulted to use std::less
template<typename Iter>
inline Iter sort_mins( Iter begin, Iter end ) {
	return sort_mins( begin, end, std::less<typename std::iterator_traits<Iter>::value_type>{} );
}
