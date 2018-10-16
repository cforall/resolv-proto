#pragma once

#include <set>
#include <unordered_set>

#include "collections.h"
#include "gc.h"
#include "mem.h"

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
    T* x = new T( forward<Args>(args)... );
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
    T* x = new T( forward<Args>(args)... );
    auto it = s.find( x );
    if ( it == s.end() ) {
        return *s.insert( it, x );
    } else {
        delete x;
        return *it;
    }
}

template<typename T>
inline const GC& operator<< (const GC& gc, const Set<T>& container) {
	for(auto obj : container) {
		gc << obj;
	}
	return gc;
}

template<typename T>
inline const GC& operator<< (const GC& gc, const SortedSet<T>& container) {
	for(auto obj : container) {
		gc << obj;
	}
	return gc;
}
