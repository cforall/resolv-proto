#pragma once

// Copyright (c) 2015 University of Waterloo
//
// The contents of this file are covered under the licence agreement in 
// the file "LICENCE" distributed with this repository.

#include <ctime>

static inline long ms_between(std::clock_t start, std::clock_t end) {
	return (end - start) / (CLOCKS_PER_SEC / 1000);
}
