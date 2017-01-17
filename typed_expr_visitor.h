#pragma once

#include <cstddef>

#include "data.h"
#include "expr.h"

/// Visitor for typed expressions. 
template<typename T>
class TypedExprVisitor {
protected:
    /// Updates a possible value from the visit and returns bool indicating whether 
    /// iteration should continue.
    bool visit( const TypedExpr*, T& );

    virtual bool visit( nullptr_t, T& ) { return true; }
    
    virtual bool visit( const VarExpr*, T& ) { return true; }
    
    bool visitChildren( const CastExpr* e, T& r ) {
        if ( const TypedExpr* arg = as_derived_safe<TypedExpr>( e->arg() ) ) {
            return visit( arg, r );
        } else return true;
    }
    virtual bool visit( const CastExpr* e, T& r ) { return visitChildren( e, r ); }
    
    bool visitChildren( const CallExpr* e, T& r ) {
        for ( const TypedExpr *arg : e->args() ) {
            if ( ! visit( arg, r ) ) return false;
        }
        return true;
    }
    virtual bool visit( const CallExpr* e, T& r ) { return visitChildren( e, r ); }
    
    bool visitChildren( const TupleElementExpr* e, T& r ) {
        // only visits source expression from first child
        if ( e->ind() == 0 ) return visit( e->of(), r );
        else return true;
    }
    virtual bool visit( const TupleElementExpr* e, T& r ) { return visitChildren( e, r ); }

    bool visitChildren( const TupleExpr* e, T& r ) {
        for ( const TypedExpr* el : e->els() ) {
            if ( ! visit( el, r ) ) return false;
        }
        return true;
    }
    virtual bool visit( const TupleExpr* e, T& r ) { return visitChildren( e, r ); }
    
    bool visitChildren( const AmbiguousExpr* e, T& r ) {
        for ( const TypedExpr* alt : e->alts() ) {
            if ( ! visit( alt, r ) ) return false;
        }
        return true;
    }
    virtual bool visit( const AmbiguousExpr* e, T& r ) { return visitChildren( e, r ); }

public:
    /// Returns a (default-constructed) value from the visit
    T operator() ( const TypedExpr* e ) {
        T r;
        visit( e, r );
        return r;
    }

    /// Updates the given value by the visitor
    T& operator() ( const TypedExpr* e, T& r ) {
        visit( e, r );
        return r;
    }

    /// Visits, starting from a default value
    T operator() ( const TypedExpr* e, T&& r ) {
        visit( e, r );
        return r;
    }
};

template<typename T>
bool TypedExprVisitor<T>::visit( const TypedExpr* e, T& r ) {
    if ( ! e ) return visit( nullptr, r );

    auto tid = typeof(e);
    if ( tid == typeof<VarExpr>() ) return visit( as<VarExpr>(e), r );
    else if ( tid == typeof<CastExpr>() ) return visit( as<CastExpr>(e), r );
    else if ( tid == typeof<CallExpr>() ) return visit( as<CallExpr>(e), r );
    else if ( tid == typeof<TupleElementExpr>() ) return visit( as<TupleElementExpr>(e), r );
    else if ( tid == typeof<TupleExpr>() ) return visit( as<TupleExpr>(e), r );
    else if ( tid == typeof<AmbiguousExpr>() ) return visit( as<AmbiguousExpr>(e), r );

    assert(false);
    return false;
}
