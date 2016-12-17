#pragma once

#include <cstddef>

#include "data.h"
#include "expr.h"

/// Visitor for typed expressions. 
template<typename T>
class TypedExprVisitor {
public:
    /// Updates a possible value from the visit and returns bool indicating whether 
    /// iteration should continue.
    virtual bool visit( nullptr_t, T& ) { return true; }
    virtual bool visit( const VarExpr*, T& ) { return true; }
    virtual bool visit( const CastExpr*, T& ) { return true; }
    virtual bool visit( const CallExpr*, T& ) { return true; }
    virtual bool visit( const TupleElementExpr*, T& ) { return true; }
    virtual bool visit( const TupleExpr*, T& ) { return true; }
    virtual bool visit( const AmbiguousExpr*, T& ) { return true; }

protected:
    /// Visits an expression and all its typed subexpressions, returning bool indicating 
    /// whether iteration should continue
    bool visitAll( nullptr_t, T& r ) { return visit( nullptr, r ); }

    bool visitAll( const VarExpr* e, T& r ) { return visit( e, r ); }

    bool visitAll( const CastExpr *e, T& r ) {
        if ( ! visit( e, r ) ) return false;
        if ( const TypedExpr* arg = as_derived_safe<TypedExpr>( e->arg() ) ) {
            return visitAll( arg, r );
        } else return true;
    }

    bool visitAll( const CallExpr* e, T& r ) {
        if ( ! visit( e, r ) ) return false;
        for ( const TypedExpr *arg : e->args() ) {
            if ( ! visitAll( arg, r ) ) return false;
        }
        return true;
    }

    bool visitAll( const TupleElementExpr* e, T& r ) {
        if ( ! visit( e, r ) ) return false;
        if ( e->ind() == 0 ) return visitAll( e->of(), r );
        else return true;
    }

    bool visitAll( const TupleExpr* e, T& r ) {
        if ( ! visit( e, r ) ) return false;
        for ( const TypedExpr* el : e->els() ) {
            if ( ! visitAll( el, r ) ) return false;
        }
        return true;
    }

    bool visitAll( const AmbiguousExpr* e, T& r ) {
        if ( ! visit( e, r ) ) return false;
        for ( const TypedExpr* alt : e->alts() ) {
            if ( ! visitAll( alt, r ) ) return false;
        }
        return true;
    }

    bool visitAll( const TypedExpr* e, T& r ) {
        if ( ! e ) return visitAll( nullptr, r );

        auto tid = typeof(e);
        if ( tid == typeof<VarExpr>() ) { return visitAll( as<VarExpr>(e), r ); }
        else if ( tid == typeof<CastExpr>() ) { return visitAll( as<CastExpr>(e), r ); }
        else if ( tid == typeof<CallExpr>() ) { return visitAll( as<CallExpr>(e), r ); }
        else if ( tid == typeof<TupleElementExpr>() ) { return visitAll( as<TupleElementExpr>(e), r ); }
        else if ( tid == typeof<TupleExpr>() ) { return visitAll( as<TupleExpr>(e), r ); }
        else if ( tid == typeof<AmbiguousExpr>() ) { return visitAll( as<AmbiguousExpr>(e), r ); }

        assert(false);
        return false;
    }

public:
    /// Returns a (default-constructed) value from the visit
    T operator() ( const TypedExpr* e ) {
        T r;
        visitAll( e, r );
        return r;
    }

    /// Updates the given value by the visitor
    T& operator() ( const TypedExpr* e, T& r ) {
        visitAll( e, r );
        return r;
    }

    /// Visits, starting from a default value
    T operator() ( const TypedExpr* e, T&& r ) {
        visitAll( e, r );
        return r;
    }
};
