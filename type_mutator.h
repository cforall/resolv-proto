#pragma once

#include <cstddef>

#include "type.h"
#include "type_visitor.h"

/// Mutator for types; returns either another pointer to the argument, or a 
/// different object (based on subclass behaviour).
/*class TypeMutator {
public:
    virtual const Type* mutate( nullptr_t ) { return nullptr; }
    virtual const Type* mutate( const ConcType* t ) { return t; }
    virtual const Type* mutate( const PolyType* t ) { return t; }
    virtual const Type* mutate( const VoidType* t ) { return t; }
    virtual const Type* mutate( const TupleType* t ) {
        unsigned i;
        unsigned n = t->size();
        const Type* last = nullptr;
        for ( i = 0; i < n; ++i ) {
            const Type* ti = t->types()[i];
            last = mutate( ti );
            if ( last != ti ) goto modified;  // loop until find mutated subtype
        }
        return t;  // return original type if no mutations

        modified: List<Type> ts;  // new list
        ts.reserve( n );
        for ( unsigned j = 0; j < i; ++j ) {
            ts.push_back( t->types()[i] );  // copy un-mutated prefix in
        }
        ts.push_back( last );  // copy first mutated node in
        for ( ++i; i < n; ++i ) {
            ts.push_back( mutate( t->types()[i] ) );  // copy possibly-mutated suffix in
        }
        return new TupleType( move(ts) );  // return mutated tuple
    }

    const Type* mutate( const Type* t ) {  // not virtual; dispatch is the same for all subclasses
        if ( ! t ) return mutate(nullptr);

        auto tid = typeof(t);
        if ( tid == typeof<ConcType>() ) return mutate( as<ConcType>(t) );
        else if ( tid == typeof<PolyType>() ) return mutate( as<PolyType>(t) );
        else if ( tid == typeof<VoidType>() ) return mutate( as<VoidType>(t) );
        else if ( tid == typeof<TupleType>() ) return mutate( as<TupleType>(t) );

        assert(false);
        return nullptr;
    }
};*/

class TypeMutator : public TypeVisitor<const Type*> {
    Visit visit( const TupleType* t, const Type*& r ) override {
        unsigned i;
        unsigned n = t->size();
        const Type* last = nullptr;
        for ( i = 0; i < n; ++i ) {
            const Type* ti = last = t->types()[i];
            visitAll( ti, last );
            if ( last != ti ) goto modified;
        }
        return Visit::SKIP;  // done with this subtree

        modified:
        List<Type> ts;  // new list
        ts.reserve( n );
        for ( unsigned j = 0; j < i; ++j ) {
            ts.push_back( t->types()[j] );   // copy un-mutated prefix in
        }
        ts.push_back( last );  // copy first mutated node in
        for ( ++i; i < n; ++i ) {
            const Type* ti = t->types()[i];
            visitAll( ti, ti );
            ts.push_back( ti );  // copy possibly-mutated suffix in
        }
        r = new TupleType( move(ts) );  // mutate return value
        return Visit::SKIP;  // done with this subtree
    }
};
