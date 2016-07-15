#pragma once

#include <algorithm>
#include <cassert>
#include <utility>
#include <vector>

/// Identity functor for T
template<typename T>
struct identity {
	const T& operator() ( const T& x ) { return x; }
};

/// Default validator for n-way merge.
///
/// Validators are functors which, given a reference to the queues and a set of 
/// indices for those queues (guaranteed to be addressable), return whether or not 
/// the combination is valid.
///
/// The default validator always returns valid.
/// 
/// @param Q  The underlying queue type for for the n-way merge
template<typename Q>
struct always_valid {
	/// Returns true.
	bool operator() ( const std::vector<Q>& queues, 
	                  const std::vector<unsigned>& inds ) {
		return true;
	}
};

/// Given n > 1 sorted queues of T, produces a list of combination tuples ordered  
/// by the sum of their elements.
///
/// @param T        the element type
/// @param K        the sorting key type [default T]
/// @param Extract  the key extraction function [default identity<T>]
/// @param Q        the underlying queue type for the queues of T 
///                 [default std::vector<T>]
/// @param Valid    The combination validator [default always_valid]
/// @param Sorted   Should this merge be in sorted order?
template<typename T, 
         typename K = T, 
		 typename Extract = identity<T>, 
		 typename Q = std::vector<T>,
		 typename Valid = always_valid<Q>, 
         bool Sorted = true>
class NWayMerge {
	/// Combination of queue elements
	struct Combination {
		/// Indices of elements in backing queues
		std::vector<unsigned> inds;
		/// Cost of the combination (sum of elements in appropriate queues)
		K cost;
		/// First queue index where this combination can be increased
		/// (for uniqueness)
		unsigned lex_ind;
		
		Combination() = default;
		Combination( const Combination& ) = default;
		Combination( Combination&& ) = default;
		Combination& operator= ( const Combination& ) = default;
		Combination& operator= ( Combination&& ) = default;
		
		Combination( std::vector< unsigned >&& inds, K&& cost, unsigned lex_ind )
			: inds( std::move(inds) ), cost( std::move( cost ) ), lex_ind( lex_ind ) {}
		
		bool operator< ( const Combination& o ) const {
			// Switch sense of cost so that combinations are sorted by min-cost
			// rather than max-cost
			return o.cost < cost;
		}
	};
	
	std::vector<Q> queues;           ///< Underlying n queues
	std::vector<Combination> out_q;  ///< Combinations currently considered
	
	/// Inserts all valid successors for the given indexes, starting at lex_ind,
	/// Where the combination at inds has cost k
	void insert_next( const std::vector<unsigned>& inds, const K& k,
	                  unsigned lex_ind ) {
		unsigned n = queues.size();
		Extract cost;
		Valid valid;
		
		// Breadth-first generation of subsequent combinations with greater or 
		// equal lex_ind
		for ( unsigned i = lex_ind; i < n; ++i ) {
			unsigned j = inds[i];
			
			// skip combination if queue would be empty
			if ( j+1 >= queues[i].size() ) continue;
			
			// calculate new combination
			std::vector<unsigned> new_inds = inds;
			++new_inds[i];
			K new_k = k + cost( queues[i][j+1] ) - cost( queues[i][j] );
			
			// add combination (or successors) to output queue
			if ( valid( queues, new_inds ) ) {
				out_q.emplace_back( std::move(new_inds), std::move(new_k), i );
				if ( Sorted ) std::push_heap( out_q.begin(), out_q.end() );
			} else {
				insert_next( new_inds, new_k, lex_ind );
			}
		}
	}
	
	/// Places the first element on the output queue
	void init_out_q() {
		unsigned n = queues.size();
		assert( n > 1 );
		
		// Ensure all queues non-empty
		for ( auto& q : queues ) if ( q.empty() ) return;
		
		// Calculate cost of min element
		Extract cost;
		K k = cost( queues[0][0] );
		for ( unsigned i = 1; i < n; ++i ) {
			k += cost( queues[i][0] );
		}
		
		/// Put min-element (or successors) on out queue
		auto inds = std::vector<unsigned>( n, 0 );
		if ( Valid{}( queues, inds ) ) {
			out_q.emplace_back( std::move(inds), std::move(k), 0 );
			// no need to sort heap, only one element
		} else {
			insert_next( inds, k, 0 );
		}
	}
	
public:
	NWayMerge( const std::vector<Q>& queues )
		: queues( queues ), out_q() { init_out_q(); }
	
	NWayMerge( std::vector<Q>&& queues )
		: queues( std::move(queues) ), out_q() { init_out_q(); }
	
	/// true iff there are no more combinations
	bool empty() const { return out_q.empty(); }
	
	unsigned size() const { return out_q.size(); }
	
	/// Get top combination [undefined behaviour if empty()] 
	std::pair< K, std::vector<T> > top() const {
		const Combination& c = out_q.front();
		unsigned n = queues.size();
		
		std::vector<T> ts;
		ts.reserve( n );
		
		for ( unsigned i = 0; i < n; ++i ) {
			ts.push_back( queues[i][ c.inds[i] ] );
		}
		
		return { K{ c.cost }, std::move(ts) };
	}
	
	/// Remove top combination (generates any subsequent combinations)
	void pop() {
		// Generate successors for top element
		const Combination& c = out_q.front();
		insert_next( c.inds, c.cost, c.lex_ind );
		// Remove top from underlying queue
		if ( Sorted ) std::pop_heap( out_q.begin(), out_q.end() );
		else std::swap( out_q.front(), out_q.back() );
		out_q.pop_back();
	}
};

/// N-way merge that produces its output tuples in an unordered fashion
template<typename T, 
         typename K = T, 
		 typename Extract = identity<T>, 
		 typename Q = std::vector<T>,
		 typename Valid = always_valid<Q> >
using UnsortedNWayMerge = NWayMerge<T, K, Extract, Q, Valid, false>;
