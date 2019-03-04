#pragma once

// Copyright (c) 2015 University of Waterloo
//
// The contents of this file are covered under the licence agreement in 
// the file "LICENCE" distributed with this repository.

#include <vector>

#include "gc.h"

/// List of const T*
template<typename T>
using List = std::vector<const T*>;

template<typename T>
inline const GC& operator<< (const GC& gc, const List<T>& container) {
	for(auto&& obj : container) {
		gc << obj;
	}
	return gc;
}
