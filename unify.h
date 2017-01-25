#pragma once

#include <type_traits>

#include "binding.h"
#include "cost.h"
#include "cow.h"
#include "data.h"
#include "environment.h"
#include "type.h"

template<typename T>
using is_conc_or_named_type = typename std::enable_if< std::is_same<T, ConcType>::value 
                                                       || std::is_same<T, NamedType>::value >::type;

/// Tests if the first (parameter) type can be unified with the second (argument) type, 
/// while respecting the global type environment, and accumulating costs for the  
/// unification.
bool unify( const ConcType*, const NamedType*, Cost&, cow_ptr<Environment>& ) { return false; }

bool unify( const NamedType*, const ConcType*, Cost&, cow_ptr<Environment>& ) { return false; }

template<typename T, typename = is_conc_or_named_type<T>>
bool unify(const T*, const T*, Cost&, cow_ptr<Environment>&);

template<typename T, typename = is_conc_or_named_type<T>>
bool unify(const T*, const PolyType*, Cost&, cow_ptr<Environment>&);

template<typename T, typename = is_conc_or_named_type<T>>
bool unify(const T*, const Type*, Cost&, cow_ptr<Environment>&);

bool unify(const PolyType*, const Type*, Cost&, cow_ptr<Environment>&);

bool unify(const Type*, const Type*, Cost&, cow_ptr<Environment>&);

template<typename T, typename = is_conc_or_named_type<T>>
bool unify( const T* concParamType, const T* concArgType, 
            Cost&, cow_ptr<Environment>&) {
    return *concParamType == *concArgType;
}

template<typename T, typename = is_conc_or_named_type<T>>
bool unify(const T* concParamType, const PolyType* polyArgType, 
           Cost& cost, cow_ptr<Environment>& env) {
    // Attempts to make `concType` the representative for `polyType` in `env`; 
    // true iff no representative or `concType` unifies with the existing representative 
    const Type* boundArgType = find( env, polyArgType );
    if ( boundArgType ) return unify( concParamType, boundArgType, cost, env );
    
    bind( env, polyArgType, concParamType );
    
    ++cost.poly;  // poly-cost for global type variable binding
    return true;
}

template<typename T, typename = is_conc_or_named_type<T>>
bool unify(const T* concParamType, const Type* argType,
           Cost& cost, cow_ptr<Environment>& env) {
    auto aid = typeof(argType);
    if ( aid == typeof<ConcType>() ) 
        return unify( concParamType, as<ConcType>(argType), cost, env );
    else if ( aid == typeof<NamedType>() )
        return unify( concParamType, as<NamedType>(argType), cost, env );
    else if ( aid == typeof<PolyType>() )
        return unify( concParamType, as<PolyType>(argType), cost, env );
    else assert(false && "Unhandled argument type");

    return false; // unreachable
}

bool unify(const PolyType* polyParamType, const Type* argType,
           Cost& cost, cow_ptr<Environment>& env) {
    // Check if there is an existing binding for the parameter type
    const Type* boundParamType = find( env, polyParamType );
    if ( boundParamType ) return unify( boundParamType, argType, cost, env );

    auto aid = typeof(argType);
    if ( aid == typeof<ConcType>() || aid == typeof<NamedType>() ) {
        // make concrete type the new class representative
        bind( env, polyParamType, argType );
    } else if ( aid == typeof<PolyType>() ) {
        // add polymorphic type to type class
        bindClass( env, polyParamType, as<PolyType>(argType) );
    } else assert(false && "Unhandled argument type");
    
    ++cost.poly;  // poly-cost for global type variable binding
    return true;
}

bool unify(const Type* paramType, const Type* argType,
           Cost& cost, cow_ptr<Environment>& env) {
    auto pid = typeof(paramType);
    if ( pid == typeof<ConcType>() ) 
        return unify( as<ConcType>(paramType), argType, cost, env );
    else if ( pid == typeof<NamedType>() )
        return unify( as<NamedType>(paramType), argType, cost, env );
    else if ( pid == typeof<PolyType>() )
        return unify( as<PolyType>(paramType), argType, cost, env );
    else assert(false && "Unhandled parameter type");

    return false; // unreachable
}

/// Top-level unification call; polymorphic parameters here (and here only) are guaranteed 
/// to be local.
bool unify(const Type* paramType, const Type* argType,
           Cost& cost, TypeBinding& localEnv, cow_ptr<Environment>& env) {
    auto pid = typeof(paramType);
    if ( pid == typeof<ConcType>() ) {
        return unify( as<ConcType>(paramType), argType, cost, env );
    } else if ( pid == typeof<NamedType>() ) {
        return unify( as<NamedType>(paramType), argType, cost, env );
    } else if ( pid == typeof<PolyType>() ) {
        // top-level call, poly-params are still guaranteed to be local
        const PolyType* polyParamType = as<PolyType>(paramType);
        ++cost.poly;  // poly-cost for matching polymorphic parameter

        // Attempt to bind argument type to parameter type 
        const Type* boundType = localEnv.bind( polyParamType->name(), argType );
        if ( ! boundType ) {
            if ( const PolyType* polyArgType = as_safe<PolyType>(argType) ) {
                // add local poly type to non-local group
                bindClass( env, polyArgType, polyParamType->clone_bound( &localEnv ) );
            }
            return true;  // Unifies on successful binding.
        }
        // otherwise check bound type against argument
        return unify( boundType, argType, cost, env );
    } else assert(false && "Unhandled parameter type.");

    return false; //unreachable
}