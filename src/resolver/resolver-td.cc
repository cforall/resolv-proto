#include <cassert>

#include "resolver.h"

#include "env.h"
#include "expand_conversions.h"
#include "func_table.h"
#include "interpretation.h"
#include "resolve_assertions.h"

#include "ast/decl.h"
#include "ast/expr.h"
#include "ast/type.h"
#include "data/cast.h"

/// Resolves a function expression with any return type, binding the result to `bound` if 
/// a valid ref
template<typename Funcs>
InterpretationList resolveToAny( Resolver& resolver, const Funcs& funcs, 
                                 const FuncExpr* expr, const Env* env, 
                                 Resolver::Mode resolve_mode = ALL_NON_VOID, 
								 ClassRef bound = {} ) {
	InterpretationList results{};
	
	for ( const FuncDecl* func : funcs ) {
		// skip void-returning functions unless at top level
		if ( resolve_mode != Resolver::TOP_LEVEL && func->returns()->size() == 0 ) 
			continue;

		// skip functions without enough parameters
		if ( func->params().size() < expr->args().size() ) continue;

		// take zero-arg functions iff expr is zero-arg
		if ( func->params().empty() ^ expr->args().empty() ) continue;

		// generate forall and parameter/return types for forall
		unique_ptr<Forall> rForall = Forall::from( func->forall() );
		const Type* rType;
		List<Type> rParams;
		if ( rForall ) {
			ForallSubstitutor m{ func->forall(), rForall.get() };
			rType = m( func->returns() );
			rParams = m( func->params() );
		} else {
			rType = func->returns();
			rParams = func->params();
		}

		// make new environment for child resolution, binding function return type
		Cost rCost;
		unique_ptr<Env> rEnv = Env::from(env);
		if ( bound ) {
			// TODO account for tuple targets here
			if ( const PolyType* rPoly = as_safe<PolyType>(rType) ) {
				if ( ! bindVar( rEnv, bound, rPoly ) ) continue;
			} else {
				if ( ! bindType( rEnv, bound, rType ) ) continue;
			}
			++rCost.poly;
		}

		switch( expr->args().size() ) {
			case 0: {
				// no args, just add interpretation
				const TypedExpr* call = 
					new CallExpr{ func, List<TypedExpr>{}, move(rForall), rType };
				
				// check assertions if at top level
				if ( resolve_mode == TOP_LEVEL 
				     && ! resolveAssertions( resolver, call, rCost, rEnv ) ) 
					continue;
				
				results.push_back( new Interpretation{ call, move(rCost), move(rEnv) } );
			} break;
			case 1: {
				// single arg, accept and move up
				InterpretationList subs = resolver.resolveWithType( 
					expr->args().front(), Type::from( rParams ), rEnv.get() );
				for ( const Interpretation* sub : subs ) {
					// make interpretation for this arg
					const TypedExpr* call = 
						new CallExpr{ func, List<TypedExpr>{ sub->expr }, move(rForall), 
						              rType };
					Cost sCost = rCost + sub->cost;
					unique_ptr<Env> sEnv = flattenOut( sub->env.get(), rEnv.get() );
					
					// check assertions if at top level
					if ( resolve_mode == TOP_LEVEL 
					     && ! resolveAssertions( resolver, call, sCost, sEnv ) )
						continue;
					
					results.push_back(
						new Interpretation{ call, move(sCost), move(sEnv) } );
				}
			} break;
			default: {
				assert(!"TODO");
			} break;
		}
	}

	if ( resolve_mode == ALL_NON_VOID ) {
		expandConversions( results, resolver.conversions );
	}

	return results;
}

/// Resolves a function expression with a fixed return type, binding the result to 
/// `bound` if a valid ref
InterpretationList resolveTo( Resolver& resolver, const FuncSubTable& funcs, 
                              const FuncExpr* expr, const Type* targetType, 
							  const Env* env, ClassRef bound = {} ) {
	InterpretationList results;
	const auto& funcIndex = funcs.index();
	if ( const TypeMap<FuncList>* matches = funcIndex.get( target_type ) ) {
		// For any type (or tuple) that has the target type as a prefix
		for ( auto it = matches->begin(); it != matches->end(); ++it ) {
			// results for all the functions with that type
			InterpretationList sResults = resolveToAny( 
					resolver, it.get(), expr, env, Resolver::NO_CONVERSIONS, bound );
			if ( sResults.empty() ) continue;

			const Type* keyType = it.key();
			if ( keyType->size() > targetType->size() ) {
				// truncate result expressions down for results
				Cost sCost = Cost::from_safe( keyType->size() - targetType->size() );

				for ( const Interpretation* i : sResults ) {
					results.push_back( new Interpretation{ 
						new TruncateExpr{ i->expr, targetType }, 
						i->cost + sCost, 
						move(i->env) } );
				}
			} else {
				// just append sResults to overall results
				results.insert( results.end(), sResults.begin(), sResults.end() );
			}
		}
	}

	// TODO
	// * Loop through conversions to targetType, repeat above for those
	// * Ditto above for anything with a polymorphic return type, +(0,1,0) cost
	// * think about caching child resolutions across top-level parameters
	return results;
}

InterpretationList Resolver::resolve( const Expr* expr, const Env* env, 
                                      Resolver::Mode resolve_mode ) {
	auto eid = typeof(expr);
	if ( eid == typeof<VarExpr>() ) {
		return InterpretationList{ new Interpretation{ expr, Cost{}, Env::from(env) } };
	} else if ( eid == typeof<FuncExpr>() ) {
		const FuncExpr* funcExpr = as<FuncExpr>(expr);
		// find candidates with this function name, skipping if none
		auto withName = funcs.find( funcExpr->name() );
		if ( withName == funcs.end() ) return InterpretationList{};

		// return all candidates with this name
		return resolveToAny( *this, withName(), funcExpr, env, resolve_mode );
	} else assert(!"Unsupported expression type");

	return {};
}

InterpretationList Resolver::resolveWithType( const Expr* expr, const Type* targetType,                                                 const Env* env ) {
	auto eid = typeof(expr);
	if ( eid == typeof<VarExpr>() ) {
		// convert leaf expression to given type
		unique_ptr<Env> rEnv = Env::from( env );
		Cost rCost;
		const TypedExpr* rExpr = convertTo( targetType, expr, conversions, rEnv, rCost );
		// return expression if applicable
		return rExpr 
			? InterpretationList{ new Interpretation{ rExpr, move(rCost), move(rEnv) } } 
			: InterpretationList{};
	} else if ( eid == typeof<FuncExpr>() ) {
		const FuncExpr* funcExpr = as<FuncExpr>(expr);
		// find candidates with this function name, skipping if none
		auto withName = funcs.find( funcExpr->name() );
		if ( withName == funcs.end() ) return InterpretationList{};

		if ( const PolyType* pTarget = as_safe<PolyType>(targetType) ) {
			// polymorphic return; look for any option if unbound, bind to polytype
			unique_ptr<Env> rEnv = Env::from( env );
			ClassRef pClass = getClass( rEnv, pTarget );
			return ( pClass.bound )
				? resolveTo( *this, withName(), funcExpr, pClass.bound, rEnv.get(), 
						pClass );
				: resolveToAny( *this, withName(), funcExpr, rEnv.get(), 
						is<VoidType>(targetType) ? TOP_LEVEL : ALL_NON_VOID, pClass );
		} else {
			// monomorphic return; look for matching options
			return resolveTo( *this, withName(), funcExpr, targetType, env );
		}
	} else assert(!"Unsupported expression type");

	return {};
}