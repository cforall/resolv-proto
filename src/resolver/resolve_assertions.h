#pragma once

#include <algorithm>
#include <vector>
#include <unordered_map>
#include <utility>

#include "binding.h"
#include "binding_sub_mutator.h"
#include "cost.h"
#include "environment.h"
#include "interpretation.h"
#include "resolver.h"

#include "ast/decl.h"
#include "ast/expr.h"
#include "ast/mutator.h"
#include "ast/type.h"
#include "ast/typed_expr_mutator.h"
#include "data/collections.h"
#include "data/cow.h"
#include "data/list.h"
#include "data/mem.h"
#include "data/option.h"
#include "merge/filter.h"

// /// Recursively checks the type assertions of a function call, branching on changes to the 
// /// environment
// bool assertionsUnresolvable( Resolver& resolver, TypeBinding* bindings, 
//                              TypeBinding::AssertionList::const_iterator it, 
//                              TypeBinding::AssertionList::const_iterator end, 
// 							 Cost& cost, cow_ptr<Environment>& env, InterpretationList& out ) {
// 	BindingEnvSubMutator replaceLocals{ *bindings, env };  // replace poly types with bindings
// 	cow_ptr<Environment> env_bak = env;  // back up environment in case of failure

// 	// XXX this possibly exponential search of the backtracking list is *wrong*, and one of the 
// 	// big performance issues in the current resolver.
// 	// - it might be better to do a linear search, then try and merge the environments.
// 	// - what kind of environment structure would I need?
// 	//   - Union-find for the type classes?
// 	//   - If you tag each node by the environment it belongs to, you can build a persistent-ish
// 	//     environment data structure with a pointer to its parent structure (like your indexer)
// 	//     and a local map of union-find nodes
// 	//   - On second thought, union-find likely doesn't work so well for the type classes, as 
// 	//     it makes it difficult to reconstruct the whole class from one element. The parent idea 
// 	//     might work, though.

// 	do {
// 		// Generate FuncExpr for assertion
// 		const FuncDecl* asnDecl = it->first;
// 		List<Expr> asnArgs;
// 		asnArgs.reserve( asnDecl->params().size() );
// 		for ( const Type* pType : asnDecl->params() ) {
// 			asnArgs.push_back( new VarExpr( replaceLocals( pType ) ) );
// 		}
// 		const FuncExpr* asnFunc = new FuncExpr{ asnDecl->name(), move(asnArgs) };
// 		const Type* asnReturn = replaceLocals( asnDecl->returns() );

// 		// back up environment and check if resolution works
// 		InterpretationList satisfying = resolver.resolveWithType( asnFunc, asnReturn, env );
// 		if ( satisfying.empty() ) {
// 			env = move(env_bak);  // reset environment to previous state
// 			return true;
// 		}
		
// 		if ( satisfying.size() > 1 ) {
// 			// look for unique min-cost interpretation of the rest of the assertions
// 			const Interpretation* mini = nullptr;
// 			Cost minCost = Cost::max();
// 			cow_ptr<Environment> minEnv;
// 			InterpretationList minOut;
// 			++it;

// 			for ( const Interpretation* i : satisfying ) {
// 				Cost icost = i->cost;
// 				cow_ptr<Environment> ienv = i->env;
// 				InterpretationList iout;
// 				// skip interpretations that don't lead to valid bindings
// 				if ( it != end && 
// 						assertionsUnresolvable( resolver, bindings, it, end, icost, ienv, iout ) )
// 					continue;
				
// 				// skip interpretations that are more expensive than those already seen
// 				if ( icost > minCost ) continue;

// 				if ( icost < minCost ) {
// 					mini = i;
// 					minCost = move(icost);
// 					minEnv = move(ienv);
// 					minOut = move(iout);
// 				} else if ( icost == minCost ) {
// 					// TODO report ambiguity
// 					mini = nullptr;
// 					minCost = Cost::max();
// 					minEnv = nullptr;
// 					minOut.clear();
// 				}
// 			}

// 			if ( ! mini ) return true;

// 			// record min-cost matching
// 			out.reserve( out.size() + 1 + minOut.size() );
// 			out.push_back( mini );
// 			out.insert( out.end(), minOut.begin(), minOut.end() );
// 			env = minEnv;
// 			cost += minCost;
// 		}
		
// 		// record resolution
// 		const Interpretation* i = satisfying.front();
// 		out.push_back( i );
// 		env = i->env;
// 		cost += i->cost;
// 	} while ( ++it != end );

// 	return false;
// }

/// Combo iterator that combines interpretations into an interpretation list, merging their 
/// environments. Rejects an appended interpretation if the environments cannot be merged.
class interpretation_environment_merger {
	InterpretationList crnt;                 ///< Current list of merged interpretations
	std::vector<cow_ptr<Environment>> envs;  ///< Stack of environments, to support backtracking
public:
	/// Outputs a pair consisting of the merged environment and the list of interpretations
	using OutType = std::pair<cow_ptr<Environment>, InterpretationList>;

	interpretation_environment_merger() : crnt{}, envs{ cow_ptr<Environment>{} } {}

	ComboResult append( const Interpretation* i ) {
		cow_ptr<Environment> env = envs.back();
		if ( ! merge( env, i->env ) ) return ComboResult::REJECT_THIS;
		crnt.push_back( i );
		envs.push_back( move(env) );
		return ComboResult::ACCEPT;
	}

	void backtrack() {
		crnt.pop_back();
		envs.pop_back();
	}

	OutType finalize() {
		// backtrack always called after finalize
		return { move( envs.back() ), crnt };
	}
};

/// Comparator for interpretation_environment_merger outputs that sums their costs and caches 
/// the stored sums
struct interpretation_environment_coster {
	using Element = interpretation_environment_merger::OutType;

	/// Reports cached cost of an element
	const Cost& get( const Element& x ) {
		auto it = cache.find( &x );
		if ( it == cache.end() ) {
			Cost k;
			for ( const Interpretation* i : x.second ) { k += i->cost; }
			it = cache.emplace_hint( it, &x, k );
		}
		return it->second;
	}

	bool operator() ( const Element& a, const Element& b ) { return get( a ) < get( b ); }
private:
	std::unordered_map<Element*, Cost> cache;
};

/// Checks the type assertions of a function call.
/// Takes a resolver, the parameters needed to build the call (particularly the list of type 
/// assertions), and the existing cost and environment. Returns nullptr for no assertion resolution, 
/// a CallExpr interpretation if uniquely resolved, and an ambiguous interpretation otherwise
Interpretation* resolveAssertions( Resolver& resolver, const FuncDecl* func, List<TypedExpr>&& args, 
                                   unique_ptr<TypeBinding>&& bindings, Cost& cost, 
								   cow_ptr<Environment>& env ) {
	if ( ! bindings || bindings->assertions().empty() )
		return new Interpretation{ new CallExpr{ func, move(args), move(bindings) }, 
		                           copy(cost), copy(env) };

	// replaces assertion parameter/return types according to original environment
	BindingEnvSubMutator replaceLocals{ *bindings, env };

	// get possible satisfying assignments for every assertion
	std::vector<InterpretationList> allSatisfying;
	allSatisfying.reserve( bindings->assertions().size() );
	for ( const auto& asn : bindings->assertions() ) {
		// Generate FuncExpr for assertion
		const FuncDecl* asnDecl = asn.first;
		List<Expr> asnArgs;
		asnArgs.reserve( asnDecl->params().size() );
		for ( const Type* pType : asnDecl->params() ) {
			asnArgs.push_back( new VarExpr( replaceLocals( pType ) ) );
		}
		const FuncExpr* asnFunc = new FuncExpr{ asnDecl->name(), move(asnArgs) };
		const Type* asnReturn = replaceLocals( asnDecl->returns() );

		// back up environment and attempt to resolve assertion
		InterpretationList satisfying = resolver.resolveWithType( asnFunc, asnReturn, env );

		// quit early if no resolution; otherwise add to list of resolutions
		if ( satisfying.empty() ) return nullptr;
		allSatisfying.push_back( move(satisfying) );
	}

	// merge mutually-compatible environments together
	auto minCost = filter_combos( allSatisfying, interpretation_environment_merger{} );

	if ( minCost.empty() ) return nullptr;  // no mutually-compatible satisfying assertions

	interpretation_environment_coster costs;
	auto minPos = sort_mins( minCost.begin(), minCost.end(), costs );
	
	if ( minPos == minCost.begin() ) {  // unique min-cost assertion satisfaction
		// record matching bindings
		TypeBinding::AssertionList::const_iterator it = bindings->assertions().begin();
		for ( const Interpretation* i : minPos->second ) { bindings->bind_assertion( it++, i ); }
		
		return new Interpretation{ new CallExpr{ func, move(args), move(bindings) },
		                           move(costs.get( *minPos )), move(minPos->first) };
	} else {  // ambigious min-cost assertion satisfaction
		List<TypedExpr> alts;
		auto minIt = minCost.begin();
		while (true) {
			// record matching bindings for each min-cost alternative
			unique_ptr<TypeBinding> altBindings = copy(bindings);
			TypeBinding::AssertionList::const_iterator it = altBindings->assertions().begin();
			for ( const Interpretation* i : minIt->second ) {
				altBindings->bind_assertion( it++, i );
			}
			alts.push_back( new CallExpr{ func, copy(args), move(altBindings) } );

			if ( minIt == minPos ) break;
			++minIt;
		}

		// wrap alternatives in ambiguous alternative
		return new Interpretation{ new AmbiguousExpr{ new FuncExpr{ func->name(), move(args) }, 
		                                              func->returns(), move(alts) },
		                           move(costs.get( *minPos )), copy(env) };
	}
}

/// Checks the validity of all the type assertions in a resolved expression
class ExprAssertionsUnresolvable : public TypedExprMutator<ExprAssertionsUnresolvable> {
	Resolver& resolver;
	Cost& cost;
	cow_ptr<Environment>& env;

public:
	ExprAssertionsUnresolvable( Resolver& resolver, Cost& cost, cow_ptr<Environment>& env ) 
		: resolver(resolver), cost(cost), env(env) {}
	
	using TypedExprMutator<ExprAssertionsUnresolvable>::visit;

	bool visit( const CallExpr* e, const TypedExpr*& r ) {
		option<List<TypedExpr>> newArgs;
		mutateAll( this, e->args(), newArgs );
		if ( newArgs ) {
			// trim expressions that lose any args
			if ( newArgs->size() < e->args().size() ) {
				r = nullptr;
				return true;
			}

			// check assertions and generate new CallExpr if they match
			TypeBinding* forall = e->forall() ? new TypeBinding{ *e->forall() } : nullptr;
			if ( assertionsUnresolvable( resolver, forall, cost, env ) ) {
				delete forall;
				r = nullptr;
				return true;
			}
			r = new CallExpr{ e->func(), *move(newArgs), unique_ptr<TypeBinding>{ forall } };
		} else {
			if ( assertionsUnresolvable( resolver, as_non_const(e->forall()), cost, env ) ) {
				r = nullptr;
			}
		}
		return true;
	}

	bool visit( const AmbiguousExpr* e, const TypedExpr*& r ) {
		Cost cost_bak = cost;
		Cost cost_min = Cost::max();
		unsigned n = e->alts().size();
		bool unchanged = true;
		
		// find all min-cost resolvable alternatives
		List<TypedExpr> min_alts;
		for ( unsigned i = 0; i < n; ++i ) {
			const TypedExpr* ti = e->alts()[i];

			visit( ti, ti );
			if ( ti != e->alts()[i] ) { unchanged = false; }

			if ( ! ti ) {
				// trim unresolvable alternatives
				cost = cost_bak;
				continue;
			}

			// keep min-cost alternatives
			if ( cost < cost_min ) {
				cost_min = cost;
				min_alts.clear();
				min_alts.push_back( ti );
			} else if ( cost == cost_min ) {
				min_alts.push_back( ti );
			}

			cost = cost_bak; // reset cost for next alternative
		}

		if ( min_alts.empty() ) {
			// no matches
			r = nullptr;
			return true;
		}

		cost = cost_min;
		
		// make no change if no change needed
		if ( unchanged ) return true;
		
		// unique min disambiguates expression
		if ( min_alts.size() == 1 ) {
			r = min_alts.front();
			return true;
		}

		// otherwise new ambiguous expression
		r = new AmbiguousExpr{ e->expr(), e->type(), move(min_alts) };
		return true;
	}

	/// Nulls call expression and returns true if unresolvable; false otherwise
	bool operator() ( CallExpr* e ) {
		const TypedExpr* tp = e;
		visit( e, tp );
		return tp == nullptr;
	}
};
