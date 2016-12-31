#pragma once

#include <algorithm>
#include <memory>
#include <ostream>
#include <string>
#include <vector>

#include "ast.h"
#include "binding.h"
#include "binding_sub_mutator.h"
#include "data.h"
#include "type.h"

/// A resolver declaration
class Decl : public ASTNode {
	friend std::ostream& operator<< (std::ostream&, const Decl&);
public:
	virtual Decl* clone() const = 0;
protected:
	virtual void write(std::ostream& out) const = 0;
};

inline std::ostream& operator<< (std::ostream& out, const Decl& d) {
	d.write(out);
	return out;
}

/// A function declaration
class FuncDecl final : public Decl {
	std::string name_;                 ///< Name of function
	std::string tag_;                  ///< Disambiguating tag for function
	List<Type> params_;                ///< Parameter types of function
	const Type* returns_;              ///< Return types of function
	unique_ptr<TypeBinding> tyVars_;   ///< Names of polymorphic type variables
	
	/// Generate appropriate return type from return list
	static const Type* gen_returns(const List<Type>& rs) {
		switch ( rs.size() ) {
		case 0:
			return new VoidType();
		case 1:
			return rs.front();
		default:
			return new TupleType( rs );
		}
	}
	
public:
	typedef Decl Base;
	
	FuncDecl(const std::string& name_, const std::string& tag_, 
	         const List<Type>& params_, const Type* returns_, 
			 const unique_ptr<TypeBinding>& tyVars_ )
		: name_(name_), tag_(tag_), params_(params_), 
		  returns_(returns_), tyVars_( copy(tyVars_) ) {
		// Rebind the polymorphic types to the new binding instance
		if ( ! this->tyVars_ ) return;
		auto m = BindingSubMutator{ *this->tyVars_ };
		for ( const Type*& param : this->params_ ) {
			m.mutate( param );
		}
		m.mutate( this->returns_ );
	}
	
	FuncDecl(const std::string& name_, const List<Type>& params_, 
	         const List<Type>& returns_, unique_ptr<TypeBinding>&& tyVars_ )
		: name_(name_), tag_(), params_(params_), 
		  returns_( gen_returns( returns_ ) ), 
		  tyVars_( move(tyVars_) ) {}
	
	FuncDecl(const std::string& name_, const std::string& tag_, 
	         const List<Type>& params_, const List<Type>& returns_, unique_ptr<TypeBinding>&& tyVars_ )
		: name_(name_), tag_(tag_), params_(params_), 
		  returns_( gen_returns( returns_ ) ), 
		  tyVars_( move(tyVars_) ) {}
	
	virtual Decl* clone() const {
		return new FuncDecl( name_, tag_, params_, returns_, tyVars_ );
	}

	virtual void accept( Visitor& v ) const { v.visit( this ); }
	
	bool operator== (const FuncDecl& that) const { 
		return name_ == that.name_ && tag_ == that.tag_;
	}
	bool operator!= (const FuncDecl& that) const { return !(*this == that); }
	
	const std::string& name() const { return name_; }
	const std::string& tag() const { return tag_; }
	const List<Type>& params() const { return params_; }
	const Type* returns() const { return returns_; }
	const unique_ptr<TypeBinding>& tyVars() const { return tyVars_; }

protected:
	virtual void trace(const GC& gc) const {
		gc << params_ << returns_;
	}

	virtual void write(std::ostream& out) const {
		out << *returns_ << " ";
		out << name_;
		if ( ! tag_.empty() ) { out << "-" << tag_; }
		for ( auto& t : params_ ) { out << " " << *t; }
	}
};

/// A "variable" declaration
class VarDecl final : public Decl {
	const ConcType* ty_;  ///< Type of variable
public:
	typedef Decl Base;
	
	VarDecl(const ConcType* ty_) : ty_(ty_) {}
	
	virtual Decl* clone() const { return new VarDecl( ty_ ); }

	virtual void accept( Visitor& v ) const { v.visit( this ); }
	
	bool operator== (const VarDecl& that) const { return ty_ == that.ty_; }
	bool operator!= (const VarDecl& that) const { return !(*this == that); }
	
	const ConcType* ty() const { return ty_; }
protected:
	virtual void trace(const GC& gc) const { gc << ty_; }

	virtual void write(std::ostream& out) const { out << *ty_; }
};
