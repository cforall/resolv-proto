#include <cassert>
#include <functional>
#include <vector>
#include <unordered_map>
#include <utility>

#include "resolver.h"

#include "cost.h"
#include "env.h"
#include "expand_conversions.h"
#include "func_table.h"
#include "interpretation.h"
#include "resolve_assertions.h"
#include "unify.h"

#include "ast/decl.h"
#include "ast/expr.h"
#include "ast/type.h"
#include "data/cast.h"
#include "data/range.h"

/// State to iteratively build a top-down match of expressions to arguments
struct ArgPack {
	Env* env;                         ///< Current environment
	Cost cost;                        ///< Current cost
	Cost argCost;                     ///< Current argument-only cost
	List<TypedExpr> args;             ///< List of current arguments
	const TypedExpr* crnt;            ///< Current expression being built (nullptr for on_last == 0)
	unsigned on_last;                 ///< Number of unpaired type atoms on final arg
	List<Expr>::const_iterator next;  ///< Next argument expression

	ArgPack() = default;
	
	/// Initialize ArgPack with first argument iterator and initial environment
	ArgPack(const List<Expr>::const_iterator& it, const Env* e) 
		: env( Env::from(e) ), cost(), argCost(), args(), crnt(nullptr), on_last(0), next(it) {}
	
	/// Update ArgPack with new interpretation for next arg
	ArgPack(const ArgPack& old, const Interpretation* i)
		: env( Env::from(i->env) ), cost(old.cost + i->cost), argCost(old.argCost + i->cost), 
		  args(old.args), crnt(nullptr), on_last(0), next(old.next) {
		if ( old.crnt ) { args.push_back(old.crnt); }
		args.push_back(i->expr);
		++next;
	}
};

/// key type for argument cache
using ArgKey = std::pair<const Expr*, const Env*>;

namespace std {
	template<> struct hash<ArgKey> {
		typedef ArgKey argument_type;
		typedef std::size_t result_type;
		result_type operator() (const argument_type& k) const {
			return hash<const Expr*>{}(k.first) ^ hash<const Env*>{}(k.second);
		}
	};
}

/// Resolves a function expression with any return type, binding the result to `bound` if 
/// a valid ref
template<typename Funcs>
InterpretationList resolveToAny( Resolver& resolver, const Funcs& funcs, 
                                 const FuncExpr* expr, const Env* env, 
                                 Resolver::Mode resolve_mode = Resolver::ALL_NON_VOID, 
								 const PolyType* boundType = nullptr ) {
	InterpretationList results{};
	
	for ( const FuncDecl* func : funcs ) {
		// skip void-returning functions unless at top level
		if ( resolve_mode != Resolver::TOP_LEVEL && func->returns()->size() == 0 ) 
			continue;

		// skip functions without enough parameters
		if ( func->params().size() < expr->args().size() ) continue;

		// take zero-param functions iff expr is zero-arg
		if ( func->params().empty() != expr->args().empty() ) continue;

		// generate forall and parameter/return types for forall
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

		// make new environment for child resolution, binding function return type
		Cost rCost = func->poly_cost();
		Env* rEnv = Env::from(env);
		if ( boundType ) {
			if ( ! unify( boundType, rType, rCost, rEnv ) ) continue;
		}

		switch( expr->args().size() ) {
			case 0: {
				// no args, just add interpretation
				const TypedExpr* call = 
					new CallExpr{ func, List<TypedExpr>{}, move(rForall), rType };
				
				// check assertions if at top level
				if ( resolve_mode == Resolver::TOP_LEVEL 
				     && ! resolveAssertions( resolver, call, rCost, rEnv ) ) 
					continue;
				
				results.push_back( new Interpretation{ call, rEnv, move(rCost) } );
			} break;
			case 1: {
				// single arg, accept and move up
				InterpretationList subs = resolver.resolveWithType( 
					expr->args().front(), Type::from( rParams ), rEnv );
				for ( const Interpretation* sub : subs ) {
					// make interpretation for this arg
					const TypedExpr* call = 
						new CallExpr{ func, List<TypedExpr>{ sub->expr }, copy(rForall), rType };
					Cost sCost = rCost + sub->cost;
					Env* sEnv = Env::from( sub->env );
					
					// check assertions if at top level
					if ( resolve_mode == Resolver::TOP_LEVEL 
					     && ! resolveAssertions( resolver, call, sCost, sEnv ) )
						continue;
					
					results.push_back(
						new Interpretation{ call, sEnv, move(sCost), move(sub->cost) } );
				}
			} break;
			default: {
				// multiple args; proceed through matches
				std::vector<ArgPack> combos{ ArgPack{ expr->args().begin(), rEnv } };
				std::vector<ArgPack> nextCombos{};

				// Build match combos, one parameter at a time
				for ( const Type* param : rParams ) {
					// setup cache for parameters
					std::unordered_map<ArgKey, InterpretationList> argCache{};
					// find interpretations with this type
					for ( ArgPack& combo : combos ) {
						if ( combo.on_last > 0 ) {
							// TODO: test matches for leftover combo args
						}
						// skip combos with no arguments left
						if ( combo.next == expr->args().end() ) continue;
						// Find interpretation for this argument
						ArgKey key{ *combo.next, getNonempty( combo.env ) };
						auto cached = argCache.find( key );
						if ( cached == argCache.end() ) {
							// TODO: do this, but without truncation
							cached = argCache.emplace_hint( cached, move(key), 
									resolver.resolveWithType(
										*combo.next, param, getNonempty( combo.env ) ) );
						}
						InterpretationList subs = cached->second;
						// build new combos from interpretations
						for ( const Interpretation* i : subs ) {
							// TODO: handle multi-parameter args
							// Build new ArgPack for this interpretation
							nextCombos.emplace_back( combo, i );
						}
					}
					
					// reset for next parameter
					combos.swap( nextCombos );
					nextCombos.clear();
					// exit early if no combinations
					if ( combos.empty() ) break;
				}

				// Validate matching combos, adding to final result list
				for ( ArgPack& combo : combos ) {
					// skip combos with leftover arguments
					if ( combo.next != expr->args().end() ) continue;

					// make interpretation for this combo
					const TypedExpr* call = 
						new CallExpr{ func, move(combo.args), copy(rForall), rType };
					Cost cCost = rCost + combo.cost;
					
					// check assertions if at top level
					if ( resolve_mode == Resolver::TOP_LEVEL
					     && ! resolveAssertions( resolver, call, cCost, combo.env ) )
						continue;
					
					results.push_back(
						new Interpretation{ call, combo.env, move(cCost), move(combo.cost) } );
				}
			} break;
		}
	}

	if ( resolve_mode == Resolver::ALL_NON_VOID ) {
		expandConversions( results, resolver.conversions );
	}

	return results;
}

/// Resolves a function expression with a fixed return type, binding the result to 
/// `bound` if a valid ref
InterpretationList resolveTo( Resolver& resolver, const FuncSubTable& funcs, 
                              const FuncExpr* expr, const Type* targetType, 
							  const Env* env, const PolyType* boundType = nullptr ) {
	InterpretationList results;
	const auto& funcIndex = funcs.index();
	
	// For any type (or tuple that has the target type as a prefix)
	if ( const TypeMap<FuncList>* matches = funcIndex.get( targetType ) ) {
		for ( auto it = matches->begin(); it != matches->end(); ++it ) {
			// results for all the functions with that type
			InterpretationList sResults = resolveToAny( 
					resolver, it.get(), expr, env, Resolver::NO_CONVERSIONS, boundType );
			if ( sResults.empty() ) continue;

			const Type* keyType = it.key();
			bool trunc = keyType->size() > targetType->size();
			Cost sCost = Cost::zero();
			if ( trunc ) { sCost += Cost::from_safe( keyType->size() - targetType->size() ); }
			for ( const Interpretation* i : sResults ) {
				const TypedExpr* sExpr = trunc ? new TruncateExpr{ i->expr, targetType } : i->expr;
				results.push_back( new Interpretation{ sExpr, i->env, i->cost + sCost, i->cost } );
			}
		}
	}

	// Loop through conversions to targetType, repeating above
	for ( const Conversion& conv : resolver.conversions.find_to( targetType ) ) {
		// for any type (or tuple that has the conversion type as a prefix)
		if ( const TypeMap<FuncList>* matches = funcIndex.get( conv.from->type ) ) {
			for ( auto it = matches->begin(); it != matches->end(); ++it ) {
				// results for all the functions with that type
				InterpretationList sResults = resolveToAny(
					resolver, it.get(), expr, env, Resolver::NO_CONVERSIONS, boundType );
				if ( sResults.empty() ) continue;

				// cast and perhaps truncate expressions to match result type
				const Type* keyType = it.key();
				bool trunc = keyType->size() > conv.from->type->size();
				Cost sCost = conv.cost;
				if ( trunc ) { sCost += Cost::from_safe( keyType->size() - targetType->size() ); }
				for ( const Interpretation* i : sResults ) {
					const TypedExpr* sExpr = new CastExpr{ i->expr, &conv };
					if ( trunc ) {
						sExpr = new TruncateExpr{ sExpr, conv.from->type };
					}
					results.push_back(
						new Interpretation{ sExpr, i->env, i->cost + sCost, i->cost } );
				}
			}
		}
	}

	// Ditto above for anything with a polymorphic return type
	auto polys = funcIndex.get_poly_maps( targetType );
	// skip exact concrete match
	if ( polys.begin() != polys.end() && polys.begin().is_concrete() ) { ++polys; }
	for ( const TypeMap<FuncList>& matches : polys ) {
		for ( auto it = matches.begin(); it != matches.end(); ++it ) {
			const Type* keyType = it.key();

			// results for all the functions with that type
			InterpretationList sResults = resolveToAny(
				resolver, it.get(), expr, env, Resolver::NO_CONVERSIONS, boundType );
			if ( sResults.empty() ) continue;

			// truncate expressions to match result type
			unsigned n = targetType->size();
			bool trunc = keyType->size() > n;
			Cost sCost = Cost::zero();
			if ( trunc ) { sCost += Cost::from_safe( n - targetType->size() ); }
			for ( const Interpretation* i : sResults ) {
				const TypedExpr* sExpr = trunc ? new TruncateExpr{ i->expr, n } : i->expr;
				results.push_back( new Interpretation{ sExpr, i->env, i->cost + sCost, i->cost } );
			}
		}
	}
	// TODO think about caching child resolutions across top-level parameters
	return results;
}

InterpretationList Resolver::resolve( const Expr* expr, const Env* env, 
                                      Resolver::Mode resolve_mode ) {
	auto eid = typeof(expr);
	if ( eid == typeof<VarExpr>() ) {
		return InterpretationList{ new Interpretation{ as<VarExpr>(expr), Env::from(env) } };
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

InterpretationList Resolver::resolveWithType( const Expr* expr, const Type* targetType,
                                              const Env* env ) {
	auto eid = typeof(expr);
	if ( eid == typeof<VarExpr>() ) {
		// convert leaf expression to given type
		Env* rEnv = Env::from( env );
		Cost rCost;
		const TypedExpr* rExpr = 
			convertTo( targetType, as<VarExpr>(expr), conversions, rEnv, rCost );
		// return expression if applicable
		return rExpr 
			? InterpretationList{ new Interpretation{ rExpr, rEnv, move(rCost) } }
			: InterpretationList{};
	} else if ( eid == typeof<FuncExpr>() ) {
		const FuncExpr* funcExpr = as<FuncExpr>(expr);
		// find candidates with this function name, skipping if none
		auto withName = funcs.find( funcExpr->name() );
		if ( withName == funcs.end() ) return InterpretationList{};

		if ( const PolyType* pTarget = as_safe<PolyType>(targetType) ) {
			// polymorphic return; look for any option if unbound, bind to polytype
			Env* rEnv = Env::from( env );
			ClassRef pClass = getClass( rEnv, pTarget );
			return ( pClass->bound )
				? resolveTo( *this, withName(), funcExpr, pClass->bound, rEnv, pTarget )
				: resolveToAny( *this, withName(), funcExpr, rEnv, 
						is<VoidType>(targetType) ? TOP_LEVEL : ALL_NON_VOID, pTarget );
		} else {
			// monomorphic return; look for matching options
			return resolveTo( *this, withName(), funcExpr, targetType, env );
		}
	} else assert(!"Unsupported expression type");

	return {};
}