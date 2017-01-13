#pragma once

#include <cstddef>

#include "data.h"
#include "type.h"

/// Visitor for types
template<typename T>
class TypeVisitor {
protected:
    /// Updates a possible value from the visit and returns flag indicating 
    /// whether iteration should continue
    bool visit( const Type*, T& );

    virtual bool visit( nullptr_t, T& ) { return true; }
    
    virtual bool visit( const ConcType*, T& ) { return true; }
    
    virtual bool visit( const PolyType*, T& ) { return true; }
    
    virtual bool visit( const VoidType*, T& ) { return true; }

    bool visitChildren( const TupleType* t, T& r ) {
        for ( const Type* tt : t->types() ) {
            if ( ! visit( tt, r ) ) return false;
        }
        return true;
    }
    virtual bool visit( const TupleType* t, T& r ) {
        assert(false);
        return visitChildren( t, r );
    }

public:
    T operator() ( const Type* t ) {
        T r;
        visit( t, r );
        return r;
    }

    T& operator() ( const Type* t, T& r ) {
        visit( t, r );
        return r;
    }

    T operator() ( const Type* t, T&& r ) {
        visit( t, r );
        return r;
    }
};

template<typename T>
bool TypeVisitor<T>::visit( const Type* t, T& r ) {
    if ( ! t ) return visit( nullptr, r );

    auto tid = typeof(t);
    if ( tid == typeof<ConcType>() ) return visit( as<ConcType>(t), r );
    else if ( tid == typeof<PolyType>() ) return visit( as<PolyType>(t), r );
    else if ( tid == typeof<VoidType>() ) return visit( as<VoidType>(t), r );
    else if ( tid == typeof<TupleType>() ) return visit( as<TupleType>(t), r );

    assert(false);
    return false;
}
