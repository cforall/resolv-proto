#pragma once

#include <ostream>

#include "cost.h"
#include "expr.h"
#include "type.h"
#include "utility.h"

/// An ambiguous interpretation with a given type
class AmbiguousExpr : public TypedExpr {
	Brw<Type> type_;
public:
	typedef Expr Base;
	
	AmbiguousExpr( Brw<Type> type_ ) : type_( type_ ) {}
	
	virtual Ptr<Expr> clone() const { return make<AmbiguousExpr>( type_ ); }
	
	virtual Brw<Type> type() const { return type_; }

protected:
	 virtual void write(std::ostream& out) const {
		 out << "<ambiguous expression of type " << *type_ << ">";
	 }
};

/// Typed interpretation of an expression
struct Interpretation {
	Shared<TypedExpr> expr;  /// Base expression for interpretation
	Cost cost;               /// Cost of interpretation
	
	/// Make an interpretation for an expression [default null]; 
	/// may provide cost [default 0]
	Interpretation( Shared<TypedExpr>&& expr = Shared<TypedExpr>{}, 
	                Cost&& cost = Cost{} )
		: expr( std::move(expr) ), cost( std::move(cost) ) {}
	
	/// true iff the interpretation is ambiguous; 
	/// interpretation must have a valid base
	bool is_ambiguous() const {
		return brw_as<AmbiguousExpr>( expr ) != nullptr;
	}
	
	/// true iff the interpretation is unambiguous and has a valid base
	bool is_valid() const { return expr != nullptr && ! is_ambiguous(); }
	
	/// Get the type of the interpretation
	Brw<Type> type() const { return expr->type(); }
	
	/// Returns a fresh invalid interpretation
	static Interpretation make_invalid() { return Interpretation{}; }
};

inline std::ostream& operator<< ( std::ostream& out, const Interpretation& i ) {
	if ( i.expr == nullptr ) {
		out << "<invalid interpretation>" << std::endl;
		return out;
	}
	
	out << i.cost << " as " << *i.type() << "\n"
	    << "\t" << *i.expr << std::endl;
	return out;
}

/// List of interpretations
typedef List<Interpretation, Raw> InterpretationList;
