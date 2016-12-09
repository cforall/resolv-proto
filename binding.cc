#include <ostream>

#include "binding.h"

#include "decl.h"
#include "gc.h"
#include "type.h"

std::ostream& operator<< (std::ostream& out, const TypeBinding& tb) {
    if ( tb.empty() ) return out << "{}";

    out << "{";
    auto it = tb.bindings_.begin();
    while (true) {
        out << it->first << " => ";
        if ( it->second ) { out << *it->second; } else { out << "???"; }
        if ( ++it == tb.bindings_.end() ) break;
        out << ", ";
    }
    return out << "}";
}

const GC& operator<< (const GC& gc, const TypeBinding* tb) {
    if ( ! tb ) return gc;

    for ( auto it = tb->bindings_.begin(); it != tb->bindings_.end(); ++it ) {
        gc << it->second;
    }

    return gc;
}

TypeBinding* make_binding( const FuncDecl* func ) {
    if ( func->tyVars().empty() ) return nullptr;
    return new TypeBinding{ func->name(), func->tyVars().begin(), func->tyVars().end() };
}
