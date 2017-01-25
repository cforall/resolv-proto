#pragma once

#include "data.h"
#include "gc.h"
#include "type.h"
#include "type_map.h"

typedef TypeMap< const Type* > CanonicalTypeMap;

/// Gets the canonical type of type T from a TypeMap
template<typename T, typename... Args>
inline const T* get_canon(CanonicalTypeMap& m, Args&&... args) {
    CanonicalTypeMap* mm = m.get_as<T>( args... );
    if ( mm ) {
        const Type **tt = mm->get();
        if ( tt ) {
            return as<T>(*tt);
        } else {
            const T* t = new T{ forward<Args>(args)... };
            mm->set( t );
            return t;
        }
    } else {
        const T* t = new T{ forward<Args>(args)... };
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
