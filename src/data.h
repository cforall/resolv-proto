#pragma once

#include <cassert>
#include <functional>
#include <memory>
#include <set>
#include <typeinfo>
#include <typeindex>
#include <type_traits>
#include <unordered_set>
#include <utility>
#include <vector>

#include "gc.h"

/// use unique_ptr
using std::unique_ptr;
using std::make_unique;

/// Take ownership of a value
using std::move;

/// Shallow copy of an object
template<typename T>
inline T copy(const T& x) { return x; }

/// Copy an element through a unique pointer
template<typename T>
inline unique_ptr<T> copy(const unique_ptr<T>& p) {
    return unique_ptr<T>{ p ? new T{*p} : nullptr };
}

/// forward arguments
using std::forward;

/// Gets the unique identifier for a type
template<typename T>
std::type_index typeof() { return std::type_index{ typeid(T) }; } 

/// Gets the unique identifier for the type of the base of a pointer
template<typename B>
std::type_index typeof( const B* p ) { return std::type_index{ typeid(*p) }; }

/// Checks if is a pointer to D
template<typename D, typename B>
bool is( const B* p ) { return typeof<D>() == typeof(p); }

/// Checks if is a pointer some type derived from D
template<typename D, typename B>
bool is_derived( const B* p ) { return dynamic_cast<const D*>(p) != nullptr; }

/// Converts to a pointer to D without checking for safety
template<typename D, typename B>
D* as( B* p ) { return reinterpret_cast<D*>(p); }

/// Converts to a pointer to const D without checking for safety
template<typename D, typename B>
const D* as( const B* p ) { return reinterpret_cast<const D*>(p); }

/// Converts to a pointer to D, nullptr if not a pointer to D
template<typename D, typename B>
D* as_safe( B* p ) { return is<D>(p) ? as<D>(p) : nullptr; }

/// Converts to a pointer to const D, nullptr if not a pointer to D
template<typename D, typename B>
const D* as_safe( const B* p ) { return is<D>(p) ? as<D>(p) : nullptr; }

/// Converts to a pointer to D, asserting if not a pointer to D
template<typename D, typename B>
D* as_checked( B* p ) {
    if ( ! is<D>(p) ) assert(false);
    return as<D>(p);
}

/// Converts to a pointer to const D, asserting if not a pointer to D
template<typename D, typename B>
const D* as_checked( const B* p ) {
    if ( ! is<D>(p) ) assert(false);
    return as<D>(p);
}

/// Converts to a pointer to D without checking for safety.
template<typename D, typename B>
D* as_derived( B* p ) { return static_cast<D*>(p); }

/// Converts to a pointer to const D without checking for safety.
template<typename D, typename B>
const D* as_derived( const B* p ) { return static_cast<const D*>(p); }

/// Converts to a pointer to D, 
/// nullptr if not a pointer to some type derived from D
template<typename D, typename B>
D* as_derived_safe( B* p ) { return dynamic_cast<D*>(p); }

/// Converts to a pointer to const D, 
/// nullptr if not a pointer to some type derived from D
template<typename D, typename B>
const D* as_derived_safe( const B* p ) { return dynamic_cast<const D*>(p); }

/// Strips const off a reference type
template<typename T>
inline T& as_non_const( const T& r ) { return const_cast<T&>(r); }

/// Strips const off a pointer type
template<typename T>
inline T* as_non_const( const T* p ) { return const_cast<T*>(p); }

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
