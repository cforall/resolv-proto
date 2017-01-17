#pragma once

#include "data.h"
#include "option.h"

/// Mutates all of ts0, a List<T>. If there are mutated elements, will 
/// produce a List<T> ts in the current scope with the complete list, 
/// otherwise will return with appropriate short-circuiting.
#define MUTATE_ALL( T, ts0 ) \
unsigned _i; \
unsigned _n = ts0.size(); \
const T* _last = nullptr; \
for ( _i = 0; _i < _n; ++_i ) { \
    const T* _ti = _last = ts0[_i]; \
    if ( ! visit( _ti, _last ) ) return false; \
    if ( _last != _ti ) goto modified; \
} \
return true; \
\
modified: \
List<T> ts; \
ts.reserve( _n ); \
for ( unsigned _j = 0; _j < _i; ++_j ) { \
    ts.push_back( ts0[_j] ); \
} \
ts.push_back( _last ); \
for ( ++_i; _i < _n; ++_i ) { \
    const Type* _ti = ts0[_i]; \
    if ( ! visit( _ti, _ti ) ) return false; \
    ts.push_back( _ti ); \
}
