#pragma once

#include "type.h"
#include "type_mutator.h"

/// Replaces polymorphic types by their local-environment binding
class ReplaceLocal : public TypeMutator<ReplaceLocal> {
public:
    using TypeMutator<ReplaceLocal>::visit;

    bool visit( const PolyType* t, const Type*& r ) {
        if ( t->src() ) {
            const Type* sub = (*t->src())[ t->name() ];
            if ( sub ) r = sub;
        }
        return true;
    }

    bool visit( const TupleType* t, const Type*& r ) {
        option<List<Type>> newTypes;
        mutateAll( this, t->types(), newTypes );
        if ( newTypes ) {
            r = new TupleType{ *move(newTypes) };
        }
        return true;
    }
};
