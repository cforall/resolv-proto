#pragma once

#include <cassert>
#include <functional>
#include <utility>

#include "conversion.h"
#include "cost.h"
#include "env.h"
#include "interpretation.h"
#include "type_map.h"

#include "data/cast.h"
#include "data/compare.h"
#include "data/list.h"
#include "data/mem.h"
#include "data/range.h"
#include "lists/eager_merge.h"

/// Inserts a new interpretation into `expanded` with type `ty` if there is no 
/// interpretation already there that is cheaper
void setOrUpdateInterpretation( TypeMap<const Interpretation*>& expanded, const Type* ty, 
                                const Interpretation* i ) {
    auto existing = expanded.get( ty );
    if ( ! existing ) {
        // Type doesn't exist in map
        expanded.insert( ty, i );
    } else if ( const Interpretation** j = existing->get() ) {
        // Pre-existing interpretation ...
        switch ( compare(**j, *i) ) {
        case gt:  // ... with higher cost
            existing->set( ty, i );
            break;
        case eq:  // ... with equal cost
            existing->set( ty, Interpretation::merge_ambiguous( *j, i ) );
            break;
        case lt:  // ... ignore pre-existing interpretation with lower cost
            break;
        }
    } else {
        // No value for type exists in map
        existing->set( ty, i );
    }
}

/// If `j` is null or more expensive than `i`, replaces `j` with `i`.  
/// If the costs are equal,  will generate an ambiguous interpretation.
void setOrUpdateInterpretation( const Interpretation*& j, const Interpretation* i ) {
    if ( ! j ) {  // replace missing interpretation
        j = i;
    } else switch ( compare(*j, *i) ) {
    case gt:  // replace higher-cost interpretation
        j = i;
        break;
    case eq:  // merge ambiguous equal-cost interpretations
        j = Interpretation::merge_ambiguous( j, i );
        break;
    case lt:  // ignore new interpretation when existing has lower cost
        break;
    }
}

/// Replaces `results` with the conversion-expanded version
void expandConversions( InterpretationList& results, ConversionGraph& conversions ) {
    // Unique expanded interpretation for each type
    TypeMap<const Interpretation*> expanded;

    for ( const Interpretation* i : results ) {
        auto ty = i->expr->type();

        setOrUpdateInterpretation( expanded, ty, i );

        auto tid = typeof( ty );
        if ( typeof<ConcType>() == tid || typeof<NamedType>() == tid ) {
            for ( const Conversion& conv : conversions.find_from( ty ) ) {
                const Type* to = conv.to->type;

                setOrUpdateInterpretation( expanded, to, new Interpretation{ 
                    new CastExpr{ i->expr, &conv }, i->env, i->cost + conv.cost, i->argCost } );
            }
        } else if ( typeof<TupleType>() == tid ) {
            const TupleType* tty = as<TupleType>(ty);

            // list of conversions with default "self" non-conversion in each queue
            using ConversionQueues 
                = std::vector< defaulted_range<Conversion, ConversionGraph::const_iterator> >;
            ConversionQueues convs;
            convs.reserve( tty->size() );
            Conversion no_conv;  // special marker for self-conversion
            for ( unsigned j = 0; j < tty->size(); ++j ) {
                convs.emplace_back( no_conv, conversions.find_from( tty->types()[j] ) );
            }

            struct conversion_cost {
                const Cost& operator() ( const Conversion& c ) { return c.cost; }
            };

            bool first = true;
            for_each_cost_combo<Conversion, Cost, conversion_cost>(
                convs, 
                [tty,i,&first,&expanded]( 
                        const ConversionQueues& qs, const Indices& inds, const Cost& c ) {
                    // skip self iteration (no conversions on any queue)
                    if ( first ) { first = false; return; }
                    
                    List<Type> tys;  ///< underlying tuple types 
                    tys.reserve( qs.size() );
                    for ( unsigned j = 0; j < qs.size(); ++j ) {
                        unsigned k = inds[j];
                        const Conversion& conv = qs[j][k];

                        tys.push_back( k == 0 ? tty->types()[j] : conv.to->type );
                    }

                    // build tuple conversion and underlying types
                    List<TypedExpr> els;
                    els.reserve( qs.size() );
                    Cost toCost = i->cost;
                    for ( unsigned j = 0; j < qs.size(); ++j ) {
                        unsigned k = inds[j];
                        const Conversion& conv = qs[j][k];

                        auto *el = new TupleElementExpr( i->expr, j );
                        if ( k == 0 ) {
                            els.push_back( el );
                        } else {
                            els.push_back( new CastExpr{ el, &conv } );
                            toCost += conv.cost;
                        }
                    }
                    TupleExpr* e = new TupleExpr{ move(els) };

                    setOrUpdateInterpretation( expanded, e->type(), new Interpretation{
                        e, i->env, toCost, i->argCost } );
                } );
        }
    }

    // replace results with expanded results
    results.clear();
    for (auto it = expanded.begin(); it != expanded.end(); ++it) {
        results.push_back( it.get() );
    }
}

/// Attempts to bind a typeclass r to the concrete type conc in the current environment, 
/// returning true if successful
bool classBinds( ClassRef r, const Type* conc, Env*& env, Cost& cost ) {
    // test for match if class already has a representative.
    // TODO this is a restriction to exact polymorphic type binding, but loosening it 
    //      would be non-trivial; [ or maybe I just call convertTo here... ]
    if ( r->bound ) return *r->bound == *conc;
    // otherwise make concrete class the new typeclass representative
    bindType( env, r, conc );
    // ++cost.vars;
    return true;
}

/// Replaces expression with the best conversion to the given type, updating cost and env 
/// as needed. Returns nullptr for no such conversion; may update env and cost on such a 
/// failure result.
const TypedExpr* convertTo( const Type* ttype, const TypedExpr* expr,                
                            ConversionGraph& conversions, Env*& env, Cost& cost ) {
    const Type* etype = expr->type();
    auto eid = typeof( etype );
    auto tid = typeof( ttype );

    if ( eid == typeof<ConcType>() || eid == typeof<NamedType>() ) {
        if ( tid == typeof<ConcType>() || tid == typeof<NamedType>() ) {
            // check exact match
            if ( *etype == *ttype ) return expr;

            // check converted match
            if ( const Conversion* conv = conversions.find_between( etype, ttype ) ) {
                cost += conv->cost;
                return new CastExpr{ expr, conv };
            }

            // no matches
            return nullptr;
        } else if ( tid == typeof<PolyType>() ) {
            // test for match if target typeclass already has representative
            ClassRef tclass = getClass( env, as<PolyType>(ttype) );
            if ( classBinds( tclass, etype, env, cost ) ) {
                ++cost.poly;
                return expr;
            } else return nullptr;
        } else if ( tid == typeof<VoidType>() ) {
            // one safe conversion to truncate concrete type to Void
            ++cost.safe;
            return new TruncateExpr{ expr, ttype };
        } else if ( tid == typeof<TupleType>() ) {
            // can't match tuples to non-tuple types
            return nullptr;
        } else assert(!"Unhandled target type");
    } else if ( eid == typeof<PolyType>() ) {
        ClassRef eclass = getClass( env, as<PolyType>(etype) );

        if ( tid == typeof<ConcType>() || tid == typeof<NamedType>() ) {
            // attempt to bind polymorphic return type to concrete paramter
            if ( classBinds( eclass, ttype, env, cost ) ) {
                ++cost.poly;
                return expr;
            } else return nullptr;
        } else if ( tid == typeof<PolyType>() ) {
            // attempt to merge two typeclasses
            if ( bindVar( env, eclass, as<PolyType>(ttype) ) ) {
                ++cost.poly;
                return expr;
            } else return nullptr;
        } else if ( tid == typeof<VoidType>() || tid == typeof<TupleType>() ) {
            // neither tuples nor void can bind to PolyType vars.
            // TODO introduce ttype vars for this
            return nullptr;
        } else assert(!"Unhandled target type");
    } else if ( eid == typeof<VoidType>() ) {
        // fail for non-void targets
        return ( tid == typeof<VoidType>() ) ? expr : nullptr;
    } else if ( eid == typeof<TupleType>() ) {
        const TupleType* etuple = as<TupleType>(etype);
        unsigned en = etuple->size(), tn = ttype->size();

        // fail for target of greater arity
        if ( tn > en ) return nullptr;

        // target of lesser arity has safe cost of the number of elements trimmed
        switch (tn) {
        case 0: {
            // truncate to void
            cost.safe += en;
            return new TruncateExpr{ expr, ttype };
        } case 1: {
            // truncate to first element
            const TypedExpr* el = convertTo( ttype, new TupleElementExpr{ expr, 0 },
                                             conversions, env, cost );
            if ( ! el ) return nullptr;
            cost.safe += en - 1;
            return el;
        } default: {
            // make tuple out of the appropriate prefix
            const TupleType* ttuple = as<TupleType>(ttype);

            List<TypedExpr> els;
            els.reserve( ttuple->size() );
            for ( unsigned j = 0; j < tn; ++j ) {
                const TypedExpr* el = convertTo( ttuple->types()[j],
                                                 new TupleElementExpr{ expr, j },
                                                 conversions, env, cost );
                if ( ! el ) return nullptr;
                els.push_back( el );
            }
            cost.safe += en - tn;
            return new TupleExpr{ move(els) };
        }}
    } else assert(!"Unhandled expression type");

    return nullptr; // unreachable
}

/// Replaces `results` with the best interpretation (possibly conversion-expanded) as 
/// `targetType`.
/// If `targetType` is polymorphic, may result in multiple best interpretations.
InterpretationList convertTo( const Type* targetType, InterpretationList&& results, 
                              ConversionGraph& conversions ) {
    // best interpretation as targetType, null for none such
    TypeMap<const Interpretation*> best;

    for ( const Interpretation* i : results ) {
        const Type* ty = i->expr->type();
    
        if ( *ty == *targetType ) {
            // set interpretation if type matches
            setOrUpdateInterpretation( best, ty, i );
        } else {
            Cost cost = i->cost;
            Env* newEnv = Env::from( i->env );
            const TypedExpr* newExpr = 
                convertTo( targetType, i->expr, conversions, newEnv, cost );
            if ( newExpr ) {
                setOrUpdateInterpretation( best, newExpr->type(), new Interpretation{ 
                    newExpr, newEnv, move(cost), copy(i->argCost) } );
            }
        }
    }

    InterpretationList bests;
    for ( const auto& b : best ) { bests.push_back( b.second ); }
    return bests;
}
