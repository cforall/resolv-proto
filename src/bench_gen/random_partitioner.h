#pragma once

#include <array>
#include <cassert>
#include <cstddef>
#include <initializer_list>
#include <map>
#include <vector>
#include <utility>

#include "rand.h"

class RandomPartitioner {
	/// C_memo[k - 2][n - 2k] is n choose k, if it has already been computed
	/// Memo table is shared across all instances of RandomParitioner
	static std::vector< std::vector<double> > C_memo;

	/// Random engine
	def_random_engine& e;
	/// Cached partitioners for (n, k)
	std::map< std::pair<unsigned, unsigned>, DiscreteRandomGenerator > gs;

	/// Fills the memo row ki out to nend (inclusive). Assumes row exists.
	static void fill_memo_at(unsigned ki, unsigned nend) {
		std::vector<double>& k_memo = C_memo[ki];
		unsigned ni = k_memo.size();

		if ( ki == 0 ) {  // special case for first row
			double sum = ( ni == 0 ) ? 3.0 : k_memo[ ni - 1 ]; // C(3,2) == 3
			
			while ( ni <= nend ) {
				sum += ni + 2.0*ki + 3.0;  // n - 1, n = ni + 2*ki + 4
				k_memo.push_back( sum );
				++ni;
			}
			return;
		}

		std::vector<double>& k1_memo = C_memo[ki - 1];
		// make sure previous row filled in (needs n-1, all n's are +2 in previous row)
		if ( k1_memo.size() <= nend + 1 ) {
			fill_memo_at( ki - 1, nend + 1 );
		}

		// special case for beginning of row
		if ( ni == 0 ) {
			k_memo.push_back( 2.0 * k1_memo[1] ); // C(2k, k) = 2*C(2(k-1)+1, k-1)
			++ni;
		}

		double sum = k_memo[ ni - 1 ];
		while ( ni <= nend ) {
			sum += k1_memo[ ni + 1 ]; // C(n-1,k-1); n's are +2 in previous row
			k_memo.push_back( sum );
			++ni;
		}
	}

	/// C(n, k) produces n choose k, caching all non-trivial subresults
	static double C(unsigned n, unsigned k) {
		if ( k == 1 ) return (double)n;
		if ( k == 0 || k == n ) return 1.0;
		if ( k > n ) return 0.0;
		if ( k > n/2 ) return C(n, n - k);

		unsigned ki = k - 2;
		if ( ki >= C_memo.size() ) {
			// fill C_memo rows out to k
			for (unsigned j = C_memo.size(); j <= ki; ++j) {
				C_memo.emplace_back();
			}
		}

		std::vector<double>& k_memo = C_memo[ ki ];

		unsigned ni = n - 2*k;
		if ( ni >= k_memo.size() ) {
			// fill k_memo columns out to n
			fill_memo_at( ki, ni );
		}

		return k_memo[ni];
	}

	/// Gets the generator for (n, k), creating it if necessary
	DiscreteRandomGenerator& g( unsigned n, unsigned k ) {
		auto key = std::make_pair(n, k);
		auto it = gs.find( key );
		if ( it == gs.end() ) {
			std::vector<double> ws;
			ws.reserve( n-k+1 );
			ws.push_back( 0.0 );  // never a 0-size partition
			for (unsigned i = 1; i <= n-k; ++i) {
				// weight other partitions by number of combinations they leave
				ws.push_back( C(n-i-1, k-1) );
			}

			auto gen = DiscreteRandomGenerator{ e, ws.begin(), ws.end() };
			it = gs.emplace_hint( it, key, std::move(gen) );
		}
		return it->second;
	}

public:
	RandomPartitioner( def_random_engine& engine ) : e(engine), gs() {}

	/// Produces a random parition of n elements into k non-empty partitions.
	/// Places the (exclusive) end indices of the partitions in parts
	void get_nonempty( unsigned n, std::initializer_list<unsigned*> parts ) {
		unsigned k = parts.size();
		assert( n >= k );
		
		auto it = parts.begin();
		unsigned last = 0;
		for (unsigned i = 0; i < k-1; ++i) {
			// increase partition by random amount weighted by number of remaining combos
			**(it++) = last += g( n-last, k-1-i )();
		}
		**it = n;
	}

	/// Produces a random parition of n elements into k possibly-empty partitions.
	/// Places the (exclusive) end indices of the partitions in parts
	void get( unsigned n, std::initializer_list<unsigned*> parts ) {
		unsigned k = parts.size();
		get_nonempty( n + k, parts );

		unsigned i = 1;
		for (auto it = parts.begin(); it != parts.end(); ++it) {
			**it -= i++;
		}
	}
};
