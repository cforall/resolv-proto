#pragma once

#include <ostream>

#include "cost.h"
#include "expr.h"
#include "type.h"
#include "utility.h"

/// Typed interpretation of an expression
struct Interpretation {
	Shared<TypedExpr> expr;  /// Base expression for interpretation
	Cost cost;               /// Cost of interpretation
	
/*	Interpretation() = default;
	Interpretation(const Interpretation&) = default;
	Interpretation(Interpretation&&) = default;
	Interpretation& operator= (const Interpretation&) = default;
	Interpretation& operator= (Interpretation&&) = default;
*/	
	/// Make an interpretation for an expression [default null]; 
	/// may provide cost [default 0]
	Interpretation( Shared<TypedExpr>&& expr = Shared<TypedExpr>{}, 
	                Cost&& cost = Cost{} )
		: expr( std::move(expr) ), cost( std::move(cost) ) {}
	
	/// true iff the interpretation is ambiguous
	bool is_ambiguous() const {
		return ref_as<AmbiguousExpr>( expr ) != nullptr;
	}
	
	/// true iff the interpretation is unambiguous and has a valid base
	bool is_valid() const { return expr != nullptr && ! is_ambiguous(); }
	
	/// Get the type of the interpretation
	Ref<Type> type() const { return expr->type(); }
	
	/// Returns a fresh invalid interpretation
	static Interpretation make_invalid() { return Interpretation{}; }
};

std::ostream& operator<< ( std::ostream& out, const Interpretation& i ) {
	if ( i.expr == nullptr ) {
		out << "<<INVALID>>" << std::endl;
		return out;
	}
	
	out << i.cost << " as " << *i.type() << "\n"
	    << "\t" << *i.expr << std::endl;
	return out;
}

/// List of interpretations
typedef List<Interpretation, Raw> InterpretationList;
