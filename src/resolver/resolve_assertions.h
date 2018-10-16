#pragma once

#include <algorithm>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "cost.h"
#include "env.h"
#include "env_substitutor.h"
#include "resolver.h"

#include "ast/decl_visitor.h"
#include "ast/expr.h"
#include "ast/type_visitor.h"
#include "ast/typed_expr_mutator.h"
#include "data/cast.h"
#include "data/debug.h"
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
		std::vector<Env> envs;
	public:
		/// Outputs a pair consisting of the merged environment and the list of interpretations
		using OutType = std::pair<Env, InterpretationList>;

		interpretation_env_merger( const Env& env ) : crnt{}, envs{} {
			envs.push_back( env );
		}

		ComboResult append( const Interpretation* i ) {
			Env env = envs.back();
			if ( i->env && ! env.merge( i->env ) ) return ComboResult::REJECT_THIS;
			crnt.push_back( i );
			envs.push_back( env );
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

#ifdef RP_RES_TEC
	/// Cached assertion items
	struct AssnCacheItem {
		InterpretationList satisfying;  ///< List of satisfying expressions
		List<Decl>         deferIds;    ///< Deferred assertions which resolve to this cache item

		/// No satisfying assertions
		AssnCacheItem() = default;
		/// Unambiguous satisfying assertion
		AssnCacheItem( const Interpretation* i ) : satisfying{ i }, deferIds{} {}
		/// Ambiguous satisfying assertions for `d`
		AssnCacheItem( InterpretationList&& is, const Decl* d )
			: satisfying( move(is) ), deferIds{ d } {}
	};

	using AssnCache = std::unordered_map<std::string, AssnCacheItem>;

	/// Wrapper to make an AssnCache item look like a nested vector
	struct DeferItem {
		using size_type = InterpretationList::size_type;

		std::string key;         ///< Key for this deferred item
		const AssnCache* cache;  ///< Cache this key indexes into

		DeferItem( const std::string& k, const AssnCache& c ) : key(k), cache(&c) {}

		// simulate vector interface enough for filter
		size_type size() const { return cache->at(key).satisfying.size(); }
		
		bool empty() const { return cache->at(key).satisfying.empty(); }

		const Interpretation* operator[] ( size_type i ) const {
			return cache->at(key).satisfying[i];
		}

		// sortable by key
		// TODO look into optimizing combination process with other sort orders
		bool operator< ( const DeferItem& o ) const { return key < o.key; }

		bool operator== ( const DeferItem& o ) const { return key == o.key; }
	};
#endif

	Resolver& resolver;        ///< Resolver to perform searches.
	Env& env;                  ///< Environment to bind types and assertions into.
#ifdef RP_RES_TEC
	/// Map of environment-normalized assertion manglenames to satisfying expressions
	AssnCache assnCache;
	/// `assnCache` keys that have more than one satisfying assertion
	std::vector<DeferItem> deferKeys;
#else
	/// Assertions that cannot be bound at their own call site
	List<Decl> deferIds;
	DeferList deferred;        ///< Available interpretations of deferred assertions
#endif
	List<Decl> openAssns;      ///< Sub-assertions of bound assertions
	unsigned recursion_depth;  ///< Depth of sub-assertions being bound
	bool has_ambiguous;        ///< Expression tree has ambiguous subexpressions; fail fast
#if defined RP_RES_IMM
	bool do_recurse;           ///< Should the visitor recurse over the whole tree?
#endif

	/// Assertion expression and the type to resolve it to
	struct AssertionInstance {
		const Expr* expr;       ///< Expression to resolve
		const Type* target;     ///< Type to resolve to
	#ifdef RP_RES_TEC
		std::stringstream key;  ///< Caching key for resolution de-duplication

		AssertionInstance() : expr(nullptr), target(nullptr), key() {}
	#else
		AssertionInstance() : expr(nullptr), target(nullptr) {}
	#endif
	};

	/// Derives an assertion problem instance from a declaration
	class AssertionInstantiator : public DeclVisitor<AssertionInstantiator, AssertionInstance> 
#ifdef RP_RES_TEC	
		, TypeVisitor<AssertionInstantiator, std::stringstream>
#endif
	{

#ifdef RP_RES_TEC
		const Env& env;  ///< Environment to do mangle-substitution in)
#endif

	public:
		using DeclVisitor<AssertionInstantiator, AssertionInstance>::visit;
		using DeclVisitor<AssertionInstantiator, AssertionInstance>::operator();

#ifdef RP_RES_TEC
		using TypeVisitor<AssertionInstantiator, std::stringstream>::visit;

	private:
		/// print all types in a (non-empty) list, space separated
		void visit_all( const List<Type>& ts, std::stringstream& out ) {
			auto it = ts.begin();
			visit( *it, out );
			while ( ++it != ts.end() ) {
				out << " ";
				visit( *it, out );
			}
		}

	public:
		bool visit( const ConcType* t, std::stringstream& out ) {
			out << *t;
			return true;
		}

		bool visit( const NamedType* t, std::stringstream& out ) {
			out << t->name();
			if ( ! t->params().empty() ) {
				out << "<";
				visit_all( t->params(), out );
				out << ">";
			}
			return true;
		}

		bool visit( const PolyType* t, std::stringstream& out ) {
			// fallback to t if unknown type
			ClassRef r = env.findRef( t );
			if ( ! r ) { out << *t; return true; }
			// print class root if unbound class
			const Type* bound = r.get_bound();
			if ( ! bound ) { out << *r.get_root(); return true; }
			// print bound type, tagged as substitution
			out << "%";
			visit( bound, out );
			return true;
		}

		bool visit( const VoidType* t, std::stringstream& out ) {
			out << *t;
			return true;
		}

		bool visit( const TupleType* t, std::stringstream& out ) {
			// class invariant that tuple type is non-empty
			visit_all( t->types(), out );
			return true;
		}

		bool visit( const FuncType* t, std::stringstream& out ) {
			out << "[";
			if ( t->returns()->size() > 0 ) {
				out << " ";
				visit( t->returns(), out );
			}
			out << " :";
			if ( ! t->params().empty() ) {
				out << " ";
				visit_all( t->params(), out );
			}
			out << " ]";
			return true;
		}

		AssertionInstantiator( const Env& env ) : env{env} {}
#else
		AssertionInstantiator( const Env& ) {}
#endif
		
		bool visit( const FuncDecl* d, AssertionInstance& r ) {
			List<Expr> args;
			args.reserve( d->params().size() );
#ifdef RP_RES_TEC
			visit( d->returns(), r.key );
			r.key << " " << d->name();
#endif
			for ( const Type* pType : d->params() ) {
				args.push_back( new ValExpr{ pType } );
#ifdef RP_RES_TEC
				r.key << " ";
				visit( pType, r.key );
#endif
			}

			r.expr = new FuncExpr{ d->name(), move(args) };
			r.target = d->returns();
			return true;
		}

		bool visit( const VarDecl* d, AssertionInstance& r ) {
			r.expr = new NameExpr{ d->name() };
			r.target = d->type();
#ifdef RP_RES_TEC
			visit( d->type(), r.key );
			r.key << " &" << d->name();
#endif

			return true;
		}
	};

	/// Resolves an assertion, returning false if no satisfying assertion, binding it if one, 
	/// adding to the defer-list if more than one.
	bool resolveAssertion( const Decl* asn ) {
		// generate expression for assertion
		AssertionInstance inst = AssertionInstantiator{ env }( asn );

#ifdef RP_RES_TEC
		// look in cache for key
		std::string asnKey = inst.key.str();
		auto it = assnCache.find( asnKey );

		// attempt to resolve assertion if this is the first time seen
		if ( it == assnCache.end() ) {
			InterpretationList satisfying = resolver.resolveWithType( inst.expr, inst.target, env );

			switch ( satisfying.size() ) {
				case 0: {  // mark no satisfying, fail
					assnCache.emplace_hint( it, asnKey, AssnCacheItem{} );
					return false;
				} case 1: {  // unique satisfying assertion, add to environment
					const Interpretation* s = satisfying.front();
					if ( bindRecursive( asn, s->expr ) ) {  // succeed and mark in cache
						if ( s->env ) { env.merge( s->env ); }
						
						assnCache.emplace_hint( it, asnKey, AssnCacheItem{ s } );
						return true;
					} else {  // can't bind recursively, fail
						assnCache.emplace_hint( it, asnKey, AssnCacheItem{} );
						return false;
					}
				} default: {  // ambiguous satisfying assertions, defer evaluation
					assnCache.emplace_hint( it, asnKey, 
						// correct for ambiguous environments not being descended from root
						AssnCacheItem{ splitAmbiguous( move(satisfying) ), asn } );
					deferKeys.emplace_back( asnKey, assnCache );
					return true;
				}
			}
		}

		// repeat earlier resolution
		switch ( it->second.satisfying.size() ) {
			case 0:   // fail on already failed assertions
				return false;
			case 1:   // already recursively bound, just need to add invididual assertion
				env.bindAssertion( asn, it->second.satisfying[0]->expr );
				return true;
			default:  // already ambiguous, ensure that it is deferred
				it->second.deferIds.emplace_back( asn );
				deferKeys.emplace_back( asnKey, assnCache );
				return true;
		}
#else
		// attempt to resolve assertion
		InterpretationList satisfying = resolver.resolveWithType( inst.expr, inst.target, env );

		switch ( satisfying.size() ) {
			case 0: { // no satisfying assertions: return failure
				return false;
			} case 1: { // unique satisfying assertion: add to environment
				const Interpretation* s = satisfying.front();
				if ( bindRecursive( asn, s->expr ) ) {
					if ( s->env ) env.merge( s->env );
					return true;
				} else {
					return false;
				}
			} default: { // multiple satisfying assertions: defer evaluation
				deferIds.push_back( asn );
				// ambiguous interpretations have environments that are not versions of the correct 
				// environment
				deferred.push_back( splitAmbiguous( move(satisfying) ) );
				return true;
			}
		}
#endif
	}

	/// Binds an assertion to its resolved expression, adding recursive assertions to the open set. 
	/// Returns false if resolved expression is ambiguous or contains assertions that would 
	/// exceed maximum recursion depth, true otherwise.
	bool bindRecursive( const Decl* asn, const TypedExpr* rexpr ) {
		auto aid = typeof(asn);
		if ( aid == typeof<FuncDecl>() ) {
			// reject any function assertion binding that isn't a call expression
			const CallExpr* cexpr = as_safe<CallExpr>(rexpr);
			if ( ! cexpr ) return false;
			
			// add recursive assertions to open set
			if ( const Forall* rforall = cexpr->forall() ) {
				const List<Decl>& rassns = rforall->assertions();
				
				// reject if hit max recursion depth
				if ( recursion_depth >= resolver.max_recursive_assertions && ! rassns.empty() )
					return false;
				
				// add open assertions and variables
				openAssns.insert( openAssns.end(), rassns.begin(), rassns.end() );
				for ( const PolyType* v : rforall->variables() ) { env.insertVar( v ); }
			}
		} else if ( aid == typeof<VarDecl>() ) {
			// reject any variable assertion binding that isn't a VarExpr
			if ( ! is<VarExpr>(rexpr) ) return false;
		} else unreachable("invalid declaration");

		// bind assertion
		env.bindAssertion( asn, rexpr );
		return true;
	}

	AssertionResolver( AssertionResolver& o, Env& env )
		: resolver(o.resolver), env(env)
#if defined RP_RES_TEC
		  , assnCache(), deferKeys()
#else
		  , deferIds(), deferred()
#endif
		  , openAssns(), recursion_depth(o.recursion_depth), has_ambiguous(false) 
#if defined RP_RES_IMM
		  , do_recurse(o.do_recurse)
#endif
		  {}

public:
	AssertionResolver( Resolver& resolver, Env& env 
#if defined RP_RES_IMM
		, bool recurse = false 
#endif
	) : resolver(resolver), env(env)
#if defined RP_RES_TEC
		, assnCache(), deferKeys()
#else
		, deferIds(), deferred()
#endif
	    , openAssns(), recursion_depth(1), has_ambiguous(false)
#if defined RP_RES_IMM
	    ,  do_recurse(recurse)
#endif
	{}

	using TypedExprMutator<AssertionResolver>::visit;
	using TypedExprMutator<AssertionResolver>::operator();

	bool visit( const CallExpr* e, const TypedExpr*& r ) {		
#if defined RP_RES_IMM
		if ( do_recurse ) {
#endif
			// recurse on children first
			option<List<TypedExpr>> newArgs;
			if ( ! mutateAll( this, e->args(), newArgs ) ) {
				r = nullptr;
				return false;
			}

			// rebuild node if appropriate
			if ( newArgs ) {
				// trim expressions that lose any args
				if ( newArgs->size() < e->args().size() ) {
					r = nullptr;
					return false;
				}

				// rebind to new forall clause if necessary
				// TODO should maybe run a ForallSubstitutor over the environment here...
				e = new CallExpr{ e->func(), *move(newArgs), Forall::from( e->forall() ) };
			}
#if defined RP_RES_IMM
			// skip re-resolving assertions if recursing back over the tree in immediate mode
			r = e;
			return true;
		}
#endif
		// exit early if no assertions on this node
		if ( ! e->forall() ) {
			r = e;
			return true;
		}
		
		// bind assertions
		for ( const Decl* asn : e->forall()->assertions() ) {
			if ( ! resolveAssertion( asn ) ) {
				r = nullptr;
				return false;
			}
		}

		// ensure all type variables exist in the environment
		for ( const PolyType* v : e->forall()->variables() ) { env.insertVar( v ); }
		
		r = e;
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
			Env alt_env = env;
			// skip alternatives that can't be resolved in the current environment
			if ( alt->env && ! alt_env.merge( alt->env ) ) {
				changed = true;
				continue;
			}
			// re-resolve alternative assertions under current environment
			AssertionResolver alt_resolver{ *this, alt_env };
			alt_resolver.mutate( alt_expr );
			if ( alt_expr != alt_bak ) { changed = true; }

			// skip un-resolvable alternatives
			if ( ! alt_expr ) continue;

			// keep min-cost alternatives
			Interpretation* new_alt = 
				new Interpretation{ alt_expr, alt_env, move(alt->cost), copy(alt->argCost) };
			if ( min_alts.empty() ) {
				min_alts.push_back( new_alt );
				local_ambiguous = alt_resolver.has_ambiguous;
			} else switch ( compare( *new_alt, *min_alts[0] ) ) {
			case lt:
				min_alts.clear();
				min_alts.push_back( new_alt );
				local_ambiguous = alt_resolver.has_ambiguous;
				break;
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
			env = min_alts.front()->env;
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

#if defined RP_RES_IMM
		// end early if failure, no need to resolve assertions
		if ( r == nullptr ) return r;
#else
		// end early if ambiguous or failure, no need to resolve assertions
		if ( has_ambiguous || r == nullptr ) return r;
#endif

		do {
			// attempt to disambiguate deferred assertion matches with additional information
#if defined RP_RES_TEC
			if ( ! deferKeys.empty() ) {
				// trim deferred keys down to relevant list
				std::sort( deferKeys.begin(), deferKeys.end() );
				auto last = std::unique( deferKeys.begin(), deferKeys.end() );
				deferKeys.erase( last, deferKeys.end() );

				auto compatible = filter_combos( deferKeys, interpretation_env_merger{ env } );
#else
			if ( ! deferred.empty() ) {
				auto compatible = filter_combos( deferred, interpretation_env_merger{ env } );
#endif
				if ( compatible.empty() ) return r = nullptr; // no mutually-compatible assertions

				interpretation_env_coster costs;
#if defined RP_RES_IMM
				if ( compatible.size() == 1 ) {  // unique set of compatible matches
					env = compatible.front().first;
					for ( unsigned i = 0; i < deferIds.size(); ++i ) {
						// fail if cannot bind assertion
						if ( ! bindRecursive( deferIds[i], compatible.front().second[i]->expr ) ) {
							return r = nullptr;
						}
					}
				} else {
					// keep all mutually compatible assertion matches
					List<Interpretation> alts;
					auto it = compatible.begin();
					do {
						// build an interpretation from each compatible assertion binding
						Env alt_env = it->first;
						Cost alt_cost;
						for ( unsigned i = 0; i < deferIds.size(); ++i ) {
							alt_env.bindAssertion( deferIds[i], it->second[i]->expr );
							alt_cost += it->second[i]->cost;
						}
						alts.push_back( new Interpretation{ r, move(alt_env), move(alt_cost) } );
					} while ( ++it != compatible.end() );
					return r = new AmbiguousExpr{ r, r->type(), move(alts) };
				}
#else
				// sort deferred assertion matches by cost
				auto minPos = sort_mins( compatible.begin(), compatible.end(), costs );
				if ( minPos == compatible.begin() ) {  // unique min-cost
					env = minPos->first;
#ifdef RP_RES_TEC
					for ( unsigned i = 0; i < deferKeys.size(); ++i ) {
						AssnCacheItem& cache = assnCache[ deferKeys[i].key ];
						// fail if cannot bind assertion
						if ( ! bindRecursive( cache.deferIds[0], minPos->second[i]->expr ) ) {
							return r = nullptr;
						}
						// bind other assertions to same resolution
						for ( unsigned j = 1; j < cache.deferIds.size(); ++j ) {
							env.bindAssertion( cache.deferIds[j], minPos->second[i]->expr );
						}
						// done with these deferred IDs
						cache.satisfying = InterpretationList{ minPos->second[i] };
						cache.deferIds.clear();
					}
#else
					for ( unsigned i = 0; i < deferIds.size(); ++i ) {
						// fail if cannot bind assertion
						if ( ! bindRecursive( deferIds[i], minPos->second[i]->expr ) ) {
							return r = nullptr;
						}
					}
#endif
				} else {  // ambiguous min-cost assertion matches
					List<Interpretation> alts;
					Cost alt_cost = costs.get( *minPos );
					auto it = compatible.begin();
					while (true) {
						// build an interpretation for each compatible min-cost assertion binding
						Env alt_env = it->first;
#ifdef RP_RES_TEC
						for ( unsigned i = 0; i < deferKeys.size(); ++i ) {
							const AssnCacheItem& cache = assnCache[ deferKeys[i].key ];
							for ( unsigned j = 0; j < cache.deferIds.size(); ++j ) {
								alt_env.bindAssertion( cache.deferIds[j], it->second[i]->expr );
							}
						}
#else
						for ( unsigned i = 0; i < deferIds.size(); ++i ) {
							alt_env.bindAssertion( deferIds[i], it->second[i]->expr );
						}
#endif
						alts.push_back( new Interpretation{ r, move(alt_env), copy(alt_cost) } );
						if ( it == minPos ) break; else ++it;
					}
					return r = new AmbiguousExpr{ r, r->type(), move(alts) };
				}
#endif
#ifdef RP_RES_TEC
				deferKeys.clear();
#else
				deferIds.clear();
				deferred.clear();
#endif
			}

			if ( openAssns.empty() ) break;
			List<Decl> toSolve{};
			toSolve.swap( openAssns );

			// fail if assertions too deeply nested
			++recursion_depth;
			if ( recursion_depth > resolver.max_recursive_assertions ) return r = nullptr;

			// bind recursive assertions
			for ( const Decl* asn : toSolve ) {
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
inline bool resolveAssertions( Resolver& resolver, const TypedExpr*& call, Env& env ) {
	AssertionResolver assnResolver{ resolver, env };
	return assnResolver.mutate( call ) != nullptr;
}
