#pragma once

#include "binding.h"
#include "data.h"
#include "type.h"

/// Tests if the parameter type can be unified with the argument type while respecting 
/// the current type binding `env`. paramType and argType should have size 1.
bool unify(const Type* paramType, const Type* argType, TypeBinding& env) {
    auto pid = typeof(paramType);
    if ( pid == typeof<ConcType>() ) {
        auto concParamType = as<ConcType>(paramType);
        
        if ( auto concArgType = as_safe<ConcType>(argType) ) {
            // Check exact match of concrete param <=> concrete arg 
            return *concParamType == *concArgType;
        } else assert(false && "Unhandled argument type");

    } else if ( pid == typeof<PolyType>() ) {
        auto polyParamType = as<PolyType>(paramType);

        // Attempt to bind argument type to parameter type 
        const Type* boundType = env.bind( polyParamType->name(), argType );
        if ( ! boundType ) return true;  // Unifies on sucessful binding.
        // Check existing binding <=> arg type
        return unify(boundType, argType, env);

    } else assert(false && "Unhandled parameter type.");

    return false; //unreachable
}