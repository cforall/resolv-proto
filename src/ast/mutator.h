#pragma once

// Copyright (c) 2015 University of Waterloo
//
// The contents of this file are covered under the licence agreement in 
// the file "LICENCE" distributed with this repository.

#include "data/list.h"
#include "data/mem.h"
#include "data/option.h"

/// Mutates all of the input list using the mutation functions of self. 
/// If there are mutated elements and no short-circuit, will produce a 
/// new List<T> in out. Omits any null pointers from the list. 
/// Returns last short-circuit code.
template<typename Self, typename T>
bool mutateAll( Self* self, const List<T>& in, option<List<T>>& out ) {
    unsigned i;
    unsigned n = in.size();
    const T* last = nullptr;
    // scan for first modified element
    for ( i = 0; i < n; ++i ) {
        const T* ti = last = in[i];
        if ( ! self->visit( ti, last ) ) return false;
        if ( last != ti ) goto modified;
    }
    return true;  // no changes
    
    modified:
    List<T> ts;
    ts.reserve( n );
    // copy unmodified items into output
    for ( unsigned j = 0; j < i; ++j ) {
        ts.push_back( in[j] );
    }
    // copy modified items into output
    if ( last ) ts.push_back( last );
    for ( ++i; i < n; ++i ) {
        const T* ti = in[i];
        if ( ! self->visit( ti, ti ) ) return false;
        if ( ti ) ts.push_back( ti );
    }
    out = move(ts);
    return true;
}
