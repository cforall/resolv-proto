#pragma once

#include <utility>

#include "conversion.h"
#include "cost.h"
#include "interpretation.h"
#include "type_map.h"

/// Replaces `results` with the conversion-expanded version
void expandConversions( InterpretationList& results, ConversionGraph& conversions ) {
    // Unique expanded interpretation for each type
    TypeMap< const Interpretation* > expanded;

    for ( const Interpretation* i : results ) {
        auto ty = i->expr->type();
        auto existing = expanded.get( ty );
        if ( ! existing ) {
            // Type doesn't exist in map
            expanded.insert( ty, i );
        } else if ( const Interpretation** j = existing->get() ) {
            // Pre-existing interpretation ...
            if ( i->cost < (*j)->cost ) {
                // ... with higher cost
                existing->set( i );
            } else if ( i->cost == (*j)->cost ) {
                // ... with equal cost
                existing->set( Interpretation::make_ambiguous( ty, i->cost ) );
            }   // ... ignore pre-existing with lower cost
        } else {
            // No value for type exists in map
            existing->set( i );
        }

#ifdef RP_USER_CONVS
        #error User conversion expansion not yet implemented
#else // ! RP_USER_CONVS
        // TODO fix me to not use canonicalized types
        for ( const Conversion& conv : conversions.find( ty ) ) {
            const Type* to = conv.to->type;
            Cost toCost = i->cost + conv.cost;

            auto existing = expanded.get( ty );
            if ( ! existing ) {
                expanded.insert( to, new Interpretation{ new CastExpr{ i->expr, &conv },
                                                         move(toCost) } );
            } else if ( const Interpretation** j = existing->get() ) {
                if ( i->cost < (*j)->cost ) {
                    existing->set( new Interpretation{ new CastExpr{ i->expr, &conv },
                                                       move(toCost) } );
                } else if ( i->cost == (*j)->cost ) {
                    existing->set( Interpretation::make_ambiguous( ty, move(toCost) ) );
                } // else ignore, existing interpretation is better
            } else {
                existing->set( new Interpretation{ new CastExpr{ i->expr, &conv },
                                                   move(toCost) } );
            }
        }
#endif

    }

    // replace results with expanded results
    results.clear();
    results.reserve( expanded.size() );
    for (auto it = expanded.begin(); it != expanded.end(); ++it) {
        results.push_back( it.get() );
    }
}
