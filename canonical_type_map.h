#pragma once

#include "gc.h"
#include "type.h"
#include "type_map.h"

typedef TypeMap< ConcType* > CanonicalTypeMap;

template<typename... Args>
inline const ConcType* get_canon(CanonicalTypeMap& m, Args&&... args) {
    ConcType* t = new ConcType( std::forward<Args>(args)... );
    CanonicalTypeMap* mm = m.get( t );
    if ( mm ) {
        ConcType **tt = mm->get();
        if ( tt ) {
            t = nullptr; // Clear t for GC
            return *tt;
        } else {
            mm->set( t );
            return t;
        }
    } else {
        m.insert( t, t );
        return t;
    }
}

inline const GC& operator<< (const GC& gc, const CanonicalTypeMap& m) {
    for ( auto it = m.cbegin(); it != m.cend(); ++it ) {
        gc << it.get();
    }
    return gc;
} 
