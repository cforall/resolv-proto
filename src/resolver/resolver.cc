#include "resolver.h"

#include "env.h"
#include "interpretation.h"

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

	InterpretationList results = resolve( expr, Env::none(), Mode::top_level() );
	
	// return invalid interpretation on empty results
	if ( results.empty() ) {
		on_invalid( expr );
		return Interpretation::make_invalid();
	}

	if ( results.size() > 1 ) {
		// sort min-cost results to beginning
		InterpretationList::iterator min_pos = 
			sort_mins( results.begin(), results.end(), ByValueCompare<Interpretation>{} );
		
		// ambiguous if more than one min-cost interpretation
		if ( min_pos != results.begin() ) {
			on_ambiguous( expr, results.begin(), ++min_pos );
			return Interpretation::make_invalid();
		}
	}
	
	const Interpretation* candidate = results.front();
	
	// handle ambiguous candidate from conversion expansion
	if ( ResolvedExprInvalid{ on_ambiguous }( candidate->expr ) ) {
		return Interpretation::make_invalid();
	}

	// check for unbound type variables in environment
	List<TypeClass> unbound = getUnbound( candidate->env );
	if ( ! unbound.empty() ) {
		on_unbound( expr, unbound );
		return Interpretation::make_invalid();
	}
	
	return candidate;
}
