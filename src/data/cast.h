#pragma once
// Various more-ergonomic cast wrappers for dealing with types by const pointer

#include <typeinfo>
#include <typeindex>

#include "debug.h"

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
    assume( is<D>(p), "checked cast failed" );
    return as<D>(p);
}

/// Converts to a pointer to const D, asserting if not a pointer to D
template<typename D, typename B>
const D* as_checked( const B* p ) {
    assume( is<D>(p), "checked cast failed" );
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
