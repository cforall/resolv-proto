#include <ostream>

#include "binding.h"

#include "decl.h"
#include "gc.h"
#include "interpretation.h"
#include "type.h"

std::ostream& operator<< (std::ostream& out, const TypeBinding& tb) {
    if ( tb.empty() ) return out << "{}";

    out << "{";
    if ( ! tb.bindings_.empty() ) {
        auto it = tb.bindings_.begin();
        while (true) {
            out << it->first << " => ";
            if ( it->second ) { out << *it->second; } else { out << "???"; }
            if ( ++it == tb.bindings_.end() ) break;
            out << ", ";
        }
    }
    for ( auto asn : tb.assertions_ ) {
        out << " | " << *asn.first << " => ";
        if ( asn.second ) { out << *asn.second; } else { out << "???"; }
    }
    return out << "}";
}

const GC& operator<< (const GC& gc, const TypeBinding* tb) {
    if ( ! tb ) return gc;

    for ( auto it = tb->bindings_.begin(); it != tb->bindings_.end(); ++it ) {
        gc << it->second;
    }

    for ( auto jt = tb->assertions_.begin(); jt != tb->assertions_.end(); ++jt ) {
        gc << jt->first << jt->second;
    }

    return gc;
}
