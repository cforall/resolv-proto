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

/// Produces a mutated copy of a typed expression, where any un-mutated subexpressions are 
/// shared with the original.
template<typename Self>
class TypedExprMutator : public TypedExprVisitor<Self, const TypedExpr*> {
protected:
    /// Cache for common sub-expressions
    std::unordered_map<const TypedExpr*, const TypedExpr*> memo;

public:
    using TypedExprVisitor<Self, const TypedExpr*>::visit;

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

    bool visit( const CallExpr* e, const TypedExpr*& r ) {
        option<List<TypedExpr>> newArgs;
        if ( ! mutateAll( this, e->args(), newArgs ) ) return false;
        if ( ! newArgs ) return true;
        if ( newArgs->size() < e->args().size() ) {  // trim expressions that lose any args
            r = nullptr;
            return true;
        }

        // instantiate new forall and substitute function type, if needed
        unique_ptr<Forall> forall = Forall::from( e->forall() );
        const FuncDecl* func = e->func();
        if ( forall ) {
            ForallSubstitutor m{ e->forall(), forall.get() };
            func = m(func);
            // TODO should newArgs be mutated according to the new forall?
            // I think no, as long as the polymorphic parameter types don't leak down to the args.
        }

        r = new CallExpr{ func, *move(newArgs), move(forall) };
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
        if ( newEls->size() < e->els().size() ) {  // trim expressions that lose any elements
            r = nullptr;
            return true;
        }
        r = new TupleExpr{ *move(newEls) }; // mutate return value
        return true;
    }

    bool visit( const AmbiguousExpr* e, const TypedExpr*& r ) {
        option<List<TypedExpr>> newAlts;
        if ( ! mutateAll( this, e->alts(), newAlts ) ) return false;
        if ( ! newAlts ) return true;
        switch ( newAlts->size() ) {  // trimmed alternatives may disambiguate expression
        case 0:
            r = nullptr; break;
        case 1:
            r = newAlts->front(); break;
        default:
            r = new AmbiguousExpr{ e->expr(), e->type(), *move(newAlts) }; break;
        }
        return true;
    }

    const TypedExpr* mutate( const TypedExpr*& e ) {
        (*this)( e, e );
        return e;
    }
};
