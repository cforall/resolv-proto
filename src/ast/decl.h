#pragma once

#include <algorithm>
#include <ostream>
#include <string>
#include <vector>

#include "ast.h"
#include "forall.h"
#include "spec_counter.h"
#include "type.h"

#include "data/cast.h"
#include "data/list.h"
#include "data/mem.h"
#include "data/option.h"

#include "resolver/cost.h"

/// A resolver declaration
class Decl : public ASTNode {
	std::string name_;  ///< Name of declaration
	std::string tag_;   ///< Disambiguating tag of declaration

public:
	Decl( const std::string& name_ ) : name_(name_), tag_() {}
	Decl( const std::string& name_, const std::string& tag_ ) : name_(name_), tag_(tag_) {}

	virtual Decl* clone() const = 0;

	const std::string& name() const { return name_; }
	const std::string& tag() const { return tag_; }

	virtual const Type* type() const = 0;
};

/// A variable declaration
class VarDecl final : public Decl {
	const Type* type_;  ///< Type of variable

public:
	typedef Decl Base;

	VarDecl(const std::string& name_, const Type* type_) : Decl(name_), type_(type_) {}
	VarDecl(const std::string& name_, const std::string& tag_, const Type* type_) 
		: Decl(name_, tag_), type_(type_) {}
	VarDecl(const std::string& name_, const std::string& tag_, List<Type>&& type_)
		: Decl(name_, tag_), type_( Type::from( move(type_) ) ) {}
	
	Decl* clone() const override { return new VarDecl{ name(), tag(), type_ }; }

	bool operator== (const VarDecl& that) const { 
		return name() == that.name() && tag() == that.tag();
	}
	bool operator!= (const VarDecl& that) const { return !(*this == that); }

	const Type* type() const override { return type_; }

	void write(std::ostream& out, ASTNode::Print style) const override {
		if ( style != ASTNode::Print::Concise ) {
			type_->write( out, style );
			out << " ";
		}
		out << name();
		if ( ! tag().empty() ) { out << "-" << tag(); }
	}

protected:
	void trace(const GC& gc) const override {
		gc << type_;
	}
};

/// A function declaration
class FuncDecl final : public Decl {
	const FuncType* type_;       ///< Type of the function
	unique_ptr<Forall> forall_;  ///< Names of polymorphic type variables
	
public:
	typedef Decl Base;
	
	FuncDecl(const std::string& name_, List<Type>&& params_, List<Type>&& returns_)
		: Decl(name_), type_( new FuncType { move(params_), move(returns_) } ), forall_() {}
	
	FuncDecl(const std::string& name_, const std::string& tag_, 
	         List<Type>&& params_, List<Type>&& returns_, unique_ptr<Forall>&& forall_ )
		: Decl(name_, tag_), type_( new FuncType { move(params_), move(returns_) } ), 
		  forall_( move(forall_) ) {}
	
	FuncDecl(const std::string& name_, const std::string& tag_,
	         List<Type>&& params_, const Type* returns_, unique_ptr<Forall>&& forall_)
		: Decl(name_, tag_), type_( new FuncType { move(params_), returns_ } ),  
		  forall_( move(forall_) ) {}
	
	FuncDecl(const std::string& name_, const std::string& tag_, const FuncType* type_, 
	         unique_ptr<Forall>&& forall_)
		: Decl(name_, tag_), type_(type_), forall_(move(forall_)) {}
	
	Decl* clone() const override {
		return new FuncDecl{ name(), tag(), type_, copy(forall_) };
	}
	
	bool operator== (const FuncDecl& that) const { 
		return name() == that.name() && tag() == that.tag();
	}
	bool operator!= (const FuncDecl& that) const { return !(*this == that); }
	
	const FuncType* type() const override { return type_; }
	const List<Type>& params() const { return type_->params(); }
	const Type* returns() const { return type_->returns(); }
	const Forall* forall() const { return forall_.get(); }

	/// The cost of this function's forall clause, if any
	Cost poly_cost() const {
		// zero poly cost if zero polymorphism
		if ( forall_ == nullptr ) return Cost::zero();

		SpecCounter count;
		Cost k = cost_of( *forall_ );

		// sum specialization costs for parameters
		for ( const Type* p : type_->params() ) {
			k.spec += count( p ).value_or( 0 );
		}

		// sum specialization costs for return values
		if ( const TupleType* rt = as_safe<TupleType>(type_->returns()) ) {
			for ( const Type* r : rt->types() ) {
				k.spec += count( r ).value_or( 0 );
			}
		} else {
			k.spec += count( type_->returns() ).value_or( 0 );
		}

		return k;
	}

	void write(std::ostream& out, ASTNode::Print style) const override {
		if ( style != ASTNode::Print::Concise ) {
			type_->returns()->write( out, style );
			out << " ";
		}
		out << name();
		if ( ! tag().empty() ) { out << "-" << tag(); }
		if ( style != ASTNode::Print::Concise ) {
			for ( auto& t : type_->params() ) {
				out << " ";
				t->write( out, style );
			}
			if ( forall_ ) for ( auto asn : forall_->assertions() ) {
				out << " | ";
				asn->write( out, style );
			}
		}
	}

protected:
	void trace(const GC& gc) const override {
		gc << type_ << forall_.get();
	}
};
