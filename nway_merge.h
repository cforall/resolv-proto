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
	bool operator() ( const std::vector< Q >& queues, 
	                  const std::vector< unsigned >& inds ) {
		return true;
	}
};

/// Wrapper around std::priority_queue with the proper number of template parameters
template<typename T>
using std_priority_queue = std::priority_queue<T>;


/// Given n > 1 sorted queues of T, produces a list of combination tuples ordered  
/// by the sum of their elements.
///
/// @param T        the element type
/// @param K        the sorting key type [default T]
/// @param Extract  the key extraction function [default identity<T>]
/// @param Q        the underlying queue type for the queues of T 
///                 [default std_vector]
/// @param Valid    The combination validator [default always_valid]
/// @param OutQ     the output queue type [default std_priority_queue]
template<typename T, 
         typename K = T, 
		 typename Extract = identity<T>, 
		 template<typename> class Q = std_vector,
		 typename Valid = always_valid< Q<T> >, 
         template<typename> class OutQ = std_priority_queue>
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
			return o.cost < cost;
		}
	};
	
	std::vector< Q<T> > queues;  ///< Underlying n queues
	OutQ< Combination > out_q;   ///< Combinations currently considered
	
	void update_cost(  )
	
	/// Inserts all valid successors for the given indexes, starting at lex_ind,
	/// Where the combination at inds has cost k
	void insert_next( const std::vector< unsigned >& inds, const K& k,
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
				out_q.emplace( std::move(new_inds), std::move(new_k), i );
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
		auto inds = std::vector<unsigned>{ n, 0 };
		if ( Valid{}( queues, inds ) ) {
			out_q.emplace( std::move(inds), std::move(k), 0 );
		} else {
			insert_next( inds, k, 0 );
		}
	}
	
public:
	NWaySumMerge( const std::vector< Q<T> >& queues )
		: queues( queues ), out_q() { init_out_q(); }
	
	NWaySumMerge( std::vector< Q<T> >&& queues )
		: queues( std::move(queues) ), out_q() { init_out_q(); }
	
	/// true iff there are no more combinations
	bool empty() const { return out_q.empty(); }
	
	unsigned size() const { return out_q.size(); }
	
	/// Get top combination [undefined behaviour if empty()] 
	std::pair< K, std::vector< T > > top() const {
		Combination& c = out_q.top();
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
		// Generate successors for top element
		Combination& c = out_q.top();
		insert_next( c.inds, c.cost, c.lex_ind );
		// Remove top from underlying queue
		out_q.pop();
	}
};

/// Wrapper around std::queue with the proper number of template parameters
template<typename T>
using std_queue = std::queue<T>;

/// N-way sum merge that produces its output tuples in an unordered fashion
template<typename T, 
         typename K = T, 
		 typename Extract = identity<T>, 
		 template<typename> class Q = std_vector,
		 typename Valid = always_valid< Q<T> > >
using UnorderedNWayMerge = NWayMerge<T, K, Extract, Q, Valid, std_queue>;
