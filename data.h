#pragma once

#include <functional>
#include <memory>
#include <set>
#include <unordered_set>
#include <utility>
#include <vector>

#include "gc.h"

/// Take ownership of a value
using std::move;

/// Shallow copy of an object
template<typename T>
inline T copy(const T& x) { return x; }

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

/// List of const T*
template<typename T>
using List = std::vector<const T*>;

/// Set of const T*
template<typename T>
using Set = std::unordered_set<const T*, ByValueHash<T>, ByValueEquals<T>>;

/// Sorted set of const T*
template<typename T>
using SortedSet = std::set<const T*, ByValueCompare<T>>;

/// Gets canonical reference for an object from the set;
/// If the object does not exist in the set, is inserted from the given arguments
template<typename T, typename... Args>
inline const T* get_canon(Set<T>& s, Args&&... args) {
    T* x = new T( std::forward<Args>(args)... );
    auto it = s.find( x );
    if ( it == s.end() ) {
        return *s.insert( it, x );
    } else {
        delete x;
        return *it;
    }
}

/// Gets canonical reference for an object from the set;
/// If the object does not exist in the set, is inserted from the given arguments
template<typename T, typename... Args>
inline const T* get_canon(SortedSet<T>& s, Args&&... args) {
    T* x = new T( std::forward<Args>(args)... );
    auto it = s.find( x );
    if ( it == s.end() ) {
        return *s.insert( it, x );
    } else {
        delete x;
        return *it;
    }
}

template<typename T>
inline const GC& operator<< (const GC& gc, const List<T>& container) {
	for(auto&& obj : container) {
		gc << obj;
	}
	return gc;
}

template<typename T>
inline const GC& operator<< (const GC& gc, const Set<T>& container) {
	for(auto&& obj : container) {
		gc << obj;
	}
	return gc;
}

template<typename T>
inline const GC& operator<< (const GC& gc, const SortedSet<T>& container) {
	for(auto&& obj : container) {
		gc << obj;
	}
	return gc;
}
