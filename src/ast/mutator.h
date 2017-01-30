#pragma once

#include "data.h"
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
    for ( i = 0; i < n; ++i ) {
        const T* ti = last = in[i];
        if ( ! self->visit( ti, last ) ) return false;
        if ( last != ti ) goto modified;
    }
    return true;
    
    modified:
    List<T> ts;
    ts.reserve( n );
    for ( unsigned j = 0; j < i; ++j ) {
        ts.push_back( in[j] );
    }
    if ( last ) ts.push_back( last );
    for ( ++i; i < n; ++i ) {
        const T* ti = in[i];
        if ( ! self->visit( ti, ti ) ) return false;
        if ( ti ) ts.push_back( ti );
    }
    out = move(ts);
    return true;
}
