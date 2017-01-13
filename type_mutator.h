#pragma once

#include <cstddef>

#include "type.h"
#include "type_visitor.h"

class TypeMutator : public TypeVisitor<const Type*> {
    using TypeVisitor<const Type*>::visit;
    
    bool visit( const TupleType* t, const Type*& r ) override {
        unsigned i;
        unsigned n = t->size();
        const Type* last = nullptr;
        for ( i = 0; i < n; ++i ) {
            const Type* ti = last = t->types()[i];
            if ( ! visit( ti, last ) ) return false;
            if ( last != ti ) goto modified;
        }
        return true;

        modified:
        List<Type> ts;  // new list
        ts.reserve( n );
        for ( unsigned j = 0; j < i; ++j ) {
            ts.push_back( t->types()[j] );   // copy un-mutated prefix in
        }
        ts.push_back( last );  // copy first mutated node in
        for ( ++i; i < n; ++i ) {
            const Type* ti = t->types()[i];
            if ( ! visit( ti, ti ) ) return false;
            ts.push_back( ti );  // copy possibly-mutated suffix in
        }
        r = new TupleType( move(ts) );  // mutate return value
        return true;
    }

public:
    const Type* mutate( const Type*& t ) {
        (*this)( t, t );
        return t;
    }
};
