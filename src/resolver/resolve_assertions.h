#pragma once

#include <unordered_map>
#include <utility>
#include <vector>

#include "cost.h"
#include "env.h"
#include "env_substitutor.h"
#include "resolver.h"

#include "ast/expr.h"
#include "ast/typed_expr_mutator.h"
#include "data/cast.h"
#include "data/list.h"
#include "data/mem.h"
#include "lists/filter.h"
#include "lists/sort_min.h"

/// Ensures that all assertions in an expression tree are resolved.
/// May disambiguate ambiguous expressions based on assertion cost or resolvability.
class AssertionResolver : public TypedExprMutator<AssertionResolver> {
	using DeferList = std::vector<InterpretationList>;

	/// Combo iterator that combines interpretations into an interpretation list, merging 
	/// their environments. Rejects an appended interpretation if the environments cannot 
	/// be merged.
	class interpretation_env_merger {
		/// Current list of merged interpretations
		InterpretationList crnt;
		/// Stack of environments, to support backtracking
		std::vector<const Env*> envs;
	public:
		/// Outputs a pair consisting of the merged environment and the list of interpretations
		using OutType = std::pair<const Env*, InterpretationList>;

		interpretation_env_merger( const Env* env ) : crnt{}, envs{} {
			envs.push_back( env );
		}

		ComboResult append( const Interpretation* i ) {
			Env* env = Env::from( envs.back() );
			if ( ! merge( env, i->env ) ) return ComboResult::REJECT_THIS;
			crnt.push_back( i );
			envs.push_back( getNonempty( env ) );
			return ComboResult::ACCEPT;
		}

		void backtrack() {
			crnt.pop_back();
			envs.pop_back();
		}

		OutType finalize() { return { envs.back(), crnt }; }
	};

	/// Comparator for interpretation_env_merger outputs that sums their costs and caches 
	/// the stored sums
	class interpretation_env_coster {
	public:
		using Element = interpretation_env_merger::OutType;
	private:
		using Memo = std::unordered_map<const Element*, Cost>;
		Memo cache;  ///< Cache of element costs
	public:
		/// Reports cached cost of an element
		const Cost& get( const Element& x ) {
			Memo::const_iterator it = cache.find( &x );
			if ( it == cache.end() ) {
				Cost k;
				for ( const Interpretation* i : x.second ) { k += i->cost; }
				it = cache.emplace_hint( it, &x, k );
			}
			return it->second;
		}

		bool operator() ( const Element& a, const Element& b ) {
			return get( a ) < get( b );
		}
	};

	Resolver& resolver;        ///< Resolver to perform searches.
	Cost& cost;                ///< Cost of accumulated bindings.
	Env*& env;                 ///< Environment to bind types and assertions into.
	List<FuncDecl> deferIds;   ///< Assertions that cannot be bound at their own call
	DeferList deferred;        ///< Available interpretations of deferred assertions
	List<FuncDecl> openAssns;  ///< Sub-assertions of bound assertions
	unsigned recursion_depth;  ///< Depth of sub-assertions being bound
	bool has_ambiguous;        ///< Expression tree has ambiguous subexpressions; fail fast

	/// Resolves an assertion, returning false if no satisfying assertion, binding it if one, 
	/// adding to the defer-list if more than one.
	bool resolveAssertion( const FuncDecl* asn ) {
		// generate FuncExpr for assertion
		List<Expr> asnArgs;
		asnArgs.reserve( asn->params().size() );
		for ( const Type* pType : asn->params() ) {
			asnArgs.push_back( new VarExpr( pType ) );
		}
		const FuncExpr* asnExpr = new FuncExpr{ asn->name(), move(asnArgs) };

		// attempt to resolve assertion
		InterpretationList satisfying = resolver.resolveWithType( asnExpr, asn->returns(), env );

		switch ( satisfying.size() ) {
			case 0: { // no satisfying assertions: return failure
				return false;
			} case 1: { // unique satisfying assertion: add to environment
				const Interpretation* s = satisfying.front();
				if ( bindRecursive( asn, s->expr ) ) {
					merge( env, s->env );
					return true;
				} else {
					return false;
				}
			} default: { // multiple satisfying assertions: defer evaluation
				deferIds.push_back( asn );
				deferred.push_back( move(satisfying) );
				return true;
			}
		}
	}

	/// Binds an assertion to its resolved expression, adding recursive assertions to the open set. 
	/// Returns false if resolved expression is ambiguous or contains assertions that would 
	/// exceed maximum recursion depth, true otherwise.
	bool bindRecursive( const FuncDecl* asn, const TypedExpr* rexpr ) {
		// reject any assertion binding that isn't a call expression
		const CallExpr* cexpr = as_safe<CallExpr>(rexpr);
		if ( ! cexpr ) return false;
		
		// add recursive assertions to open set
		if ( const Forall* rforall = cexpr->forall() ) {
			const List<FuncDecl>& rassns = rforall->assertions();
			
			// reject if hit max recursion depth
			if ( recursion_depth >= resolver.max_recursive_assertions && ! rassns.empty() )
				return false;
			
			// add open assertions and variables
			openAssns.insert( openAssns.end(), rassns.begin(), rassns.end() );
			for ( const PolyType* v : rforall->variables() ) { insertVar( env, v ); }
		}

		// bind assertion
		bindAssertion( env, asn, rexpr );
		return true;
	}

public:
	AssertionResolver( Resolver& resolver, Cost& cost, Env*& env, unsigned depth = 1 )
		: resolver(resolver), cost(cost), env(env), deferIds(), deferred(), openAssns(), 
		  recursion_depth(depth), has_ambiguous(false) {}
	
	using TypedExprMutator<AssertionResolver>::visit;
	using TypedExprMutator<AssertionResolver>::operator();

	bool visit( const CallExpr* e, const TypedExpr*& r ) {
		// recurse on children first
		option<List<TypedExpr>> newArgs;
		if ( ! mutateAll( this, e->args(), newArgs ) ) {
			r = nullptr;
			return false;
		}

		// rebuild node if appropriate
		const CallExpr* ee = e;  // use ee instead of e after this block
		if ( newArgs ) {
			// trim expressions that lose any args
			if ( newArgs->size() < e->args().size() ) {
				r = nullptr;
				return false;
			}

			// rebind to new forall clause if necessary
			// TODO should maybe run a ForallSubstitutor over the environment here...
			ee = new CallExpr{ e->func(), *move(newArgs), Forall::from( e->forall() ) };
		}

		// exit early if no assertions on this node
		if ( ! ee->forall() ) {
			r = ee;
			return true;
		}
		
		// bind assertions
		for ( const FuncDecl* asn : ee->forall()->assertions() ) {
			if ( ! resolveAssertion( asn ) ) {
				r = nullptr;
				return false;
			}
		}

		// ensure all type variables exist in the environment
		for ( const PolyType* v : ee->forall()->variables() ) { insertVar( env, v ); }
		
		r = ee;
		return true;
	}


	bool visit( const AmbiguousExpr* e, const TypedExpr*& r ) {
		// TODO I'm a little worried that this won't properly defer disambiguation of 
		// some ambiguous alternatives. However, making it a separate, deferred pass 
		// might mutate the Forall in containing CallExpr nodes without updating the 
		// environment.

		// find all min-cost resolvable alternatives
		bool changed = false;
		List<Interpretation> min_alts;
		bool local_ambiguous = false;
		for ( const Interpretation* alt : e->alts() ) {
			const TypedExpr* alt_expr = alt->expr;
			const TypedExpr* alt_bak = alt_expr;
			Cost alt_cost = Cost::zero();
			Env* alt_env = Env::from( env );
			// skip alternatives that can't be resolved in the current environment
			if ( ! merge( alt_env, alt->env ) ) {
				changed = true;
				continue;
			}
			// re-resolve alternative assertions under current environment
			AssertionResolver alt_resolver{ resolver, alt_cost, alt_env, recursion_depth };
			alt_resolver.mutate( alt_expr );
			if ( alt_expr != alt_bak ) { changed = true; }

			// skip un-resolvable alternatives
			if ( ! alt_expr ) continue;

			// keep min-cost alternatives
			Interpretation* new_alt = 
				new Interpretation{ alt_expr, alt_env, move(alt_cost), copy(alt->argCost) };
			if ( min_alts.empty() ) {
				min_alts.push_back( new_alt );
				local_ambiguous = alt_resolver.has_ambiguous;
			} else switch ( compare( *new_alt, *min_alts[0] ) ) {
			case lt:
				min_alts.clear();
				local_ambiguous = false;
				// fallthrough
			case eq:
				min_alts.push_back( new_alt );
				local_ambiguous |= alt_resolver.has_ambiguous;
				break;
			case gt:
				break;
			}
		}

		if ( min_alts.empty() ) {
			// no matches
			r = nullptr;
			return true;
		}

		// unique min disambiguates expression
		if ( min_alts.size() == 1 ) {
			env = Env::from( min_alts.front()->env );
			r = min_alts.front()->expr;
			has_ambiguous |= local_ambiguous;
			return true;
		}

		// new ambiguous expression if changed
		if ( changed ) { r = new AmbiguousExpr{ e->expr(), e->type(), move(min_alts) }; }
		has_ambiguous = true;
		return true;
	}

	const TypedExpr* mutate( const TypedExpr*& r ) {
		(*this)( r, r );

		// end early if ambiguous or failure, no need to resolve assertions
		if ( has_ambiguous || r == nullptr ) return r;

		do {
			// attempt to disambiguate deferred assertion matches with additional information
			if ( ! deferred.empty() ) {
				auto compatible = 
					filter_combos<const Interpretation*>( deferred, interpretation_env_merger{ env } );
				if ( compatible.empty() ) return r = nullptr; // no mutually-compatible assertions

				// sort deferred assertion matches by cost
				interpretation_env_coster costs;
				auto minPos = sort_mins( compatible.begin(), compatible.end(), costs );
				if ( minPos == compatible.begin() ) {  // unique min-cost
					env = Env::from( minPos->first );
					for ( unsigned i = 0; i < deferIds.size(); ++i ) {
						// fail if cannot bind assertion
						if ( ! bindRecursive( deferIds[i], minPos->second[i]->expr ) ) {
							return r = nullptr;
						}
					}
				} else {  // ambiguous min-cost assertion matches
					List<Interpretation> alts;
					Cost alt_cost = costs.get( *minPos );
					auto it = compatible.begin();
					while (true) {
						// build an interpretation for each compatible min-cost assertion binding
						Env* alt_env = Env::from( it->first );
						for ( unsigned i = 0; i < deferIds.size(); ++i ) {
							bindAssertion( alt_env, deferIds[i], it->second[i]->expr );
						}
						alts.push_back( new Interpretation{ r, alt_env, copy(alt_cost) } );
						if ( it == minPos ) break; else ++it;
					}
					return r = new AmbiguousExpr{ r, r->type(), move(alts) };
				}

				deferred.clear();
			}

			if ( openAssns.empty() ) break;
			List<FuncDecl> toSolve{};
			toSolve.swap( openAssns );

			// fail if assertions too deeply nested
			++recursion_depth;
			if ( recursion_depth > resolver.max_recursive_assertions ) return r = nullptr;

			// bind recursive assertions
			for ( const FuncDecl* asn : toSolve ) {
				if ( ! resolveAssertion( asn ) ) return r = nullptr;
			}

		} while ( ! openAssns.empty() );

		return r;
	}
};

/// Resolves all assertions contained recursively in call, accumulating conversion cost 
/// and type/assertion bindings in cost and env, respectively. Returns whether all 
/// assertions can be consistently bound at a unique minimum cost; may mutate call to 
/// disambiguate ambiguous expressions.
bool resolveAssertions( Resolver& resolver, const TypedExpr*& call, Cost& cost, Env*& env ) {
	AssertionResolver assnResolver{ resolver, cost, env };
	return assnResolver.mutate( call ) != nullptr;
}

/// Resolves assertions over const Env
inline bool resolveAssertions( Resolver& resolver, const TypedExpr*& call, Cost& cost, 
		const Env*& env) {
	Env* mEnv = Env::from( env );
	if ( ! resolveAssertions( resolver, call, cost, mEnv ) ) return false;
	env = getNonempty( mEnv );
	return true;
}
