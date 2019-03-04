#pragma once

// Copyright (c) 2015 University of Waterloo
//
// The contents of this file are covered under the licence agreement in 
// the file "LICENCE" distributed with this repository.

#include <cstddef>

#include "type.h"

#include "data/cast.h"
#include "data/debug.h"

/// Visitor for types.
/// Uses curiously-recurring template pattern for static dispatch of visit overloads
template<typename Self, typename T>
class TypeVisitor {
public:
    /// Updates a possible value from the visit and returns flag indicating 
    /// whether iteration should continue
    bool visit( const Type* t, T& r ) {
        if ( ! t ) return as<Self>(this)->visit( nullptr, r );

        auto tid = typeof(t);
        if ( tid == typeof<ConcType>() )
            return as<Self>(this)->visit( as<ConcType>(t), r );
        else if ( tid == typeof<NamedType>() )
            return as<Self>(this)->visit( as<NamedType>(t), r );
        else if ( tid == typeof<PolyType>() )
            return as<Self>(this)->visit( as<PolyType>(t), r );
        else if ( tid == typeof<VoidType>() )
            return as<Self>(this)->visit( as<VoidType>(t), r );
        else if ( tid == typeof<TupleType>() )
            return as<Self>(this)->visit( as<TupleType>(t), r );
        else if ( tid == typeof<FuncType>() )
            return as<Self>(this)->visit( as<FuncType>(t), r );

        unreachable("invalid Type type");
        return false;
    }

    /// Visits all children
    bool visitChildren( const NamedType* t, T& r ) {
        for ( const Type* tt : t->params() ) {
            if ( ! visit( tt, r ) ) return false;
        }
        return true;
    }

    /// Visits all children
    bool visitChildren( const TupleType* t, T& r ) {
        for ( const Type* tt : t->types() ) {
            if ( ! visit( tt, r ) ) return false;
        }
        return true;
    }

    /// Visits all children (parameters first)
    bool visitChildren( const FuncType* t, T& r ) {
        for ( const Type* tt : t->params() ) {
            if ( ! visit( tt, r ) ) return false;
        }
        if ( ! visit( t->returns(), r ) ) return false;
        return true;
    }

    /// Default implmentation of visit
    bool visit( std::nullptr_t, T& ) { return true; }
    
    /// Default implmentation of visit
    bool visit( const ConcType*, T& ) { return true; }

    /// Default implementation of visit
    bool visit( const NamedType* t, T& r ) { return visitChildren( t, r ); }
    
    /// Default implmentation of visit
    bool visit( const PolyType*, T& ) { return true; }
    
    /// Default implmentation of visit
    bool visit( const VoidType*, T& ) { return true; }

    /// Default implmentation of visit
    bool visit( const TupleType* t, T& r ) { return visitChildren( t, r ); }

    /// Default implmentation of visit
    bool visit( const FuncType* t, T& r ) { return visitChildren( t, r ); }

    /// Returns the result of visiting a default-constructed T
    T operator() ( const Type* t ) {
        T r{};
        visit( t, r );
        return r;
    }

    /// Visits the object provided by reference
    T& operator() ( const Type* t, T& r ) {
        visit( t, r );
        return r;
    }

    /// Visits the object provided by move
    T operator() ( const Type* t, T&& r ) {
        visit( t, r );
        return r;
    }
};
