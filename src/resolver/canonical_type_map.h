#pragma once

#include "ast/type.h"
#include "data/cast.h"
#include "data/gc.h"
#include "data/mem.h"
#include "data/list.h"
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
            mm->set( t, t );
            return t;
        }
    } else {
        const T* t = new T{ forward<Args>(args)... };
        m.insert( t, t );
        return t;
    }
}

/// Specialization for canonical types of generic named types
inline const NamedType* get_canon(CanonicalTypeMap& m, const std::string& n, List<Type>&& ps) {
    CanonicalTypeMap* mm = m.get_as<NamedType>( n );
    if ( mm ) {
        mm = mm->get( ps );
        if ( mm ) {
            const Type** tt = mm->get();
            if ( tt ) {
                return as<NamedType>(*tt);
            } else {
                const NamedType* t = new NamedType{ n, move(ps) };
                mm->set( t, t );
                return t;
            }
        }
    }

    const NamedType* t = new NamedType{ n, move(ps) };
    m.insert( t, t );
    return t;
}

inline const GC& operator<< (const GC& gc, const CanonicalTypeMap& m) {
    for ( auto it = m.cbegin(); it != m.cend(); ++it ) {
        gc << it.get();
    }
    return gc;
} 
