#pragma once

#include <cstddef>

#include "data.h"
#include "type.h"
#include "visitor.h"

/// Visitor for types
template<typename T>
class TypeVisitor {
public:
    /// Updates a possible value from the visit and returns flag indicating 
    /// whether iteration should continue
    virtual Visit visit( nullptr_t, T& ) { return Visit::CONT; }
    virtual Visit visit( const ConcType*, T& ) { return Visit::CONT; }
    virtual Visit visit( const PolyType*, T& ) { return Visit::CONT; }
    virtual Visit visit( const VoidType*, T& ) { return Visit::CONT; }
    virtual Visit visit( const TupleType*, T& ) { return Visit::CONT; }

protected:
    /// Visits a type and all subtypes, returning bool indicating whether iteration 
    /// should continue.
    bool visitAll( nullptr_t, T& r ) { return visit( nullptr, r ); }

    bool visitAll( const ConcType* t, T& r ) { return visit( t, r ); }

    bool visitAll( const PolyType* t, T& r ) { return visit( t, r ); }

    bool visitAll( const VoidType* t, T& r ) { return visit( t, r ); }

    bool visitAll( const TupleType* t, T& r ) {
        auto flag = visit( t, r );
        if ( flag == Visit::DONE ) return false;
        if ( flag == Visit::CONT ) for ( const Type* tt : t->types() ) {
            if ( ! visitAll( tt, r ) ) return false;
        }
        return true;
    }

    bool visitAll( const Type* t, T& r ) {
        if ( ! t ) return visitAll( nullptr, r );

        auto tid = typeof(t);
        if ( tid == typeof<ConcType>() ) return visitAll( as<ConcType>(t), r );
        else if ( tid == typeof<PolyType>() ) return visitAll( as<PolyType>(t), r );
        else if ( tid == typeof<VoidType>() ) return visitAll( as<VoidType>(t), r );
        else if ( tid == typeof<TupleType>() ) return visitAll( as<TupleType>(t), r );

        assert(false);
        return false;
    }

public:
    T operator() ( const Type* t ) {
        T r;
        visitAll( t, r );
        return r;
    }

    T& operator() ( const Type* t, T& r ) {
        visitAll( t, r );
        return r;
    }

    T operator() ( const Type* t, T&& r ) {
        visitAll( t, r );
        return r;
    }
};
