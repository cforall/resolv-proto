#pragma once

#include <type_traits>

#include "cost.h"
#include "env.h"

#include "ast/type.h"
#include "data/cast.h"

template<typename T>
using is_conc_or_named_type = typename std::enable_if< std::is_same<T, ConcType>::value 
                                                       || std::is_same<T, NamedType>::value >::type;

/// Tests if the first (parameter) type can be unified with the second (argument) type, 
/// while respecting the global type environment, and accumulating costs for the  
/// unification.
bool unify( const ConcType*, const NamedType*, Cost&, unique_ptr<Env>& ) { return false; }

bool unify( const NamedType*, const ConcType*, Cost&, unique_ptr<Env>& ) { return false; }

template<typename T, typename = is_conc_or_named_type<T>>
bool unify(const T*, const T*, Cost&, unique_ptr<Env>&);

template<typename T, typename = is_conc_or_named_type<T>>
bool unify(const T*, const PolyType*, Cost&, unique_ptr<Env>&);

template<typename T, typename = is_conc_or_named_type<T>>
bool unify(const T*, const Type*, Cost&, unique_ptr<Env>&);

bool unify(const PolyType*, const Type*, Cost&, unique_ptr<Env>&);

bool unify(const Type*, const Type*, Cost&, unique_ptr<Env>&);

template<typename T, typename>
bool unify( const T* concParamType, const T* concArgType, Cost&, unique_ptr<Env>&) {
    return *concParamType == *concArgType;
}

template<typename T, typename>
bool unify(const T* concParamType, const PolyType* polyArgType, Cost& cost, unique_ptr<Env>& env) {
    // Attempts to make `concType` the representative for `polyType` in `env`; 
    // true iff no representative or `concType` unifies with the existing representative 
    const Type* boundArgType = find( env, polyArgType );
    if ( boundArgType ) return unify( concParamType, boundArgType, cost, env );
    
    bind( env, polyArgType, concParamType );
    
    ++cost.poly;  // poly-cost for global type variable binding
    return true;
}

template<typename T, typename>
bool unify(const T* concParamType, const Type* argType, Cost& cost, unique_ptr<Env>& env) {
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

bool unify(const PolyType* polyParamType, const Type* argType, Cost& cost, unique_ptr<Env>& env) {
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

bool unify(const Type* paramType, const Type* argType, Cost& cost, unique_ptr<Env>& env) {
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
