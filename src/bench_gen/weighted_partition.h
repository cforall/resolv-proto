#pragma once

// Copyright (c) 2015 University of Waterloo
//
// The contents of this file are covered under the licence agreement in 
// the file "LICENCE" distributed with this repository.

#include <map>

template<typename Gen>
std::map<unsigned, unsigned> weighted_partition( Gen& g, unsigned n ) {
	std::map<unsigned, unsigned> count;

	for (unsigned i = 0; i < n; ++i) {
		++count[ g() ];
	}

	return count;
}
