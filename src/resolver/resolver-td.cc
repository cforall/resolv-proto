// Copyright (c) 2015 University of Waterloo
//
// The contents of this file are covered under the licence agreement in 
// the file "LICENCE" distributed with this repository.

#include <functional>
#include <vector>
#include <unordered_map>
#include <utility>

#include "resolver.h"

#include "arg_pack.h"
#include "cost.h"
#include "env.h"
#include "env_substitutor.h"
#include "expand_conversions.h"
#include "func_table.h"
#include "interpretation.h"
#include "resolve_assertions.h"
#include "type_map.h"
#include "unify.h"

#include "ast/decl.h"
#include "ast/expr.h"
#include "ast/is_poly.h"
#include "ast/type.h"
#include "data/cast.h"
#include "data/debug.h"
#include "data/mem.h"
#include "data/range.h"

/// Resolves to an unbound type variable
InterpretationList resolveToUnbound( Resolver& resolver, const Expr* expr, 
									 const PolyType* targetType, const Env& env, 
									 ResolverMode resolve_mode = {} ) {
	InterpretationList results;

#if defined RP_ASN_IMM
	auto mode = ResolverMode{}.with_assertions_if( resolve_mode.check_assertions );
#else
	auto mode = ResolverMode{};
#endif
	InterpretationList& subs = resolver.cached( expr, mode, 
		[&resolver,&env]( const Expr* e, ResolverMode mode ) {
			return resolver.resolve( e, env, mode );
		} );

	for ( const Interpretation* i : subs ) {
		// loop over subexpression results, binding result types to target type
		const TypedExpr* rExpr = i->expr;
		Env rEnv = env;
		if ( i->env && ! rEnv.merge( i->env ) ) continue;
		ClassRef r = rEnv.getClass( targetType );
		Cost rCost = i->cost;

		const Type* rType = i->type();
		const TupleType* tType = as_safe<TupleType>(rType);
		if ( tType ) { rType = tType->types()[0]; }
		
		auto rid = typeof(rType);
		if ( rid == typeof<ConcType>() || rid == typeof<NamedType>() ) {
			if ( ! classBinds( r, rType, rEnv, rCost ) ) continue;
		} else if ( rid == typeof<PolyType>() ) {
			// skip incompatibly-bound results
			if ( ! rEnv.bindVar( r, as<PolyType>(rType) ) ) continue;
			rCost.poly += 2;
		} else continue; // skip results that aren't atomic types

		// optionally truncate tuple expressions
		if ( tType != nullptr && resolve_mode.truncate ) {
			rExpr = new TruncateExpr{ rExpr, rType };
			rCost.safe += tType->size() - 1;
		}

		// build new result with updated environment
		results.push_back( new Interpretation{ rExpr, move(rEnv), move(rCost), copy(i->argCost) } );
	}
	return results;
}

InterpretationList resolveWithExtType( 
	Resolver&, const Expr*, const Type*, const Env&, ResolverMode );

/// Returns true if the first type element of rType unifyies with boundType
bool unifyExt( const Type* boundType, const Type* rType, Cost& rCost, Env& rEnv ) {
	if ( const TupleType* trType = as_safe<TupleType>(rType) ) {
		return unify( boundType, trType->types()[0], rCost, rEnv );
	} else return unify( boundType, rType, rCost, rEnv );
}

/// Resolves a function expression with any return type
template<typename Funcs>
InterpretationList resolveToAny( Resolver& resolver, const Funcs& funcs, 
                                 const FuncExpr* expr, const Env& env, 
                                 ResolverMode resolve_mode = {} ) {
	InterpretationList results{};
	
	
	for ( const FuncDecl* func : funcs ) {
		// skip void-returning functions unless at top level
		if ( ! resolve_mode.allow_void && func->returns()->size() == 0 ) continue;

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
		Env rEnv = env;

		switch( expr->args().size() ) {
			case 0: {
				// no args, just add interpretation
				const TypedExpr* call = 
					new CallExpr{ func, List<TypedExpr>{}, move(rForall), rType };
				
				// check assertions if at top level
				if ( RP_ASSN_CHECK( ! resolveAssertions( resolver, call, rEnv ) ) ) continue;
				
				results.push_back( new Interpretation{ call, move(rEnv), func->poly_cost() } );
			} break;
			case 1: {
				// single arg, accept and move up
				InterpretationList subs = resolveWithExtType(
					resolver, expr->args().front(), Type::from( rParams ), rEnv, resolve_mode );
				for ( const Interpretation* sub : subs ) {
					// make interpretation for this arg
					const TypedExpr* call = new CallExpr{ 
						func, List<TypedExpr>{ sub->expr }, copy(rForall), rType };
					Env sEnv = sub->env;
					
					// check assertions if at top level
					if ( RP_ASSN_CHECK( ! resolveAssertions( resolver, call, sEnv ) ) ) 
						continue;
					
					results.push_back( new Interpretation{ 
						call, move(sEnv), func->poly_cost() + sub->cost, copy(sub->argCost) } );
				}
			} break;
			default: {
				// multiple args; proceed through matches
				std::vector<ArgPack> combos{ ArgPack{ expr->args().begin(), rEnv } };
				std::vector<ArgPack> nextCombos{};

				// Build match combos, one parameter at a time
				for ( const Type* param : rParams ) {
					assume(param->size() == 1, "parameter list should be flattened");
					// find interpretations with this type
					for ( ArgPack& combo : combos ) {
						// test matches for leftover combo args
						if ( combo.on_last > 0 ) {
							const TupleType* cType = as_checked<TupleType>(combo.crnt->type());
							unsigned cInd = cType->size() - combo.on_last;
							const Type* crntType = cType->types()[ cInd ];
							Cost cCost = combo.cost;
							Env cEnv = combo.env;

							if ( unify( param, crntType, cCost, cEnv ) ) {
								// build new combo which consumes more arguments
								nextCombos.emplace_back( 
									combo, move(cCost), move(cEnv), combo.on_last - 1 );
							}

							// replace combo with truncated expression
							combo.truncate();
						}
						// skip combos with no arguments left
						if ( combo.next == expr->args().end() ) continue;
						// Find interpretations for this argument
						InterpretationList subs = resolveWithExtType( 
							resolver, *combo.next, param, combo.env, 
							copy(resolve_mode).without_truncation() );
						// build new combos from interpretations
						for ( const Interpretation* i : subs ) {
							// Build new ArgPack for this interpretation, noting leftovers
							nextCombos.emplace_back( combo, i, i->type()->size() - 1 );
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

					// truncate any incomplete combos
					combo.truncate();

					// make interpretation for this combo
					const TypedExpr* call = 
						new CallExpr{ func, move(combo.args), copy(rForall), rType };
					Env& cEnv = const_cast<Env&>(combo.env);
					
					// check assertions if at top level
					if ( RP_ASSN_CHECK( ! resolveAssertions( resolver, call, cEnv ) ) )
						continue;
					
					results.push_back( new Interpretation{ 
						call, move(cEnv), func->poly_cost() + combo.cost, 
						move(combo.argCost) } );
				}
			} break;
		}
	}

	if ( resolve_mode.expand_conversions ) {
		expandConversions( results, resolver.conversions );
	}

	return results;
}

/// Resolves a function expression with a fixed return type
template<typename Funcs>
InterpretationList resolveTo( Resolver& resolver, const Funcs& funcs, const FuncExpr* expr, 
                              const Type* targetType, ResolverMode resolve_mode = {} ) {
	InterpretationList results;
	Env env;
	
	for ( const auto& fit : funcs ) {
		const auto& funcIndex = fit->second.index();

		// For any type (or tuple that has the target type as a prefix)
		if ( const TypeMap<FuncList>* matches = funcIndex.get( targetType ) ) {
			for ( auto it = matches->begin(); it != matches->end(); ++it ) {
				// results for all the functions with that type
				Env matchEnv = env;
				InterpretationList sResults = resolveToAny( 
						resolver, it.get(), expr, matchEnv, 
						copy(resolve_mode).without_conversions().with_void_as( targetType ) );
				if ( sResults.empty() ) continue;

				const Type* keyType = it.key();
				bool trunc = resolve_mode.truncate && keyType->size() > targetType->size();
				Cost sCost = Cost::zero();
				if ( trunc ) { sCost.safe += keyType->size() - targetType->size(); }
				for ( const Interpretation* i : sResults ) {
					const TypedExpr* sExpr = 
						trunc ? new TruncateExpr{ i->expr, targetType } : i->expr;
					results.push_back( 
						new Interpretation{ sExpr, i->env, i->cost + sCost, i->cost } );
				}
			}
		}

		// Loop through conversions to targetType, repeating above
		for ( const Conversion& conv : resolver.conversions.find_to( targetType ) ) {
			// for any type (or tuple that has the conversion type as a prefix)
			if ( const TypeMap<FuncList>* matches = funcIndex.get( conv.from->type ) ) {
				for ( auto it = matches->begin(); it != matches->end(); ++it ) {
					// results for all the functions with that type
					Env convEnv = env;
					InterpretationList sResults = resolveToAny(
						resolver, it.get(), expr, convEnv, 
						copy(resolve_mode).without_conversions() );
					if ( sResults.empty() ) continue;

					// cast and perhaps truncate expressions to match result type
					const Type* keyType = it.key();
					unsigned kn = keyType->size(), cn = conv.from->type->size();
					bool trunc = resolve_mode.truncate && kn > cn;
					Cost sCost = conv.cost;
					if ( trunc ) { sCost.safe += kn - cn; }
					for ( const Interpretation* i : sResults ) {
						const TypedExpr* sExpr = nullptr;
						if ( trunc ) {
							sExpr = 
								new CastExpr{ new TruncateExpr{ i->expr, conv.from->type }, &conv };
						} else if ( kn > cn ) {
							assume( cn == 1, "multi-element conversions unsupported" );
							List<TypedExpr> els;
							els.reserve( kn );
							els.push_back( 
								new CastExpr{ new TupleElementExpr{ i->expr, 0 }, &conv } );
							for (unsigned j = 1; j < kn; ++j) {
								els.push_back( new TupleElementExpr{ i->expr, j } );
							}
							sExpr = new TupleExpr{ move(els) };
						} else {
							sExpr = new CastExpr{ i->expr, &conv };
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
				Env sEnv = env;
				InterpretationList sResults = resolveToAny(
					resolver, it.get(), expr, sEnv, copy(resolve_mode).without_conversions() );
				if ( sResults.empty() ) continue;

				// truncate and unify expressions to match result type
				unsigned n = targetType->size();
				bool trunc = resolve_mode.truncate && keyType->size() > n;
				Cost sCost = trunc ? Cost::zero() : Cost::from_safe( keyType->size() - n );
				for ( const Interpretation* i : sResults ) {
					Env iEnv = i->env;
					Cost iCost = i->cost;
					if ( ! unify( targetType, i->type(), iCost, iEnv ) ) continue;

					const TypedExpr* sExpr = trunc ? new TruncateExpr{ i->expr, n } : i->expr;
					results.push_back( new Interpretation{ sExpr, iEnv, iCost + sCost, i->cost } );
				}
			}
		}
	}
	
	return results;
}

template<typename Funcs>
InterpretationList resolveToPoly( Resolver& resolver, const Funcs& funcs, 
                                  const FuncExpr* expr, const Type* targetType, 
								  ResolverMode resolve_mode = {} ) {
	InterpretationList results;
	Env env;

	for ( const auto& fit : funcs ) {
		const auto& funcIndex = fit->second.index();

		// For any function that matches the target type (any polymorphic variables should be unbound)
		for ( const TypeMap<FuncList>& matches : funcIndex.get_matching( targetType ) ) {
			for ( auto it = matches.begin(); it != matches.end(); ++it ) {
				// results for all the functions with that type
				Env matchEnv = env;
				InterpretationList sResults = resolveToAny(
					resolver, it.get(), expr, matchEnv, 
					copy(resolve_mode).without_conversions().with_void_as( targetType ) );
				if ( sResults.empty() ) continue;

				const Type* keyType = it.key();
				bool trunc = resolve_mode.truncate && keyType->size() > targetType->size();
				Cost sCost = Cost::zero();
				if ( trunc ) { sCost.safe += keyType->size() - targetType->size(); }
				for ( const Interpretation* i : sResults ) {
					const TypedExpr* sExpr = 
						trunc ? new TruncateExpr{ i->expr, targetType } : i->expr;
					results.push_back( 
						new Interpretation{ sExpr, i->env, i->cost + sCost, i->cost } );
				}
			}
		}

		// Loop through conversions to types matching target type, repeating above
		const auto& conversions = resolver.conversions;
		for ( const TypeMap<ConversionNode>& convMatches 
				: conversions.find_matching( targetType ) ) {
			const ConversionNode* conv = convMatches.get();
			if ( ! conv ) continue;  // skip matches for extensions of this type

			// unify target type with conversion target
			const Type* convTarget = conv->type;
			Cost convCost = Cost::zero();
			Env convEnv = env;
			unify( targetType, convTarget, convCost, convEnv );

			// loop through conversions to conversion target
			for ( const Conversion& conv : conversions.range_of( conv->in ) ) {
				const Type* fromType = conv.from->type;
				// for any type (or tuple that has the conversion type as a prefix)
				if ( const TypeMap<FuncList>* matches = funcIndex.get( fromType ) ) {
					for ( auto it = matches->begin(); it != matches->end(); ++it ) {
						// results for all functions with that type
						InterpretationList sResults = resolveToAny(
							resolver, it.get(), expr, convEnv, 
							copy(resolve_mode).without_conversions() );
						if ( sResults.empty() ) continue;

						// cast and perhaps truncate expressions to match result type
						const Type* keyType = it.key();
						unsigned kn = keyType->size(), cn = fromType->size();
						bool trunc = resolve_mode.truncate && kn > cn;
						Cost sCost = convCost + conv.cost;
						if ( trunc ) { sCost.safe += kn - cn; }
						for ( const Interpretation* i : sResults ) {
							const TypedExpr* sExpr = nullptr;
							if ( trunc ) {
								sExpr = 
									new CastExpr{ new TruncateExpr{ i->expr, fromType }, &conv };
							} else if ( kn > cn ) {
								assume( cn == 1, "multi-element conversions unsupported" );
								List<TypedExpr> els;
								els.reserve(kn);
								els.push_back(
									new CastExpr{ new TupleElementExpr{ i->expr, 0 }, &conv } );
								for (unsigned j = 1; j < kn; ++j) {
									els.push_back( new TupleElementExpr{ i->expr, j } );
								}
								sExpr = new TupleExpr{ move(els) };
							} else {
								sExpr = new CastExpr{ i->expr, &conv };
							}

							results.push_back(
								new Interpretation{ sExpr, i->env, i->cost + sCost, i->cost } );
						}
					}
				}
			}
		}
	}

	return results;
}

InterpretationList Resolver::resolve( const Expr* expr, const Env& env, 
                                      ResolverMode resolve_mode ) {
	InterpretationList results;

	auto eid = typeof(expr);
	if ( eid == typeof<ValExpr>() ) {
		results.push_back( new Interpretation{ as<ValExpr>(expr), copy(env) } );	
	} else if ( eid == typeof<NameExpr>() ) {
		// find all candidates with this name
		auto withName = funcs.vars.findAll( as<NameExpr>(expr)->name() );
		for ( const auto& it : withName ) {
			for ( const Decl* decl : it->second ) {
				results.push_back(
					new Interpretation{ new VarExpr{ decl }, copy(env), Cost::zero() } );
			}
		}
	} else if ( eid == typeof<FuncExpr>() ) {
		const FuncExpr* funcExpr = as<FuncExpr>(expr);
		// find candidates with this function name, skipping if none
		auto withName = funcs.funcs.findAll( funcExpr->name() );
		
		// return all candidates with this name
		for ( const auto& it : withName ) {
			InterpretationList sresults = resolveToAny( 
				*this, it->second, funcExpr, env, copy(resolve_mode).without_conversions() );
			results.reserve( results.size() + sresults.size() );
			results.insert( results.end(), sresults.begin(), sresults.end() );
		}
	} else unreachable("Unsupported expression type");

	if ( resolve_mode.expand_conversions ) {
		expandConversions( results, conversions );
	}
	return results;
}

/// Resolves expression with the target type, subject to env, optionally truncating result 
/// expresssions.
InterpretationList resolveWithExtType( Resolver& resolver, const Expr* expr,  
		const Type* targetType, const Env& env, ResolverMode resolve_mode = {} ) {
	Cost rCost;

	// if the target type is partially polymorphic, attempt to make it concrete by the current 
	// environment
	CostedSubstitutor sub{ env, rCost.poly };
	sub( targetType );
	if ( is<PolyType>( targetType ) ) {
		// only an unbound poly type would survive as the target
		return resolveToUnbound( resolver, expr, as<PolyType>(targetType), env, resolve_mode );
	}

	auto eid = typeof(expr);
	if ( eid == typeof<ValExpr>() ) {
		// convert leaf expression to given type
		Env rEnv = env;
		const TypedExpr* rExpr =
			convertTo( targetType, as<ValExpr>(expr), resolver.conversions, rEnv, rCost, 
			           resolve_mode.truncate );
		// return expression if applicable
		return rExpr 
			? InterpretationList{ new Interpretation{ rExpr, copy(rEnv), move(rCost) } }
			: InterpretationList{};
	} else if ( eid == typeof<NameExpr>() ) {
		InterpretationList results;

		// find all candidates with this name
		auto withName = resolver.funcs.vars.findAll( as<NameExpr>(expr)->name() );
		for ( const auto& it : withName ) {
			for ( const Decl* decl : it->second ) {
				results.push_back( 
					new Interpretation{ new VarExpr{ decl }, copy(env), Cost::zero() } );
			}
		}

		// return min-cost interpretations as target
		return convertTo( targetType, move(results), resolver.conversions );
	} else if ( eid == typeof<FuncExpr>() ) {
		const FuncExpr* funcExpr = as<FuncExpr>(expr);
		// find candidates with this function name, skipping if none
		auto withName = resolver.funcs.funcs.findAll( funcExpr->name() );
		
		// pull resolutions from cache
		InterpretationList& subs = sub.is_poly() ?
			resolver.cached( targetType, funcExpr, resolve_mode, 
				[&resolver,&withName]( const Type* ty, const Expr* e, ResolverMode mode ) {
					return resolveToPoly( 
						resolver, withName, as<FuncExpr>(e), ty, mode );
				} ) :
			resolver.cached( targetType, funcExpr, resolve_mode, 
				[&resolver,&withName]( const Type* ty, const Expr* e, ResolverMode mode ) {
					return resolveTo( resolver, withName, as<FuncExpr>(e), ty, mode ); 
				} );
		if ( env.empty() && rCost == Cost::zero() ) return subs;
		
		InterpretationList rSubs;
		rSubs.reserve( subs.size() );
		for ( const Interpretation* i : subs ) {
			const Type* sType = i->type();
			Env sEnv = env;
			Cost sCost = is_poly(sType) ? i->cost : rCost + i->cost;
			if ( sub.is_poly() && ! unifyExt( targetType, sType, sCost, sEnv ) ) continue;
			if ( i->env && ! sEnv.merge( i->env ) ) continue;
			rSubs.push_back( 
				new Interpretation{ i->expr, move(sEnv), move(sCost), copy(i->argCost) } );
		}
		return rSubs;
	} else unreachable("Unsupported expression type");

	return {}; // unreachable
}

InterpretationList Resolver::resolveWithType( const Expr* expr, const Type* targetType,
                                              const Env& env ) {
#if defined RP_ASN_IMM
	auto resolve_mode = ResolverMode{}.without_assertions();
#else
	auto resolve_mode = ResolverMode{};
#endif
	return resolveWithExtType( *this, expr, targetType, env, resolve_mode );
}