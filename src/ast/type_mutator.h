#pragma once

#include "mutator.h"
#include "type.h"
#include "type_visitor.h"

#include "data.h"
#include "option.h"

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
            switch ( newTypes->size() ) {  // de-tuple if needed
            case 0:
                r = new VoidType; break;
            case 1:
                r = newTypes->front(); break;
            default:
                r = new TupleType{ *move(newTypes) }; break;  // mutate return value
            }
        }
        return true;
    }

    /// Mutates the provided type pointer.
    const Type* operator() ( const Type* t ) {
        visit( t, t );
        return t;
    }

    /// Replaces the provided type pointer by a mutated copy, returning the copy for convenience.
    const Type* mutate( const Type*& t ) {
        visit( t, t );
        return t;
    }
};
