#pragma once

#include <functional>
#include <ostream>
#include <utility>

#include "cost.h"
#include "data.h"
#include "expr.h"
#include "gc.h"
#include "type.h"

/// An ambiguous interpretation with a given type
class AmbiguousExpr : public TypedExpr {
	const Type* type_;  ///< Type of ambiguous expression
public:
	typedef Expr Base;
	
	AmbiguousExpr( const Type* type_ ) : type_( type_ ) {}
	
	virtual Expr* clone() const { return new AmbiguousExpr( type_ ); }

	virtual void accept( Visitor& ) const { /* do nothing; not known by Visitor */ }
	
	virtual const Type* type() const { return type_; }

protected:
	virtual void trace(const GC& gc) const { gc << type_; }

	virtual void write(std::ostream& out) const {
		out << "<ambiguous expression of type " << *type_ << ">";
	}
};

/// Representation of a polymorphic return type from a function
class PolyRetType : public Type {
	std::string name_;     ///< Name of the polymorphic type variable
	const CallExpr* src_;  ///< Function call this type belongs to
public:
	typedef Type Base;

	PolyRetType( const std::string& name_, const CallExpr* src_ ) : name_(name_), src_(src_) {}

	virtual Type* clone() const { return new PolyRetType( name_, src_ ); }

	virtual void accept( Visitor& ) const { /* do nothing; not known by Visitor */ }

	const std::string& name() const { return name_; }
	const CallExpr* src() const { return src_; }

	virtual unsigned size() const { return 1; }

protected:
	virtual void trace(const GC& gc) const { gc << src_; }

	virtual void write(std::ostream& out) const {
		out << name_ << "[" << src_->func()->name() << "]";
	}

	virtual bool equals(const Type& obj) const {
		const PolyRetType* that = as_safe<PolyRetType>(&obj);
		return that && (src_ == that->src_) && (name_ == that->name_); 
	}

	virtual std::size_t hash() const {
		return (std::hash<std::string>()( name_ ) << 1) ^ std::hash<const CallExpr*>()( src_ );
	}
};

/// Typed interpretation of an expression
struct Interpretation : public GC_Object {
	const TypedExpr* expr;  /// Base expression for interpretation
	Cost cost;              /// Cost of interpretation
	
	/// Make an interpretation for an expression [default null]; 
	/// may provide cost [default 0]
	Interpretation( const TypedExpr* expr = nullptr, 
	                Cost&& cost = Cost{} )
		: expr( expr ), cost( move(cost) ) {}
	
	friend void swap(Interpretation& a, Interpretation& b) {
		using std::swap;

		swap(a.expr, b.expr);
		swap(a.cost, b.cost);
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

	/// Returns a fresh ambiguous interpretation
	static Interpretation* make_ambiguous( const Type* t, const Cost& c ) {
		return new Interpretation{ new AmbiguousExpr{ t }, copy(c) };
	}
	static Interpretation* make_ambiguous( const Type* t, Cost&& c ) {
		return new Interpretation{ new AmbiguousExpr{ t }, move(c) };
	}
};

inline std::ostream& operator<< ( std::ostream& out, const Interpretation& i ) {
	if ( i.expr == nullptr ) {
		out << "<invalid interpretation>";
		return out;
	}
	
	out << "[" << *i.type() << " @ " << i.cost << "] " << *i.expr;
	return out;
}

/// List of interpretations
typedef List<Interpretation> InterpretationList;
