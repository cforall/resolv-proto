#pragma once

#include <cstddef>

#include "expr.h"

#include "data.h"

/// Visitor for typed expressions. 
template<typename Self, typename T>
class TypedExprVisitor {
public:
    /// Updates a possible value from the visit and returns bool indicating whether 
    /// iteration should continue.
    bool visit( const TypedExpr* e, T& r ) {
        if ( ! e ) return as<Self>(this)->visit( nullptr, r );

        auto tid = typeof(e);
        if ( tid == typeof<VarExpr>() )\
            return as<Self>(this)->visit( as<VarExpr>(e), r );
        else if ( tid == typeof<CastExpr>() ) 
            return as<Self>(this)->visit( as<CastExpr>(e), r );
        else if ( tid == typeof<CallExpr>() )
            return as<Self>(this)->visit( as<CallExpr>(e), r );
        else if ( tid == typeof<TupleElementExpr>() )
            return as<Self>(this)->visit( as<TupleElementExpr>(e), r );
        else if ( tid == typeof<TupleExpr>() )
            return as<Self>(this)->visit( as<TupleExpr>(e), r );
        else if ( tid == typeof<AmbiguousExpr>() )
            return as<Self>(this)->visit( as<AmbiguousExpr>(e), r );

        assert(false);
        return false;
    }

    bool visitChildren( const CastExpr* e, T& r ) {
        if ( const TypedExpr* arg = as_derived_safe<TypedExpr>( e->arg() ) ) {
            return visit( arg, r );
        } else return true;
    }

    bool visitChildren( const CallExpr* e, T& r ) {
        for ( const TypedExpr *arg : e->args() ) {
            if ( ! visit( arg, r ) ) return false;
        }
        return true;
    }

    bool visitChildren( const TupleElementExpr* e, T& r ) {
        // only visits source expression from first child
        if ( e->ind() == 0 ) return visit( e->of(), r );
        else return true;
    }

    bool visitChildren( const TupleExpr* e, T& r ) {
        for ( const TypedExpr* el : e->els() ) {
            if ( ! visit( el, r ) ) return false;
        }
        return true;
    }

    bool visitChildren( const AmbiguousExpr* e, T& r ) {
        for ( const TypedExpr* alt : e->alts() ) {
            if ( ! visit( alt, r ) ) return false;
        }
        return true;
    }

    bool visit( nullptr_t, T& ) { return true; }
    
    bool visit( const VarExpr*, T& ) { return true; }
    
    bool visit( const CastExpr* e, T& r ) { return visitChildren( e, r ); }
    
    bool visit( const CallExpr* e, T& r ) { return visitChildren( e, r ); }
    
    bool visit( const TupleElementExpr* e, T& r ) { return visitChildren( e, r ); }

    bool visit( const TupleExpr* e, T& r ) { return visitChildren( e, r ); }
    
    bool visit( const AmbiguousExpr* e, T& r ) { return visitChildren( e, r ); }

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
