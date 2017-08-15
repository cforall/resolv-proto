#pragma once

#include <algorithm>
#include <ostream>
#include <string>
#include <vector>

#include "ast.h"
#include "forall.h"
#include "type.h"

#include "data/cow.h"
#include "data/list.h"
#include "data/mem.h"

#include "resolver/cost.h"

/// A resolver declaration
class Decl : public ASTNode {
public:
	virtual Decl* clone() const = 0;
};

/// A function declaration
class FuncDecl final : public Decl {
	std::string name_;        ///< Name of function
	std::string tag_;         ///< Disambiguating tag for function
	List<Type> params_;       ///< Parameter types of function
	const Type* returns_;     ///< Return types of function
	unique_ptr<Forall> forall_;  ///< Names of polymorphic type variables
	
	/// Generate appropriate return type from return list
	static const Type* gen_returns(const List<Type>& rs) {
		switch ( rs.size() ) {
		case 0:
			return new VoidType{};
		case 1:
			return rs.front();
		default:
			return new TupleType{ rs };
		}
	}

	/// Generate appropriate return type from return list
	static const Type* gen_returns(List<Type>&& rs) {
		switch ( rs.size() ) {
		case 0:
			return new VoidType{};
		case 1:
			return rs.front();
		default:
			return new TupleType{ move(rs) };
		}
	}
	
public:
	typedef Decl Base;
	
	FuncDecl(const std::string& name_, List<Type>&& params_, List<Type>&& returns_)
		: name_(name_), tag_(), params_(move(params_)), 
		  returns_( gen_returns( move(returns_) ) ), forall_() {}
	
	FuncDecl(const std::string& name_, const std::string& tag_, 
	         List<Type>&& params_, List<Type>&& returns_, unique_ptr<Forall>&& forall_ )
		: name_(name_), tag_(tag_), params_(move(params_)), 
		  returns_( gen_returns( move(returns_) ) ), forall_( move(forall_) ) {}
	
	FuncDecl(const std::string& name_, const std::string& tag_,
	         List<Type>&& params_, const Type* returns_, unique_ptr<Forall>&& forall_)
		: name_(name_), tag_(tag_), params_( move(params_) ), 
		  returns_(returns_), forall_( move(forall_) ) {}
	
	Decl* clone() const override {
		return new FuncDecl{ name_, tag_, copy(params_), returns_, copy(forall_) };
	}
	
	bool operator== (const FuncDecl& that) const { 
		return name_ == that.name_ && tag_ == that.tag_;
	}
	bool operator!= (const FuncDecl& that) const { return !(*this == that); }
	
	const std::string& name() const { return name_; }
	const std::string& tag() const { return tag_; }
	const List<Type>& params() const { return params_; }
	const Type* returns() const { return returns_; }
	const Forall* forall() const { return forall_.get(); }

	/// The cost of this function's forall clause, if any
	Cost poly_cost() const { return forall_ ? cost_of( *forall_ ) : Cost::zero(); }

	void write(std::ostream& out, ASTNode::Print style) const override {
		if ( style != ASTNode::Print::Concise ) {
			returns_->write( out, style );
			out << " ";
		}
		out << name_;
		if ( ! tag_.empty() ) { out << "-" << tag_; }
		if ( style != ASTNode::Print::Concise ) {
			for ( auto& t : params_ ) {
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
		gc << params_ << returns_ << forall_.get();
	}
};
