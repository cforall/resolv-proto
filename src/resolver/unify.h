#pragma once

// Copyright (c) 2015 University of Waterloo
//
// The contents of this file are covered under the licence agreement in 
// the file "LICENCE" distributed with this repository.

#include <type_traits>

#include "cost.h"
#include "env.h"
#include "interpretation.h"
#include "type_unifier.h"

#include "ast/type.h"
#include "data/cast.h"
#include "data/debug.h"
#include "data/list.h"

bool unify(const Type* paramType, const Type* argType, Cost& cost, Env& env) {
    Env newEnv = env;
    TypeUnifier unifier{ newEnv, cost.poly };
    if ( unifier( paramType, argType ) != nullptr ) {
        env = newEnv;
        return true;
    } else return false;
}

/// Unifies a tuple type with another type; may truncate the argument tuple to match
bool unifyTuple(const Type* paramType, const TupleType* argType, Cost& cost, Env& env) {
    auto pid = typeof(paramType);
    if ( pid == typeof<VoidType>() ) {
        cost.safe += argType->size();
        return true;
    } else if ( pid == typeof<ConcType>() ) {
        if ( unify( as<ConcType>(paramType), argType->types()[0], cost, env ) ) {
            cost.safe += argType->size() - 1;
            return true;
        } else return false;
    } else if ( pid == typeof<NamedType>() ) {
        if ( unify( as<NamedType>(paramType), argType->types()[0], cost, env ) ) {
            cost.safe += argType->size() - 1;
            return true;
        } else return false;
    } else if ( pid == typeof<PolyType>() ) {
        if ( unify( as<PolyType>(paramType), argType->types()[0], cost, env ) ) {
            cost.poly += 1;
            cost.safe += argType->size() - 1;
            return true;
        } else return false;
    } else if ( pid == typeof<FuncType>() ) {
        if ( unify( as<FuncType>(paramType), argType->types()[0], cost, env ) ) {
            cost.safe += argType->size() - 1;
            return true;
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
                env.insertVar( pEl );
                ++cost.poly;
            }
        }

        cost.safe += argType->size() - tParam->size();
        return true;
    } else unreachable("Unhandled parameter type");

    return false; // unreachable
}

/// Unifies a (possibly tuple) type with another type; mat truncate the argument type to match
bool unifyTuple(const Type* paramType, const Type* argType, Cost& cost, Env& env) {
    auto aid = typeof(argType);
    if ( aid == typeof<VoidType>() ) {
        return is<VoidType>(paramType);
    } else if ( aid == typeof<TupleType>() ) {
        return unifyTuple(paramType, as<TupleType>(argType), cost, env);
    } else {
        // single-element argument type
        auto pid = typeof(paramType);
        if ( pid == typeof<VoidType>() ) {
            // truncate to zero
            ++cost.safe;
            return true;
        } else if ( pid == typeof<TupleType>() ) {
            // can't extend to tuple
            return false;
        } else if ( pid == typeof<ConcType>() ) {
            return unify( as<ConcType>(paramType), argType, cost, env );
        } else if ( pid == typeof<NamedType>() ) {
            return unify( as<NamedType>(paramType), argType, cost, env );
        } else if ( pid == typeof<PolyType>() ) {
            return unify( as<PolyType>(paramType), argType, cost, env );
        } else if ( pid == typeof<FuncType>() ) {
            return unify( as<FuncType>(paramType), argType, cost, env );
        } else unreachable("Unhandled parameter type");

        return false; // unreachable
    }
}

/// Checks if a list of argument types match a list of parameter types.
/// The argument types may contain tuple types, which should be flattened; the parameter 
/// types will not. The two lists should be the same length after flattening.
bool unifyList( const List<Type>& params, const InterpretationList& args, Cost& cost, Env& env ) {
	unsigned i = 0;
	for ( unsigned j = 0; j < args.size(); ++j ) {
		// merge in argument environment
		if ( args[j]->env && ! env.merge( args[j]->env ) ) return false;
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
