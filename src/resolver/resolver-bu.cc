#include <algorithm>
#include <vector>

#include "resolver.h"

#if defined RP_DIR_BU
#include "arg_pack.h"
#endif
#include "cost.h"
#include "env.h"
#include "env_substitutor.h"
#include "expand_conversions.h"
#include "func_table.h"
#include "interpretation.h"
#include "resolve_assertions.h"
#include "unify.h"

#include "ast/expr.h"
#include "ast/forall.h"
#include "ast/forall_substitutor.h"
#include "ast/mutator.h"
#include "ast/typed_expr_visitor.h"
#include "ast/typed_expr_mutator.h"
#include "data/cast.h"
#include "data/collections.h"
#include "data/debug.h"
#include "data/list.h"
#include "data/mem.h"
#include "data/option.h"
#include "lists/eager_merge.h"

/// Return an interpretation for all zero-arg functions in funcs
template<typename Funcs>
InterpretationList matchFuncs( Resolver& resolver, const Funcs& funcs, const Env* outEnv, 
                               ResolverMode resolve_mode ) {
	InterpretationList results;
	
	// find functions with no parameters, skipping remaining checks if none
	auto withNParams = funcs.find( 0 );
	if ( withNParams == funcs.end() ) {
		DBG( "  with<0>params: []" );
		return results;
	}
	DBG( "  with<0>params: " << print_all_deref( withNParams() ) );
	
	// attempt to match functions to arguments
	for ( const FuncDecl* func : withNParams() ) {
		// skip functions returning no values, unless at top level
		if ( ! resolve_mode.allow_void && func->returns()->size() == 0 ) continue;

		const Env* env = outEnv;
		
		const TypedExpr* call = new CallExpr{ func, resolver.id_src };

		// check type assertions if necessary
		if ( RP_ASSN_CHECK( ! resolveAssertions( resolver, call, env ) ) ) continue;
		
		// no interpretation with zero arg cost
		results.push_back( new Interpretation{ call, env, func->poly_cost() } );
	}
	
	return results;
}

/// Return an interpretation for all single-argument interpretations in funcs
template<typename Funcs>
InterpretationList matchFuncs( Resolver& resolver, const Funcs& funcs, InterpretationList&& args, 
                               ResolverMode resolve_mode ) {
	InterpretationList results;

	for ( const Interpretation* arg : args ) {
		DBG( "  arg: " << *arg );

		// find function with appropriate number of parameters, skipping arg if none
		auto n = arg->type()->size();
		auto withNParams = funcs.find( n );
		if ( withNParams == funcs.end() ) {
			DBG( "    with<" << n << ">params: []" );
			continue;
		}
		DBG( "    with<" << n << ">params: " << print_all_deref( withNParams() ) );

		// attempt to match functions to arguments
		for ( const FuncDecl* func : withNParams() ) {
			// skip functions returning no values, unless at top level
			if ( ! resolve_mode.allow_void && func->returns()->size() == 0 ) continue;

			// Environment for call bindings
			Cost cost = func->poly_cost() + arg->cost; // cost for this call
			Env* env = Env::from( arg->env );
			unique_ptr<Forall> forall = Forall::from( func->forall(), resolver.id_src );
			List<Type> params = forall ? 
				ForallSubstitutor{ forall.get() }( func->params() ) : func->params();
			
			// skip functions that don't match the parameter types
			if ( n == 1 ) { // concrete type arg
				if ( ! unify( params[0], arg->type(), cost, env ) ) continue;
			} else {  // tuple type arg
				const List<Type>& argTypes = as<TupleType>( arg->type() )->types();
				for ( unsigned i = 0; i < n; ++i ) {
					if ( ! unify( params[i], argTypes[i], cost, env ) ) goto nextFunc;
				}
			}

			{ // extra scope so don't get goto errors for the labelled break simulation
				const TypedExpr* call = 
					new CallExpr{ func, List<TypedExpr>{ arg->expr }, move(forall) };

				// check type assertions if necessary
				if ( RP_ASSN_CHECK( ! resolveAssertions( resolver, call, env ) ) ) continue;
				
				// create new interpretation for resolved call
				results.push_back( new Interpretation{ call, env, cost, copy(arg->cost) } );
			}
		nextFunc:; }
	}

	return results;
}

#if defined RP_DIR_BU
/// Unifies prefix of argument type with parameter type (required to be size 1)
bool unifyFirst( const Type* paramType, const Type* argType, Cost& cost, Env*& env ) {
	auto aid = typeof(argType);
	if ( aid == typeof<VoidType>() ) {
		return false;  // paramType size == 1
	} else if ( aid == typeof<TupleType>() ) {
		return unify(paramType, as<TupleType>(argType)->types()[0], cost, env );
	} else {
		return unify(paramType, argType, cost, env);
	}
}

using ComboList = std::vector<InterpretationList>;

template<typename Funcs>
InterpretationList matchFuncs( Resolver& resolver, const Funcs& funcs, 
                               ComboList&& args, ResolverMode resolve_mode ) {
	InterpretationList results{};

	for ( const FuncDecl* func : funcs ) {
		// skip void-returning functions unless at top level
		if ( ! resolve_mode.allow_void && func->returns()->size() == 0 ) continue;

		// skip functions without enough parameters
		if ( func->params().size() < args.size() ) continue;

		// generate forall and parameter types for forall
		unique_ptr<Forall> rForall = Forall::from( func->forall(), resolver.id_src );
		const Type* rType;
		List<Type> rParams;
		if ( rForall ) {
			ForallSubstitutor m{ rForall.get() };
			rType = m( func->returns() );
			rParams = m( func->params() );
		} else {
			rType = func->returns();
			rParams = func->params();
		}

		Cost rCost = func->poly_cost();

		// iteratively build matches, one parameter at a time
		std::vector<ArgPack> combos{ ArgPack{ nullptr } };
		std::vector<ArgPack> nextCombos{};
		for ( const Type* param : rParams ) {
			assume(param->size() == 1, "parameter list should be flattened");
			// find interpretations with matching type
			for ( ArgPack& combo : combos ) {
				// test matches for leftover combo args
				if ( combo.on_last > 0 ) {
					const TupleType* cType = as_checked<TupleType>(combo.crnt->type());
					unsigned cInd = cType->size() - combo.on_last;
					const Type* crntType = cType->types()[ cInd ];
					Cost cCost = combo.cost;
					Env* cEnv = Env::from( combo.env );

					if ( unify( param, crntType, cCost, cEnv ) ) {
						// build new combo which consumes more arguments
						nextCombos.emplace_back( combo, move(cCost), cEnv, combo.on_last - 1 );
					}

					// replace combo with truncated expression
					combo.truncate();
				}

				// skip combos with no arguments left
				if ( combo.next == args.size() ) continue;

				// build new combos from matching interpretations
				for ( const Interpretation* i : args[combo.next] ) {
					Cost iCost = combo.cost + i->cost;
					Env* iEnv = Env::from( combo.env );

					// fail if cannot merge interpretation into combo
					if ( ! merge( iEnv, i->env ) ) continue;
					// fail if interpretation type does not match parameter
					if ( ! unifyFirst( param, i->type(), iCost, iEnv ) ) continue;

					// Build new ArgPack for this interpretation, noting leftovers
					nextCombos.emplace_back( combo, i, move(iCost), iEnv, i->type()->size() - 1 );
				}
			}

			// reset for next parameter
			combos.swap( nextCombos );
			nextCombos.clear();
			// exit early if no combinations
			if ( combos.empty() ) break;
		}

		// Validate matching combos, add to final result list
		for ( ArgPack& combo : combos ) {
			// skip combos with leftover arguments
			if ( combo.next != args.size() ) continue;

			// truncate any incomplete compbos
			combo.truncate();

			// make interpretation for this combo
			const TypedExpr* call = new CallExpr{ func, move(combo.args), copy(rForall), rType };
			const Env* cEnv = combo.env;
			
			// check type assertions if necessary
			if ( RP_ASSN_CHECK( ! resolveAssertions( resolver, call, cEnv ) ) ) continue;
			
			results.push_back( new Interpretation{ 
				call, cEnv, rCost + combo.cost, move(combo.argCost) } );
		}
	}

	return results;
}
#elif defined RP_DIR_CO
/// Create a list of argument expressions from a list of interpretations
List<TypedExpr> argsFrom( const InterpretationList& is ) {
	List<TypedExpr> args;
	for ( const Interpretation* i : is ) {
		args.push_back( i->expr );
	}
	return args;
}

/// List of argument combinations
using ComboList = std::vector< std::pair<Cost, InterpretationList> >;

/// Return an interpretation for all matches between functions in funcs and 
/// argument packs in combos
template<typename Funcs>
InterpretationList matchFuncs( Resolver& resolver, const Funcs& funcs, 
                               ComboList&& combos, ResolverMode resolve_mode ) {
	InterpretationList results;
	
	for ( const auto& combo : combos ) {
		DBG( "  combo: " << print_all_deref( combo.second ) );
		// find function with appropriate number of parameters, 
		// skipping combo if none
		auto n = 0;
		for ( const Interpretation* arg : combo.second ) {
			n += arg->type()->size();
		}
		auto withNParams = funcs.find( n );
		if ( withNParams == funcs.end() ) {
			DBG( "    with<" << n << ">Params: []" );
			continue;
		}
		DBG( "    with<" << n << ">Params: " << print_all_deref( withNParams() ) );
		
		// attempt to match functions to arguments
		for ( const FuncDecl* func : withNParams() ) {
			// skip functions returning no values unless at top level
			if ( ! resolve_mode.allow_void && func->returns()->size() == 0 ) continue;
			
			// Environment for call bindings
			Cost cost = func->poly_cost() + combo.first;
			Env* env = nullptr; // initialized by unifyList()
			unique_ptr<Forall> forall = Forall::from( func->forall(), resolver.id_src );
			List<Type> params = forall ? 
				ForallSubstitutor{ forall.get() }( func->params() ) : func->params();

			// skip functions that don't match the parameter types
			if ( ! unifyList( params, combo.second, cost, env ) ) continue;
			
			const TypedExpr* call = 
				new CallExpr{ func, argsFrom( combo.second ), move(forall) };

			// check type assertions if at top level
			if ( RP_ASSN_CHECK( ! resolveAssertions( resolver, call, env ) ) ) continue;
			
			// create new interpretation for resolved call
			results.push_back( new Interpretation{ call, env, move(cost), move(combo.first) } );
		}
	}
	
	return results;
}
#endif

/// Combination validator that fails on ambiguous interpretations in combinations
struct interpretation_unambiguous {
	bool operator() ( const std::vector< InterpretationList >& queues,
	                  const std::vector< unsigned >& inds ) {
		for( unsigned i = 0; i < queues.size(); ++i ) {
			if ( queues[ i ][ inds[i] ]->is_ambiguous() ) return false;
		}
		return true;
	}
};

InterpretationList Resolver::resolve( const Expr* expr, const Env* env, 
                                      ResolverMode resolve_mode ) {
	InterpretationList results;
	
	auto eid = typeof(expr);
	if ( eid == typeof<VarExpr>() ) {
		// do nothing for expressions which are already typed
		results.push_back( new Interpretation{ as<VarExpr>(expr), env } );
	} else if ( eid == typeof<FuncExpr>() ) {
		const FuncExpr* funcExpr = as<FuncExpr>( expr );

		// find candidates with this function name, skipping if none
		auto withName = funcs.find( funcExpr->name() );
		if ( withName == funcs.end() ) {
			DBG( "withName: []" );
			return results;
		}
		DBG( "withName: " << print_all_deref( withName() ) );
		
		// get interpretations for subexpressions (if any)
		switch( funcExpr->args().size() ) {
			case 0: {
				// environment is threaded to other matchFuncs variants from arguments
				results = matchFuncs( *this, withName(), env, resolve_mode );
			} break;
			case 1: {
				results = matchFuncs( *this, withName(), 
				                      resolve( funcExpr->args().front(), env ), 
									  resolve_mode );
			} break;
			default: {
				std::vector<InterpretationList> sub_results;
				sub_results.reserve( funcExpr->args().size() );
				for ( auto& sub : funcExpr->args() ) {
					// break early if no subexpression results
					InterpretationList sresults = resolve( sub, env );
					if ( sresults.empty() ) return results;
					
					sub_results.push_back( move(sresults) );
				}
				
#if defined RP_DIR_BU
				results = matchFuncs( *this, withName(), move(sub_results), resolve_mode );
#elif defined RP_DIR_CO
				interpretation_unambiguous valid;
				auto merged = unsorted_eager_merge<
					const Interpretation*, Cost, interpretation_cost>( sub_results, valid );
				
				results = matchFuncs( *this, withName(), move(merged), resolve_mode );
#endif
			} break;
		}
	} else unreachable("Unsupported expression type");
	
	if ( resolve_mode.expand_conversions ) {
		expandConversions( results, conversions );
	}
	
	return results;
}

InterpretationList Resolver::resolveWithType( const Expr* expr, const Type* targetType, 
	                                          const Env* env ) {
#if defined RP_RES_IMM
	auto resolve_mode = ResolverMode{}.without_conversions().with_void_as( targetType ).without_assertions();
#else
	auto resolve_mode = ResolverMode{}.without_conversions().with_void_as( targetType );
#endif
	return convertTo( targetType, resolve( expr, env, resolve_mode ), conversions );
}
