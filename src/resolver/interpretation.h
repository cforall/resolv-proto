#pragma once

#include <functional>
#include <ostream>
#include <utility>

#include "cost.h"
#include "env.h"

#include "ast/expr.h"
#include "ast/type.h"
#include "data/cast.h"
#include "data/gc.h"
#include "data/list.h"
#include "data/mem.h"

/// Typed interpretation of an expression
struct Interpretation : public GC_Object {
	const TypedExpr* expr;  ///< Base expression for interpretation
	Cost cost;              ///< Cost of interpretation
	unique_ptr<Env> env;    ///< Type variables and assertions bound by this interpretation
	
	/// Make an interpretation for an expression [default null]; 
	/// may provide cost [default 0] and environment [default empty]
	Interpretation( const TypedExpr* expr = nullptr, Cost&& cost = Cost{}, 
	                unique_ptr<Env>&& env = nullptr )
		: expr( expr ), cost( move(cost) ), env( move(env) ) {}
	
	/// Fieldwise copy-constructor
	Interpretation( const TypedExpr* expr, const Cost& cost, const unique_ptr<Env>& env )
		: expr( expr ), cost( cost ), env( Env::from( env.get() ) ) {}
	
	friend void swap(Interpretation& a, Interpretation& b) {
		using std::swap;

		swap(a.expr, b.expr);
		swap(a.cost, b.cost);
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
		                           copy(i->cost) };
	}

	/// Orders two interpretations by cost.
	bool operator< (const Interpretation& o) const { return cost < o.cost; }

protected:
	virtual void trace(const GC& gc) const {
		gc << expr << env.get();
    }
};

inline std::ostream& operator<< ( std::ostream& out, const Interpretation& i ) {
	if ( i.expr == nullptr ) return out << "<invalid interpretation>";
	
	out << "[" << *replace( i.env.get(), i.type() ) << " / " << i.cost << "]";
	if ( i.env ) { out << *i.env; }
	return out << " " << *i.expr;
}

/// List of interpretations TODO just inline typedef
typedef List<Interpretation> InterpretationList;

/// Functor to extract the cost from an interpretation
struct interpretation_cost {
	const Cost& operator() ( const Interpretation* i ) { return i->cost; }	
};
