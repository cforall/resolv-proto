#pragma once

#include <limits>
#include <vector>
#include <utility>

#include "merge.h"

#include "data/mem.h"

/// Result of a combo iterator -- one of reject this combination, reject all following combinations, 
/// or accept this combination.
enum class ComboResult { REJECT_THIS, REJECT_AFTER, ACCEPT };

/// Combo filter iterator that simply collects values into a vector, marking all values as valid.
/// @param T	The element type of the vector.
template<typename T>
class into_vector_combo_iter {
	std::vector<T> crnt_combo;
public:
	/// Outputs a vector of T
	using OutType = std::vector<T>;

	/// Adds the element to the current combination, always returning true for valid.
	ComboResult append( const T& x ) {
		crnt_combo.push_back( x );
		return ComboResult::ACCEPT;
	}

	/// Removes the last element of the current combination.
	void backtrack() { crnt_combo.pop_back(); }

	/// Returns a copy of the current combination.
	OutType finalize() { return crnt_combo; }
};

/// Filters combinations from qs by the given combo iterator. If the iterator rejects some prefix 
/// of a combination, it will not accept any combination with that prefix.
/// qs should not be empty
template<typename T, typename Q, typename ComboIter>
auto filter_combos( const std::vector<Q>& qs, ComboIter&& iter )
	-> std::vector<typename ComboIter::OutType> {
    unsigned n = qs.size();  // number of queues to combine

	std::vector<typename ComboIter::OutType> out;  // filtered output
	for ( auto& q : qs ) if ( q.empty() ) return out;  // return empty if any empty queue

	Indices inds;
	inds.reserve( n );
	inds.push_back( 0 );

	while (true) {
		unsigned i = inds.size() - 1;

		ComboResult flag = iter.append( qs[i][inds.back()] );
		
		if ( flag == ComboResult::ACCEPT ) {
			if ( i + 1 == n ) {
				// keep successful match of entire combination and continue iterating final place
				out.push_back( iter.finalize() );
				iter.backtrack();
			} else {
				// try to extend successful prefix
				inds.push_back( 0 );
				continue;
			}
		}

		if ( flag != ComboResult::REJECT_AFTER ) {
			++inds.back();
			// Try next combination if done with this one
			if ( inds.back() < qs[i].size() ) continue;
			// Otherwise done with the current row
		}

		// At this point, an invalid prefix is stored in inds and iter is in a state looking at 
		// all but the final value in inds. Now backtrack to next prefix:
		inds.pop_back();
		while ( ! inds.empty() ) {
			// try the next value at the previous index
			++inds.back();
			if ( inds.back() < qs[inds.size()-1].size() ) break;
			// if the previous index is finished, backtrack out
			iter.backtrack();
			inds.pop_back();
		}

		// Done if backtracked the entire array
		if ( inds.empty() ) return out;
	}
}
