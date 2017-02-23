#pragma once

#include <map>

template<typename Gen>
std::map<unsigned, unsigned> weighted_partition( Gen& g, unsigned n ) {
	std::map<unsigned, unsigned> count;

	for (unsigned i = 0; i < n; ++i) {
		++count[ g() ];
	}

	return count;
}
