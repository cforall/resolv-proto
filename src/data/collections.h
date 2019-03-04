#pragma once

// Copyright (c) 2015 University of Waterloo
//
// The contents of this file are covered under the licence agreement in 
// the file "LICENCE" distributed with this repository.

#include <functional>

/// Hasher for underlying type of pointers
template<typename T>
struct ByValueHash {
	std::size_t operator() (const T* p) const { return std::hash<T>()(*p); }
};

/// Equality checker for underlying type of pointers
template<typename T>
struct ByValueEquals {
	bool operator() (const T* p, const T* q) const { return *p == *q; }
};

/// Comparator for underlying type of pointers
template<typename T>
struct ByValueCompare {
	bool operator() (const T* p, const T* q) const { return *p < *q; }
};
