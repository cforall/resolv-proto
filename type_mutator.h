#pragma once

#include "data.h"
#include "mutator.h"
#include "option.h"
#include "type.h"
#include "type_visitor.h"

/// Produces a mutated copy of a type, where any un-mutated subtypes are shared with the 
/// original.
template<typename Self>
class TypeMutator : public TypeVisitor<Self, const Type*> {
public:
    using TypeVisitor<Self, const Type*>::visit;

    bool visit( const TupleType* t, const Type*& r ) {
        option<List<Type>> newTypes;
        if ( ! mutateAll( this, t->types(), newTypes ) ) return false;
        if ( newTypes ) {
            r = new TupleType{ *move(newTypes) };  // mutate return value
        }
        return true;
    }

    /// Replaces the provided type pointer by a mutated copy, returning the copy for convenience.
    const Type* mutate( const Type*& t ) {
        visit( t, t );
        return t;
    }
};
