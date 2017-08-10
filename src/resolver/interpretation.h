#pragma once

#include <functional>
#include <ostream>
#include <utility>

#include "cost.h"
#include "env.h"

#include "ast/expr.h"
#include "ast/type.h"
#include "data/cast.h"
#include "data/compare.h"
#include "data/gc.h"
#include "data/list.h"
#include "data/mem.h"

/// Typed interpretation of an expression
struct Interpretation : public GC_Object {
	const TypedExpr* expr;  ///< Base expression for interpretation
	Env* env;               ///< Type variables and assertions bound by this interpretation
	Cost cost;              ///< Overall cost of interpretation
	Cost argCost;			///< Summed cost of the interpretation's arguments
	
	/// Make an interpretation for an expression [default null]; 
	/// may provide cost [default 0] and environment [default empty]
	Interpretation( const TypedExpr* expr = nullptr, Env* env = nullptr, 
	                Cost&& cost = Cost{}, Cost&& argCost = Cost{} )
		: expr(expr), env(env), cost(move(cost)), argCost(move(argCost)) {}
	
	/// Fieldwise copy-constructor
	Interpretation( const TypedExpr* expr, const Env* env, const Cost& cost, const Cost& argCost )
		: expr(expr), env(Env::from( env )), cost(cost), argCost(argCost) {}
	
	friend void swap(Interpretation& a, Interpretation& b) {
		using std::swap;

		swap(a.expr, b.expr);
		swap(a.cost, b.cost);
		swap(a.argCost, b.argCost);
		swap(a.env, b.env);
	}
	
	/// true iff the interpretation is ambiguous; 
	/// interpretation must have a valid base
	bool is_ambiguous() const { return is<AmbiguousExpr>( expr ); }
	
	/// true iff the interpretation is unambiguous and has a valid base
	bool is_valid() const { return expr != nullptr && ! is_ambiguous(); }
	
	/// Get the type of the interpretation
	const Type* type() const { return expr->type(); }
	
	/// Returns a fresh invalid interpretation
	static Interpretation* make_invalid() { return new Interpretation{}; }

	/// Merges two interpretations (must have same cost, type, and underlying expression) 
	/// to produce a new ambiguous interpretation. No environment is stored in the outer 
	/// interpretations, but inner environments are preserved.
	static Interpretation* merge_ambiguous( const Interpretation* i, 
	                                        const Interpretation* j ) {
		List<Interpretation> alts;
		const Expr* expr;
		if ( const AmbiguousExpr* ie = as_safe<AmbiguousExpr>( i->expr ) ) {
			alts = ie->alts();
			expr = ie->expr();
		} else {
			alts.push_back( i );
			expr = i->expr;
		}
		if ( const AmbiguousExpr* je = as_safe<AmbiguousExpr>( j->expr ) ) {
			alts.insert( alts.end(), je->alts().begin(), je->alts().end() );
		} else {
			alts.push_back( j );
		}

		return new Interpretation{ new AmbiguousExpr{ expr, i->type(), move(alts) }, 
		                           nullptr, copy(i->cost), copy(i->argCost) };
	}

protected:
	virtual void trace(const GC& gc) const { gc << expr << env; }
};

/// Orders two interpretations by cost
inline comparison compare(const Interpretation& a, const Interpretation& b) {
	comparison c;
	if ( (c = compare(a.cost.unsafe, b.cost.unsafe)) != 0 ) return c;
	if ( (c = compare(a.cost.poly, b.cost.poly)) != 0 ) return c;
	if ( (c = compare(getCost(a.env), getCost(b.env))) != 0 ) return c;
	if ( (c = compare(a.cost.safe, b.cost.safe)) != 0 ) return c;
	return compare(a.argCost, b.argCost);
}

/// Orders two interpretations by cost.
inline bool operator< (const Interpretation& a, const Interpretation& b) {
	return compare(a, b) < 0;
}

inline bool operator<= (const Interpretation& a, const Interpretation& b) {
	return compare(a, b) <= 0;
}

inline bool operator> (const Interpretation& a, const Interpretation& b) {
	return compare(a, b) > 0;
}

inline bool operator>= (const Interpretation& a, const Interpretation& b) {
	return compare(a, b) >= 0;
}

inline std::ostream& operator<< ( std::ostream& out, const Interpretation& i ) {
	if ( i.expr == nullptr ) return out << "<invalid interpretation>";
	
	out << "[" << *replace( i.env, i.type() ) << " / (" 
		<< i.cost.unsafe << "," << i.cost.poly << ",";
	if ( i.env ) {
		out << i.env->cost.vars << ",";
		if ( i.env->cost.assns ) { out << "-" << i.env->cost.assns; } else { out << "0"; }
	} else { out << "0,0"; }
	out << "," << i.cost.safe << ")]";
	if ( i.env ) { out << *i.env; }
	return out << " " << *i.expr;
}

/// List of interpretations TODO just inline typedef
using InterpretationList = List<Interpretation>;

/// Functor to extract the cost from an interpretation
struct interpretation_cost {
	const Cost& operator() ( const Interpretation* i ) { return i->cost; }	
};
