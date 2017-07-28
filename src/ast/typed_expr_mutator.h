#pragma once

#include <unordered_map>

#include "expr.h"
#include "forall.h"
#include "forall_substitutor.h"
#include "mutator.h"
#include "typed_expr_visitor.h"

#include "data/cast.h"
#include "data/list.h"
#include "data/mem.h"
#include "data/option.h"

/// Produces a mutated copy of a typed expression, where any un-mutated subexpressions 
/// are shared with the original.
template<typename Self>
class TypedExprMutator : public TypedExprVisitor<Self, const TypedExpr*> {
protected:
    /// Cache for common sub-expressions
    std::unordered_map<const TypedExpr*, const TypedExpr*> memo;

public:
    using TypedExprVisitor<Self, const TypedExpr*>::visit;
    using TypedExprVisitor<Self, const TypedExpr*>::operator();

    bool visit( const CastExpr* e, const TypedExpr*& r ) {
        if ( const TypedExpr* arg = as_derived_safe<TypedExpr>( e->arg() ) ) {
            const TypedExpr* newArg = arg;
            if ( ! visit( arg, newArg ) ) return false;
            if ( newArg != arg ) {
                r = newArg ? new CastExpr{ newArg, e->cast() } : nullptr;
            }
        }
        return true;
    }

    bool visit( const TruncateExpr* e, const TypedExpr*& r ) {
        const TypedExpr* newArg = e->arg();
        if ( ! visit( e->arg(), newArg ) ) return false;
        if ( newArg != e->arg() ) {
            r = newArg ? new TruncateExpr{ newArg, e->type() } : nullptr;
        }
        return true;
    }

    bool visit( const CallExpr* e, const TypedExpr*& r ) {
        option<List<TypedExpr>> newArgs;
        if ( ! mutateAll( this, e->args(), newArgs ) ) return false;
        if ( ! newArgs ) return true;
        if ( newArgs->size() < e->args().size() ) {  // trim expressions that lose args
            r = nullptr;
            return true;
        }

        r = new CallExpr{ e->func(), *move(newArgs), Forall::from( e->forall() ) };
        return true;
    }

    bool visit( const TupleElementExpr* e, const TypedExpr*& r ) {
        auto it = memo.find( e->of() );
        if ( it != memo.end() ) { // read existing conversion from memo table
            if ( it->second != e->of() ) {
                r = it->second ? new TupleElementExpr{ it->second, e->ind() } : nullptr;
            }
            return true;
        }

        const TypedExpr* newOf = e->of();
        if ( ! visit( e->of(), newOf ) ) return false;
        memo[ e->of() ] = newOf;
        if ( newOf != e->of() ) {
            r = newOf ? new TupleElementExpr{ newOf, e->ind() } : nullptr;
        }
        return true;
    }

    bool visit( const TupleExpr* e, const TypedExpr*& r ) {
        option<List<TypedExpr>> newEls;
        if ( ! mutateAll( this, e->els(), newEls ) ) return false;
        if ( ! newEls ) return true;
        if ( newEls->size() < e->els().size() ) {
            // trim expressions that lose any elements
            r = nullptr;
            return true;
        }
        r = new TupleExpr{ *move(newEls) }; // mutate return value
        return true;
    }

    bool visit( const AmbiguousExpr* e, const TypedExpr*& r ) {
        // code copied from mutator.h:mutateAll()

        unsigned i;
        unsigned n = e->alts().size();
        const TypedExpr* last = nullptr;
        // scan for first modified element
        for ( i = 0; i < n; ++i ) {
            const TypedExpr* ei = last = e->alts()[i]->expr;
            if ( ! visit( ei, last ) ) return false;
            if ( last != ei ) goto modified;
        }
        return true;  // no changes

        modified:
        List<Interpretation> alts;
        alts.reserve( n );
        // copy unmodified items into output
        for ( unsigned j = 0; j < i; ++j ) {
            alts.push_back( e->alts()[j] );
        }
        // copy modified items into output
        if ( last ) {
            const Interpretation* ii = e->alts()[i];
            alts.push_back( new Interpretation{ last, ii->cost, ii->env } );
        }
        for ( ++i; i < n; ++i ) {
            const Interpretation* ii = e->alts()[i];
            const TypedExpr* ei = ii->expr;
            if ( ! visit( ei, ei ) ) return false;
            if ( ei == ii->expr ) {
                alts.push_back( ii );
            } else if ( ei ) {
                alts.push_back( new Interpretation{ ei, ii->cost, ii->env } );
            }
        }

        // trimmed alternatives may disambiguate expression
        switch( alts.size() ) {
        case 0:  r = nullptr; break;
        case 1:  r = alts[0]->expr; break;
        default: r = new AmbiguousExpr{ e->expr(), e->type(), move(alts) }; break;
        }
        return true;
    }

    /// Mutates the provided type pointer.
    const TypedExpr* operator() ( const TypedExpr* e ) {
        visit( e, e );
        return e;
    }

    const TypedExpr* mutate( const TypedExpr*& e ) {
        (*this)( e, e );
        return e;
    }
};
