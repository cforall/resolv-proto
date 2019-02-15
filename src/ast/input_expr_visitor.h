#pragma once

#include <cstddef>

#include "expr.h"

#include "data/cast.h"
#include "data/debug.h"

/// Visitor for expressions in the input language. 
template<typename Self, typename T>
class InputExprVisitor {
public:
    /// Updates a possible value from the visit and returns bool indicating whether 
    /// iteration should continue.
    bool visit( const Expr* e, T& r ) {
        if ( ! e ) return as<Self>(this)->visit( nullptr, r );

        auto tid = typeof(e);
        if ( tid == typeof<ValExpr>() )
            return as<Self>(this)->visit( as<ValExpr>(e), r );
        else if ( tid == typeof<NameExpr>() )
            return as<Self>(this)->visit( as<NameExpr>(e), r);
        else if ( tid == typeof<FuncExpr>() ) 
            return as<Self>(this)->visit( as<FuncExpr>(e), r );
        
        unreachable("invalid input expression");
        return false;
    }

    bool visitChildren( const FuncExpr* e, T& r ) {
        for ( const Expr *arg : e->args() ) {
            if ( ! visit( arg, r ) ) return false;
        }
        return true;
    }

    bool visit( std::nullptr_t, T& ) { return true; }
    
    bool visit( const ValExpr*, T& ) { return true; }

    bool visit( const NameExpr*, T& ) { return true; }
    
    bool visit( const FuncExpr* e, T& r ) { return visitChildren( e, r ); }
    
    /// Returns a (default-constructed) value from the visit
    T operator() ( const Expr* e ) {
        T r{};
        visit( e, r );
        return r;
    }

    /// Updates the given value by the visitor
    T& operator() ( const Expr* e, T& r ) {
        visit( e, r );
        return r;
    }

    /// Visits, starting from a default value
    T operator() ( const Expr* e, T&& r ) {
        visit( e, r );
        return r;
    }
};
