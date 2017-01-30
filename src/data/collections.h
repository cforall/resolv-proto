#pragma once

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
