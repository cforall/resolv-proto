#pragma once

#include <functional>
#include <utility>

#include "conversion.h"
#include "cost.h"
#include "data.h"
#include "eager_merge.h"
#include "interpretation.h"
#include "type_map.h"

/// Inserts a new interpretation into `expanded` with type `ty` if there is no interpretation 
/// already there that is cheaper than c; the new interpretation will be generated by calling 
/// interpretation(), a zero-arg function
template<typename F>
void setOrUpdateInterpretation( TypeMap< const Interpretation* >& expanded, const Type* ty,  
                                const Cost& c, F interpretation ) {
    auto existing = expanded.get( ty );
    if ( ! existing ) {
        // Type doesn't exist in map
        expanded.insert( ty, interpretation() );
    } else if ( const Interpretation** j = existing->get() ) {
        // Pre-existing interpretation ...
        if ( c < (*j)->cost ) {
            /// ... with higher cost
            existing->set( interpretation() );
        } else if ( c == (*j)->cost ) {
            // ... with equal cost
            existing->set( Interpretation::make_ambiguous( ty, c ) );
        }   // ... ignore pre-existing interpretation with lower cost
    } else {
        // No value for type exists in map
        existing->set( interpretation() );
    }
}

/// Replaces `results` with the conversion-expanded version
void expandConversions( InterpretationList& results, ConversionGraph& conversions ) {
    // Unique expanded interpretation for each type
    TypeMap< const Interpretation* > expanded;

    for ( const Interpretation* i : results ) {
        auto ty = i->expr->type();

        setOrUpdateInterpretation( expanded, ty, i->cost, [i]() { return i; } );

#ifdef RP_USER_CONVS
        #error User conversion expansion not yet implemented
#else // ! RP_USER_CONVS
        auto tid = typeof( ty );
        if ( typeof<ConcType>() == tid )  {
            for ( const Conversion& conv : conversions.find( ty ) ) {
                const Type* to = conv.to->type;
                Cost toCost = i->cost + conv.cost;

                setOrUpdateInterpretation( expanded, to, toCost, 
                    [i,&conv,&toCost]() { return new Interpretation{ new CastExpr{ i->expr, &conv }, 
                                                                    copy(toCost) }; } );
            }
        } else if ( typeof<TupleType>() == tid ) {
            const TupleType* tty = as<TupleType>(ty);

            /// list of conversions with default "self" non-conversion in each queue
            typedef std::vector< defaulted_vector< Conversion, std::vector<Conversion> > >
                    ConversionQueues;  
            ConversionQueues convs;
            convs.reserve( ty->size() );
            Conversion no_conv;  // special marker for self-conversion
            for ( unsigned i = 0; i < ty->size(); ++i ) {
                convs.emplace_back( no_conv, conversions.find( ty ) );
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

                    Cost toCost = i->cost + c;

                    setOrUpdateInterpretation( expanded, new TupleType{ tys }, toCost, 
                        [tty,i,&qs,&inds,&toCost]() -> Interpretation* {
                            List<TypedExpr> els;
                            els.reserve( qs.size() );
                            for ( unsigned j = 0; j < qs.size(); ++j ) {
                                unsigned k = inds[j];
                                const Conversion& conv = qs[j][k];

                                auto *el = new TupleElementExpr( i->expr, j );

                                if ( k == 0 ) {
                                    els.push_back( el );
                                } else {
                                    els.push_back( new CastExpr{ el, &conv } );
                                }
                            }

                            return new Interpretation{ new TupleExpr( move(els) ), copy(toCost) };
                        } );
                } );
        }
#endif
    }

    // replace results with expanded results
    results.clear();
    for (auto it = expanded.begin(); it != expanded.end(); ++it) {
        results.push_back( it.get() );
    }
}
