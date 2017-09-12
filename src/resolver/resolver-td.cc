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
#include "type_map.h"
#include "unify.h"

#include "ast/decl.h"
#include "ast/expr.h"
#include "ast/type.h"
#include "data/cast.h"
#include "data/mem.h"
#include "data/range.h"

/// Resolves to an unbound type variable
InterpretationList resolveToUnbound( Resolver& resolver, const Expr* expr, 
									 const PolyType* targetType, const Env* env, 
									 bool truncate = true ) {
	InterpretationList results;
	for ( const Interpretation* i : resolver.resolve( expr, env ) ) {
		// loop over subexpression results, binding result types to target type
		const TypedExpr* rExpr = i->expr;
		Env* rEnv = Env::from( i->env );
		ClassRef r = getClass( rEnv, targetType );
		Cost rCost = i->cost;

		const Type* rType = i->type();
		const TupleType* tType = as_safe<TupleType>(rType);
		if ( tType ) { rType = tType->types()[0]; }
		
		auto rid = typeof(rType);
		if ( rid == typeof<ConcType>() || rid == typeof<NamedType>() ) {
			if ( r->bound ) {
				// skip incompatibly-bound results
				if ( *rType != *r->bound ) continue;
			}
			bindType( rEnv, r, rType );
			++rCost.poly;
		} else if ( rid == typeof<PolyType>() ) {
			// skip incompatibly-bound results
			if ( ! bindVar( rEnv, r, as<PolyType>(rType) ) ) continue;
			rCost.poly += 2;
		} else continue; // skip results that aren't atomic types

		// optionally truncate tuple expressions
		if ( tType != nullptr && truncate ) {
			rExpr = new TruncateExpr{ rExpr, rType };
			rCost.safe += tType->size() - 1;
		}

		// build new result with updated environment
		results.push_back( new Interpretation{ rExpr, rEnv, move(rCost), copy(i->argCost) } );
	}
	return results;
}

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
	ArgPack(const ArgPack& old, const Interpretation* i, unsigned leftover = 0)
		: env( Env::from(i->env) ), cost(old.cost + i->cost), argCost(old.argCost + i->argCost), 
		  args(old.args), crnt(leftover ? i->expr : nullptr), on_last(leftover), next(old.next) {
		if ( old.crnt ) args.push_back(old.crnt);
		if ( on_last == 0 ) args.push_back(i->expr);
		++next;
	}

	/// Update ArgPack with new binding for leftover arg portion
	ArgPack(const ArgPack& old, Cost&& newCost, Env* newEnv, unsigned leftover)
		: env(newEnv), cost(move(newCost)), argCost(old.argCost), args(old.args), 
		  crnt(leftover ? old.crnt : nullptr), on_last(leftover), next(old.next) {
		if ( on_last == 0 ) args.push_back(old.crnt);
	}

	/// Truncate crnt if applicable to exclude on_last, add to arguments
	void finalize() {
		if ( on_last > 0 ) {
			args.push_back( new TruncateExpr{ crnt, crnt->type()->size() - on_last } );
			crnt = nullptr;
		}
	}
};
InterpretationList resolveWithExtType( Resolver&, const Expr*, const Type*, const Env*, bool );

/// Resolves a function expression with any return type, binding the result to `bound` if 
/// a valid ref
template<typename Funcs>
InterpretationList resolveToAny( Resolver& resolver, const Funcs& funcs, 
                                 const FuncExpr* expr, const Env* env, 
                                 Resolver::Mode resolve_mode = {}, 
								 const PolyType* boundType = nullptr ) {
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
				if ( resolve_mode.check_assertions 
					 && ! resolveAssertions( resolver, call, rCost, rEnv ) ) continue;
				
				results.push_back( new Interpretation{ call, rEnv, move(rCost) } );
			} break;
			case 1: {
				// single arg, accept and move up
				InterpretationList& subs = resolver.cached(
					Type::from( rParams ), expr->args().front(), rEnv,
					[&resolver]( const Type* ty, const Expr* e, const Env* env ) {
						return resolver.resolveWithType( e, ty, env );
					} );
				for ( const Interpretation* sub : subs ) {
					// make interpretation for this arg
					const TypedExpr* call = 
						new CallExpr{ func, List<TypedExpr>{ sub->expr }, copy(rForall), rType };
					Cost sCost = rCost + sub->cost;
					Env* sEnv = Env::from( sub->env );
					
					// check assertions if at top level
					if ( resolve_mode.check_assertions 
						 && ! resolveAssertions( resolver, call, sCost, sEnv ) ) continue;
					
					results.push_back(
						new Interpretation{ call, sEnv, move(sCost), copy(sub->argCost) } );
				}
			} break;
			default: {
				// multiple args; proceed through matches
				std::vector<ArgPack> combos{ ArgPack{ expr->args().begin(), rEnv } };
				std::vector<ArgPack> nextCombos{};

				// Build match combos, one parameter at a time
				for ( const Type* param : rParams ) {
					assert(param->size() == 1 && "parameter list should be flattened");
					// find interpretations with this type
					for ( ArgPack& combo : combos ) {
						if ( combo.on_last > 0 ) {
							// test matches for leftover combo args
							const TupleType* cType = as_checked<TupleType>(combo.crnt->type());
							unsigned cInd = cType->size() - combo.on_last;
							const Type* crntType = cType->types()[ cInd ];
							Cost cCost = combo.cost;
							Env* cEnv = Env::from( combo.env );

							if ( unify( param, crntType, cCost, cEnv ) ) {
								// build new combo which consumes more arguments
								nextCombos.emplace_back( 
									combo, move(cCost), cEnv, combo.on_last - 1 );
							}

							// replace combo with truncated expression
							combo.crnt = new TruncateExpr{ combo.crnt, cInd };
						}
						// skip combos with no arguments left
						if ( combo.next == expr->args().end() ) continue;
						// Find interpretations for this argument
						InterpretationList& subs = resolver.cached(
							param, *combo.next, combo.env,
							[&resolver]( const Type* ty, const Expr* e, const Env* env ) {
								return resolveWithExtType( resolver, e, ty, env, false );
							} );
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
					combo.finalize();

					// make interpretation for this combo
					const TypedExpr* call = 
						new CallExpr{ func, move(combo.args), copy(rForall), rType };
					Cost cCost = rCost + combo.cost;
					
					// check assertions if at top level
					if ( resolve_mode.check_assertions
					     && ! resolveAssertions( resolver, call, cCost, combo.env ) )
						continue;
					
					results.push_back(
						new Interpretation{ call, combo.env, move(cCost), move(combo.argCost) } );
				}
			} break;
		}
	}

	if ( resolve_mode.expand_conversions ) {
		expandConversions( results, resolver.conversions );
	}

	return results;
}

/// Resolves a function expression with a fixed return type, binding the result to 
/// `bound` if a valid ref
InterpretationList resolveTo( Resolver& resolver, const FuncSubTable& funcs, const FuncExpr* expr, 
                              const Type* targetType, const Env* env, 
							  const PolyType* boundType = nullptr, bool truncate = true ) {
	InterpretationList results;
	const auto& funcIndex = funcs.index();
	// replace target by concrete target if polymorphic
	if ( boundType ) {
		targetType = replace( env, boundType );
	}
	
	// For any type (or tuple that has the target type as a prefix)
	if ( const TypeMap<FuncList>* matches = funcIndex.get( targetType ) ) {
		for ( auto it = matches->begin(); it != matches->end(); ++it ) {
			// results for all the functions with that type
			InterpretationList sResults = resolveToAny( 
					resolver, it.get(), expr, env, 
					Resolver::Mode{}.without_conversions().with_void_if(is<VoidType>(targetType)), 
					boundType );
			if ( sResults.empty() ) continue;

			const Type* keyType = it.key();
			bool trunc = truncate && keyType->size() > targetType->size();
			Cost sCost = Cost::zero();
			if ( trunc ) { sCost.safe += keyType->size() - targetType->size(); }
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
					resolver, it.get(), expr, env, Resolver::Mode{}.without_conversions(), 
					boundType );
				if ( sResults.empty() ) continue;

				// cast and perhaps truncate expressions to match result type
				const Type* keyType = it.key();
				unsigned kn = keyType->size(), cn = conv.from->type->size();
				bool trunc = truncate && kn > cn;
				Cost sCost = conv.cost;
				if ( trunc ) { sCost.safe += kn - cn; }
				for ( const Interpretation* i : sResults ) {
					const TypedExpr* sExpr = nullptr;
					if ( trunc ) {
						sExpr = new CastExpr{ new TruncateExpr{ i->expr, conv.from->type }, &conv };
					} else if ( kn > cn ) {
						assert( cn == 1 && "multi-element conversions unsupported" );
						List<TypedExpr> els;
						els.reserve( kn );
						els.push_back( new CastExpr{ new TupleElementExpr{ i->expr, 0 }, &conv } );
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
			InterpretationList sResults = resolveToAny(
				resolver, it.get(), expr, env, Resolver::Mode{}.without_conversions(), boundType );
			if ( sResults.empty() ) continue;

			// truncate expressions to match result type
			unsigned n = targetType->size();
			bool trunc = truncate && keyType->size() > n;
			Cost sCost = Cost::zero();
			if ( trunc ) { sCost.safe += keyType->size() - n; }
			for ( const Interpretation* i : sResults ) {
				const TypedExpr* sExpr = trunc ? new TruncateExpr{ i->expr, n } : i->expr;
				results.push_back( new Interpretation{ sExpr, i->env, i->cost + sCost, i->cost } );
			}
		}
	}
	
	return results;
}

InterpretationList Resolver::resolve( const Expr* expr, const Env* env, 
                                      Resolver::Mode resolve_mode ) {
	auto eid = typeof(expr);
	if ( eid == typeof<VarExpr>() ) {
		InterpretationList results{ new Interpretation{ as<VarExpr>(expr), Env::from(env) } };
		if ( resolve_mode.expand_conversions ) {
			expandConversions( results, conversions );
		}
		return results;
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

/// Resolves expression with the target type, subject to env, optionally truncating result 
/// expresssions.
InterpretationList resolveWithExtType( Resolver& resolver, const Expr* expr,  
		const Type* targetType, const Env* env, bool truncate = true ) {
	// if the target type is polymorphic and unbound, expand out all conversion options
	const PolyType* pTarget = as_safe<PolyType>(targetType);
	if ( pTarget ) {
		ClassRef pClass = findClass( env, pTarget );
		if ( !pClass || !pClass->bound ) {
			return resolveToUnbound( resolver, expr, pTarget, env, truncate );
		}
	}

	auto eid = typeof(expr);
	if ( eid == typeof<VarExpr>() ) {
		// convert leaf expression to given type
		Env* rEnv = Env::from( env );
		Cost rCost;
		const TypedExpr* rExpr =
			convertTo( targetType, as<VarExpr>(expr), resolver.conversions, rEnv, rCost, truncate );
		// return expression if applicable
		return rExpr 
			? InterpretationList{ new Interpretation{ rExpr, rEnv, move(rCost) } }
			: InterpretationList{};
	} else if ( eid == typeof<FuncExpr>() ) {
		const FuncExpr* funcExpr = as<FuncExpr>(expr);
		// find candidates with this function name, skipping if none
		auto withName = resolver.funcs.find( funcExpr->name() );
		if ( withName == resolver.funcs.end() ) return InterpretationList{};

		return resolveTo( resolver, withName(), funcExpr, targetType, env, pTarget, truncate );
	} else assert(!"Unsupported expression type");

	return {}; // unreachable
}

InterpretationList Resolver::resolveWithType( const Expr* expr, const Type* targetType,
                                              const Env* env ) {
	return resolveWithExtType( *this, expr, targetType, env );
}