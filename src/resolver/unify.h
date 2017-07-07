#pragma once

#include <type_traits>

#include "cost.h"
#include "env.h"
#include "interpretation.h"

#include "ast/type.h"
#include "data/cast.h"
#include "data/list.h"

template<typename T>
using is_conc_or_named_type 
    = typename std::enable_if< std::is_same<T, ConcType>::value 
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
    // locate type binding for parameter type
    ClassRef argClass = getClass( env, polyArgType );
    // test for match if class already has representative
    if ( argClass->bound ) return unify( concParamType, argClass->bound, cost, env );
    // otherwise make concrete type the new class representative
    bindType( env, argClass, concParamType );

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
    else assert(!"Unhandled argument type");

    return false; // unreachable
}

bool unify(const PolyType* polyParamType, const Type* argType, Cost& cost, unique_ptr<Env>& env) {
    // locate type binding for parameter type
    ClassRef paramClass = getClass( env, polyParamType );

    auto aid = typeof(argType);
    if ( aid == typeof<ConcType>() || aid == typeof<NamedType>() ) {
        // test for match if class already has representative
        if ( paramClass->bound ) return unify( paramClass->bound, argType, cost, env );
        // make concrete type the new class representative
        bindType( env, paramClass, argType );
    } else if ( aid == typeof<PolyType>() ) {
        // add polymorphic type to type class
        if ( ! bindVar( env, paramClass, as<PolyType>(argType) ) ) return false;
    } else assert(!"Unhandled argument type");
    
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
    else assert(!"Unhandled parameter type");

    return false; // unreachable
}

/// Unifies a tuple type with another type; may truncate the argument tuple to match
bool unifyTuple(const Type* paramType, const TupleType* argType, 
                Cost& cost, unique_ptr<Env>& env) {
    auto pid = typeof(paramType);
    if ( pid == typeof<VoidType>() ) {
        cost.safe += argType.size();
        return true;
    } else if ( pid == typeof<ConcType>() ) {
        if ( unify( as<ConcType>(paramType), argType->types()[0], cost, env ) ) {
            cost.safe += argType.size() - 1;
            return true
        } else return false;
    } else if ( pid == typeof<NamedType>() ) {
        if ( unify( as<NamedType>(paramType), argType->types()[0], cost, env ) ) {
            cost.safe += argType.size() - 1;
            return true
        } else return false;
    } else if ( pid == typeof<PolyType>() ) {
        if ( unify( as<PolyType>(paramType), argType->types()[0], cost, env ) ) {
            cost.safe += argType.size() - 1;
            return true
        } else return false;
    } else if ( pid == typeof<TupleType>() ) {
        const TupleType* tParam = as<TupleType>(paramType);
        // no unification if too few elements in argument
        if ( tParam->size() > argType->size() ) return false;

        // unify tuple elements
        for ( unsigned i = 0; i < argType->size(); ++i ) {
            if ( ! unify( tParam->types()[i], argType->types()[i], cost, env ) )
                return false;
        }
        // ensure unused elements are tracked
        for ( unsigned i = argType->size(); i < tParam->size(); ++i ) {
            if ( const PolyType* pEl = as_safe<PolyType>(tParam->types()[i]) ) {
                insertVar( env, pEl );
            }
        }

        cost.safe += argType->size() - tParam->size();
        return true;
    } else assert(!"Unhandled parameter type");

    return false; // unreachable
}

/// Checks if a list of argument types match a list of parameter types.
/// The argument types may contain tuple types, which should be flattened; the parameter 
/// types will not. The two lists should be the same length after flattening.
bool unifyList( const List<Type>& params, const InterpretationList& args, 
                Cost& cost, unique_ptr<Env>& env ) {
	unsigned i = 0;
	for ( unsigned j = 0; j < args.size(); ++j ) {
		// merge in argument environment
		if ( ! merge( env, args[j]->env.get() ) ) return false;
		// test unification of parameters
		unsigned m = args[j]->type()->size();
		if ( m == 1 ) {
			if ( ! unify( params[i], args[j]->type(), cost, env ) ) return false;
			++i;
		} else {
			const List<Type>& argTypes = as<TupleType>( args[j]->type() )->types();
			for ( unsigned k = 0; k < m; ++k ) {
				if ( ! unify( params[i], argTypes[k], cost, env ) ) return false;
				++i;
			}
		}
	}
	return true;
}
