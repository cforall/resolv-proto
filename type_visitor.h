#pragma once

#include <cstddef>

#include "data.h"
#include "type.h"

/// Visitor for types.
/// Uses curiously-recurring template pattern for static dispatch of visit overloads
template<typename Self, typename T>
class TypeVisitor {
protected:
    /// Updates a possible value from the visit and returns flag indicating 
    /// whether iteration should continue
    bool visit( const Type* t, T& r ) {
        if ( ! t ) return as<Self>(this)->visit( nullptr, r );

        auto tid = typeof(t);
        if ( tid == typeof<ConcType>() )
            return as<Self>(this)->visit( as<ConcType>(t), r );
        else if ( tid == typeof<PolyType>() )
            return as<Self>(this)->visit( as<PolyType>(t), r );
        else if ( tid == typeof<VoidType>() )
            return as<Self>(this)->visit( as<VoidType>(t), r );
        else if ( tid == typeof<TupleType>() )
            return as<Self>(this)->visit( as<TupleType>(t), r );

        assert(false);
        return false;
    }

    /// Visits all children
    bool visitChildren( const TupleType* t, T& r ) {
        for ( const Type* tt : t->types() ) {
            if ( ! visit( tt, r ) ) return false;
        }
        return true;
    }

public:
    /// Default implmentation of visit
    bool visit( nullptr_t, T& ) { return true; }
    
    /// Default implmentation of visit
    bool visit( const ConcType*, T& ) { return true; }
    
    /// Default implmentation of visit
    bool visit( const PolyType*, T& ) { return true; }
    
    /// Default implmentation of visit
    bool visit( const VoidType*, T& ) { return true; }

    /// Default implmentation of visit
    bool visit( const TupleType* t, T& r ) { return visitChildren( t, r ); }

    /// Returns the result of visiting a default-constructed T
    T operator() ( const Type* t ) {
        T r;
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
