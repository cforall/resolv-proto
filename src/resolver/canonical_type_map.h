#pragma once

// Copyright (c) 2015 University of Waterloo
//
// The contents of this file are covered under the licence agreement in 
// the file "LICENCE" distributed with this repository.

#include <utility>

#include "ast/type.h"
#include "data/cast.h"
#include "data/gc.h"
#include "data/mem.h"
#include "data/list.h"
#include "type_map.h"

typedef TypeMap< const Type* > CanonicalTypeMap;

/// Gets the canonical type of type T from a TypeMap; returns true if inserted an element
template<typename T, typename... Args>
inline std::pair<const T*, bool> get_canon(CanonicalTypeMap& m, Args&&... args) {
    CanonicalTypeMap* mm = m.get_as<T>( args... );
    if ( mm ) {
        const Type **tt = mm->get();
        if ( tt ) {
            return { as<T>(*tt), false };
        } else {
            const T* t = new T{ forward<Args>(args)... };
            mm->set( t, t );
            return { t, true };
        }
    } else {
        const T* t = new T{ forward<Args>(args)... };
        m.insert( t, t );
        return { t, true };
    }
}

/// Specialization for canonical types of generic named types
inline std::pair<const NamedType*, bool> get_canon(CanonicalTypeMap& m, const std::string& n, List<Type>&& ps) {
    CanonicalTypeMap* mm = m.get_as<NamedType>( n, ps.size() );
    if ( mm ) {
        mm = mm->get( ps );
        if ( mm ) {
            const Type** tt = mm->get();
            if ( tt ) {
                return { as<NamedType>(*tt), false };
            } else {
                const NamedType* t = new NamedType{ n, move(ps) };
                mm->set( t, t );
                return { t, true };
            }
        }
    }

    const NamedType* t = new NamedType{ n, move(ps) };
    m.insert( t, t );
    return { t, true };
}

inline const GC& operator<< (const GC& gc, const CanonicalTypeMap& m) {
    for ( auto it = m.cbegin(); it != m.cend(); ++it ) {
        gc << it.get();
    }
    return gc;
} 
