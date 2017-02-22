#pragma once

#include <vector>

class RandomPartitioner {
	/// C_memo[k - 2][n - 2k] is n choose k, if it has already been computed
	/// Memo table is shared across all instances of RandomParitioner
	static std::vector< std::vector<double> > C_memo;

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

public:
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
};
