#pragma once

#include <cassert>
#include <queue>
#include <utility>
#include <vector>

/// Identity functor for T
template<typename T>
struct identity {
	const T& operator() ( const T& x ) { return x; }
};

/// Wrapper around std::vector with proper number of template parameters
template<typename T>
using std_vector = std::vector<T>;

/// Given n > 1 sorted queues of T, produces a list of combination tuples ordered  
/// by the sum of their elements.
///
/// @param T        the element type
/// @param K        the sorting key type [default T]
/// @param Extract  the key extraction function [default identity<T>]
/// @param Q        the underlying queue type for the queues of T  [default std_vector]
template<typename T, 
         typename K = T, 
		 typename Extract = identity<T>, 
		 template<typename> class Q = std_vector>
class NWaySumMerge {
	
	/// Combination of queue elements
	struct Combination {
		/// Indices of elements in backing queues
		std::vector< unsigned > inds;
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
			return cost > o.cost;
		}
	};
	
	std::vector< Q<T> > queues;               ///< Underlying n queues
	std::priority_queue< Combination > heap;  ///< Combinations currently considered
	
	/// Places the first element on the heap
	void init_heap() {
		unsigned n = queues.size();
		assert( n > 1 );
		
		// Ensure all queues non-empty
		for ( auto& q : queues ) if ( q.empty() ) return;
		
		// Calculate cost of min element
		Extract ex;
		K cost = ex( queues[0][0] );
		for ( unsigned i = 1; i < n; ++i ) {
			cost += ex( queues[i][0] );
		}
		
		/// Put min-element in heap
		heap.emplace( std::vector<unsigned>{ n, 0 }, std::move(cost), 0 );
	}
	
public:
	NWaySumMerge( const std::vector< Q<T> >& queues )
		: queues( queues ), heap() { init_heap(); }
	
	NWaySumMerge( std::vector< Q<T> >&& queues )
		: queues( std::move(queues) ), heap() { init_heap(); }
	
	/// true iff there are no more combinations
	bool empty() const { return heap.empty(); }
	
	/// Get top combination [undefined behaviour if empty()] 
	std::pair< K, std::vector< T > > top() const {
		Combination& c = heap.top();
		unsigned n = queues.size();
		
		std::vector< T > ts;
		els.reserve( n );
		
		for ( unsigned i = 0; i < n; ++i ) {
			ts.push_back( queues[i][ c.inds[i] ] );
		}
		
		return { K{ c.cost }, std::move(ts) };
	}
	
	/// Remove top combination (generates any subsequent combinations)
	void pop() {
		Combination& c = heap.top();
		unsigned n = queues.size();
		
		/// Breadth-first generation of subsequent combinations with greater or 
		/// equal lex_ind
		Extract ex;
		for ( unsigned i = c.lex_ind; i < n; ++i ) {
			unsigned j = c.inds[i];
			if ( queues[i].size() > j ) {
				std::vector<unsigned> inds = c.inds;
				++inds[i];
				
				heap.emplace( std::move(inds),
				              c.cost + ex( queues[i][j+1] ) - ex( queues[i][j] ),
							  i );
			}
		}
		
		/// Remove top from underlying heap
		heap.pop();
	}
};
