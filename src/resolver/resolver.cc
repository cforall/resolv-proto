#include "resolver.h"

#include "env.h"
#include "interpretation.h"
#if defined RP_ASN_DEF || defined RP_ASN_DCA || defined RP_ASN_IMM
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
	auto gc_guard = new_generation();  // set up new GC generation
#if defined RP_DIR_TD
	cached.clear();  // clear subexpression cache
#endif

	Env env;
	InterpretationList results = resolve( expr, env, ResolverMode::top_level() );
	
	// return invalid interpretation on empty results
	if ( results.empty() ) {
		on_invalid( expr );
		return Interpretation::make_invalid();
	}

#if defined RP_ASN_DEF || defined RP_ASN_DCA || defined RP_ASN_IMM
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
		Env rEnv = r.env;
#if defined RP_ASN_IMM
		AssertionResolver assnDisambiguator{ *this, rEnv, true };  // disambiguates by assertions
		if ( assnDisambiguator.mutate( rExpr ) != nullptr ) {
#else
		if ( resolveAssertions( *this, rExpr, rEnv ) ) {  // deferred resolution
#endif
			candidates.push_back( new Interpretation{ rExpr, rEnv, r.cost, r.argCost } );
		}
	}

	// quit if no such candidate
	if ( candidates.empty() ) {
		on_invalid( expr );
		return Interpretation::make_invalid();
	}

	// ambiguous if more than one min-cost interpretation
	if ( candidates.size() > 1 ) {
		on_ambiguous( expr, candidates.begin(), candidates.end() );
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
			return Interpretation::make_invalid();
		}
	}
	
	const Interpretation* candidate = results.front();
#endif
	
	// handle ambiguous candidate from conversion expansion
	if ( ResolvedExprInvalid{ on_ambiguous }( candidate->expr ) ) {
		return Interpretation::make_invalid();
	}

	// check for unbound type variables in environment
	std::vector<TypeClass> unbound = candidate->env.getUnbound();
	if ( ! unbound.empty() ) {
		on_unbound( expr, unbound );
		return Interpretation::make_invalid();
	}
	
	trace( candidate );
	return candidate;
}

void Resolver::beginScope() {
	funcs.beginScope();
}

void Resolver::endScope() {
	funcs.endScope();
}

void Resolver::addType(const Type* ty) {
	conversions.addType( ty );
}

void Resolver::addDecl(Decl* decl) {
	::addDecl( funcs, decl );
}

void Resolver::addExpr(const Expr* expr) {
	const Interpretation* i = (*this)(expr);
	if ( i->is_valid() ) { on_valid( expr, i ); }
}
