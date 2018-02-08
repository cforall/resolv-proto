#include "resolver.h"

#include "env.h"
#include "interpretation.h"
#ifdef RP_RES_DEF
#include "resolve_assertions.h"
#endif

#include "ast/expr.h"
#include "ast/typed_expr_visitor.h"
#include "data/list.h"
#include "lists/sort_min.h"

/// Ensures that resolved expressions have all their type variables bound and are not 
/// ambiguous.
class ResolvedExprInvalid : public TypedExprVisitor<ResolvedExprInvalid, bool> {
	AmbiguousEffect& on_ambiguous;

public:
	ResolvedExprInvalid( AmbiguousEffect& on_ambiguous ) : on_ambiguous(on_ambiguous) {}
	
	using TypedExprVisitor<ResolvedExprInvalid, bool>::visit;

	bool visit( const AmbiguousExpr* e, bool& invalid ) {
		on_ambiguous( e->expr(), e->alts().begin(), e->alts().end() );
		invalid = true;
		return false;
	}
};

const Interpretation* Resolver::operator() ( const Expr* expr ) {
	id_src = 0;  // initialize type variable IDs
	new_generation();  // set up new GC generation
#ifdef RP_DIR_TD
	cached.clear();  // clear subexpression cache
#endif

	InterpretationList results = resolve( expr, Env::none(), Mode::top_level() );
	
	// return invalid interpretation on empty results
	if ( results.empty() ) {
		on_invalid( expr );
		collect_young();
		return Interpretation::make_invalid();
	}

#ifdef RP_RES_DEF
	// sort results by cost
	std::sort( results.begin(), results.end(), ByValueCompare<Interpretation>{} );
	
	// search for cheapest resolvable candidate
	InterpretationList candidates;
	for ( unsigned i = 0; i < results.size(); ++i ) {
		// break if already have cheaper candidate
		if ( ! candidates.empty() && *candidates.front() < *results[i] ) break;

		// check candidate assertion resolution
		const Interpretation& r = *results[i];
		const TypedExpr* rExpr = r.expr;
		Env* rEnv = Env::from( r.env );
		if ( resolveAssertions( *this, rExpr, rEnv ) ) {
			candidates.push_back( new Interpretation{ rExpr, rEnv, r.cost, r.argCost } );
		}
	}

	// quit if no such candidate
	if ( candidates.empty() ) {
		on_invalid( expr );
		collect_young();
		return Interpretation::make_invalid();
	}

	// ambiguous if more than one min-cost interpretation
	if ( candidates.size() > 1 ) {
		on_ambiguous( expr, candidates.begin(), candidates.end() );
		collect_young();
		return Interpretation::make_invalid();
	}

	const Interpretation* candidate = candidates.front();
#else
	if ( results.size() > 1 ) {
		// sort min-cost results to beginning
		InterpretationList::iterator min_pos = 
			sort_mins( results.begin(), results.end(), ByValueCompare<Interpretation>{} );
		
		// ambiguous if more than one min-cost interpretation
		if ( min_pos != results.begin() ) {
			on_ambiguous( expr, results.begin(), ++min_pos );
			collect_young();
			return Interpretation::make_invalid();
		}
	}
	
	const Interpretation* candidate = results.front();
#endif
	
	// handle ambiguous candidate from conversion expansion
	if ( ResolvedExprInvalid{ on_ambiguous }( candidate->expr ) ) {
		collect_young();
		return Interpretation::make_invalid();
	}

	// check for unbound type variables in environment
	List<TypeClass> unbound = getUnbound( candidate->env );
	if ( ! unbound.empty() ) {
		on_unbound( expr, unbound );
		collect_young();
		return Interpretation::make_invalid();
	}
	
	collect_young( candidate );
	return candidate;
}
