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
		std::vector<unique_ptr<Env>> envs;
	public:
		/// Outputs a pair consisting of the merged environment and the list of interpretations
		using OutType = std::pair<unique_ptr<Env>, InterpretationList>;

		interpretation_env_merger( const Env* env ) : crnt{}, envs{} {
			envs.push_back( Env::from( env ) );
		}

		ComboResult append( const Interpretation* i ) {
			unique_ptr<Env> env = Env::from( envs.back().get() );
			if ( ! merge( env, i->env.get() ) ) return ComboResult::REJECT_THIS;
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

	Resolver& resolver;       ///< Resolver to perform searches.
	Cost& cost;               ///< Cost of accumulated bindings.
	unique_ptr<Env>& env;     ///< Environment to bind types and assertions into.
	List<FuncDecl> deferIds;  ///< Assertions that cannot be bound at their own call
	DeferList deferred;       ///< Available interpretations of deferred assertions
public:
	AssertionResolver( Resolver& resolver, Cost& cost, unique_ptr<Env>& env )
		: resolver(resolver), cost(cost), env(env), deferIds(), deferred() {}
	
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
			unique_ptr<Forall> forall = Forall::from( e->forall() );
			// TODO should maybe run a ForallSubstitutor over the environment here...
			ee = new CallExpr{ e->func(), *move(newArgs), move(forall) };
		}

		// exit early if no assertions on this node
		if ( ! ee->forall() ) {
			r = ee;
			return true;
		}
		
		// bind assertions
		for ( const FuncDecl* asn : ee->forall()->assertions() ) {
			// generate FuncExpr for assertion
			/* EnvSubstitutor replaceLocals{ env.get() }; */
			List<Expr> asnArgs;
			asnArgs.reserve( asn->params().size() );
			for ( const Type* pType : asn->params() ) {
				asnArgs.push_back( new VarExpr( /*replaceLocals(*/ pType /*)*/ ) );
			}
			const FuncExpr* asnExpr = new FuncExpr{ asn->name(), move(asnArgs) };
			const Type* asnRet = /*replaceLocals(*/ asn->returns() /*)*/;

			// attempt to resolve assertion
			// TODO this visitor should probably be integrated into the resolver; that 
			//      way the defer-list of assertions can be iterated over at the top 
			//      level and overly-deep recursive invocations can be caught.
			InterpretationList satisfying = 
				resolver.resolveWithType( asnExpr, asnRet, env.get() );

			switch ( satisfying.size() ) {
				case 0: { // no satisfying assertions: return failure
					r = nullptr;
					return false;
				} case 1: { // unique satisfying assertion: add to environment
					const Interpretation* s = satisfying.front();
					merge( env, s->env.get() );
					cost += s->cost;
					bindAssertion( env, asn, s->expr );
					break;
				} default: { // multiple satisfying assertions: defer evaluation
					deferIds.push_back( asn );
					deferred.emplace_back( move(satisfying) );
					break;
				}
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
		for ( const Interpretation* alt : e->alts() ) {
			const TypedExpr* alt_expr = alt->expr;
			const TypedExpr* alt_bak = alt_expr;
			Cost alt_cost = cost + alt->cost;
			unique_ptr<Env> alt_env = Env::from( env.get() );
			// skip alternatives that can't be resolved in the current environment
			if ( ! merge( alt_env, alt->env.get() ) ) {
				changed = true;
				continue;
			}
			// re-resolve alternative assertions under current environment
			AssertionResolver alt_resolver{ resolver, alt_cost, alt_env };
			alt_resolver.mutate( alt_expr );
			if ( alt_expr != alt_bak ) { changed = true; }

			// skip un-resolvable alternatives
			if ( ! alt_expr ) continue;

			// keep min-cost alternatives
			if ( min_alts.empty() || alt_cost < min_alts[0]->cost ) {
				min_alts.clear();
				min_alts.push_back( 
					new Interpretation{ alt_expr, move(alt_cost), move(alt_env) } );
			} else if ( alt_cost == min_alts[0]->cost ) {
				min_alts.push_back(
					new Interpretation{ alt_expr, move(alt_cost), move(alt_env) } );
			}
		}

		if ( min_alts.empty() ) {
			// no matches
			r = nullptr;
			return true;
		}

		cost = min_alts[0]->cost;

		// unique min disambiguates expression
		if ( min_alts.size() == 1 ) {
			merge( env, min_alts[0]->env.get() );
			r = min_alts.front()->expr;
			return true;
		}

		// new ambiguous expression if changed
		if ( changed ) { r = new AmbiguousExpr{ e->expr(), e->type(), move(min_alts) }; }
		return true;
	}

	const TypedExpr* mutate( const TypedExpr*& r ) {
		(*this)( r, r );

		// end early if no deferred assertions
		if ( deferred.empty() ) return r;

		// attempt to disambiguate deferred assertion matches with additional information
		auto compatible = 
			filter_combos<const Interpretation*>( 
				deferred, interpretation_env_merger{ env.get() } );
		if ( compatible.empty() ) return r = nullptr; // no mutually-compatible assertions

		// sort deferred assertion matches by cost
		interpretation_env_coster costs;
		auto minPos = sort_mins( compatible.begin(), compatible.end(), costs );
		if ( minPos == compatible.begin() ) {  // unique min-cost
			// NOTE env is a parent of minPos->first, so we can't just set 
			//      env = minPos->first, but need to merge in the child parts
			// TODO write flatten( env, parent ) -> new_env method for this case
			merge( env, minPos->first.get() );
			cost += costs.get( *minPos );
			for ( unsigned i = 0; i < deferIds.size(); ++i ) {
				bindAssertion( env, deferIds[i], minPos->second[i]->expr );
			}
		} else {  // ambiguous min-cost assertion matches
			List<Interpretation> alts;
			Cost alt_cost = cost + costs.get( *minPos );
			auto it = compatible.begin();
			do {
				// build an interpretation for each compatible min-cost assertion binding
				unique_ptr<Env> alt_env = move(it->first);
				for ( unsigned i = 0; i < deferIds.size(); ++i ) {
					bindAssertion( alt_env, deferIds[i], it->second[i]->expr );
				}
				alts.push_back( new Interpretation{ r, move(alt_cost), move(alt_env) } );
			} while ( it != minPos );
			r = new AmbiguousExpr{ r, r->type(), move(alts) };
		}

		return r;
	}
};

/// Resolves all assertions contained recursively in call, accumulating conversion cost 
/// and type/assertion bindings in cost and env, respectively. Returns whether all 
/// assertions can be consistently bound at a unique minimum cost; may mutate call to 
/// disambiguate ambiguous expressions.
bool resolveAssertions( Resolver& resolver, const TypedExpr*& call, Cost& cost, 
		unique_ptr<Env>& env ) {
	AssertionResolver assnResolver{ resolver, cost, env };
	return assnResolver.mutate( call ) != nullptr;
}
