#pragma once

// Copyright (c) 2015 University of Waterloo
//
// The contents of this file are covered under the licence agreement in 
// the file "LICENCE" distributed with this repository.

#include "mutator.h"
#include "type.h"
#include "type_visitor.h"

#include "data/list.h"
#include "data/mem.h"
#include "data/option.h"

/// Produces a mutated copy of a type, where any un-mutated subtypes are shared with the 
/// original.
template<typename Self>
class TypeMutator : public TypeVisitor<Self, const Type*> {
public:
    using TypeVisitor<Self, const Type*>::visit;

    bool visit( const NamedType* t, const Type*& r ) {
        option<List<Type>> newTypes;
        if ( ! mutateAll( this, t->params(), newTypes ) ) return false;
        if ( newTypes ) {
            // rebuild with new parameters
            r = new NamedType{ t->name(), *move(newTypes) };
        }
        return true;
    }

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

    bool visit( const FuncType* t, const Type*& r ) {
        const Type* newReturns = t->returns();
        if ( ! visit( t->returns(), newReturns ) ) return false;
        option<List<Type>> newParams;
        if ( ! mutateAll( this, t->params(), newParams ) ) return false;
        if ( newReturns != t->returns() || newParams.has_value() ) {
            // rebuild with new types
            r = new FuncType{ *move(newParams), newReturns };
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
