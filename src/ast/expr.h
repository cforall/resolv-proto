#pragma once

// Copyright (c) 2015 University of Waterloo
//
// The contents of this file are covered under the licence agreement in 
// the file "LICENCE" distributed with this repository.

#include <ostream>
#include <string>
#include <utility>
#include <vector>

#include "ast.h"
#include "decl.h"
#include "forall.h"
#include "forall_substitutor.h"
#include "type.h"

#include "data/cast.h"
#include "data/debug.h"
#include "data/list.h"
#include "data/mem.h"
#include "resolver/conversion.h"

class Interpretation;

/// A resolvable expression
class Expr : public ASTNode {
public:
	virtual Expr* clone() const = 0;
};

/// An expression that is already typed
class TypedExpr : public Expr {
public:
	/// Get the resolved type for this expression
	virtual const Type* type() const = 0; 
};

/// An expression representing a single value of a type
class ValExpr : public TypedExpr {
	const Type* ty_;  ///< Type of variable expression
public:
	typedef Expr Base;
	
	ValExpr(const Type* ty_) : ty_( ty_ ) { assume( ty_, "var type is valid" ); }
	
	Expr* clone() const override { return new ValExpr( ty_ ); }
	
	const Type* type() const override { return ty_; }

	void write(std::ostream& out, ASTNode::Print style) const override {
		ty_->write( out, style );
	}

protected:
	void trace(const GC& gc) const override { gc << ty_; }
};

/// An expression representing a declaration name
class NameExpr : public Expr {
	std::string name_;  ///< Name of the expression referenced
public:
	typedef Expr Base;

	NameExpr(const std::string& name_) : name_(name_) {}

	Expr* clone() const override { return new NameExpr{ name_ }; }

	const std::string& name() const { return name_; }

	void write(std::ostream& out, ASTNode::Print) const override {
		out << "&" << name_;
	}
};

/// An expression resolved to a declaration
class VarExpr : public TypedExpr {
	const Decl* var_;  ///< Declaration referenced
public:
	typedef Expr Base;

	VarExpr( const Decl* var_ ) : var_( var_ ) {}

	Expr* clone() const override { return new VarExpr{ var_ }; }

	const Decl* var() const { return var_; }

	const Type* type() const override { return var_->type(); }

	void write(std::ostream& out, ASTNode::Print style) const override {
		out << "&" << var_->name();
		if ( style != ASTNode::Print::InputStyle && ! var_->tag().empty() ) {
			out << "-" << var_->tag();
		}
	}
};

/// A converting cast expression
class CastExpr : public TypedExpr {
	const Expr* arg_;         ///< Argument to the cast
	const Conversion* cast_;  ///< Conversion applied
public:
	typedef Expr Base;
	
	CastExpr(const Expr* arg_, const Conversion* cast_)
		: arg_( arg_ ), cast_( cast_ ) {}
	
	Expr* clone() const override { return new CastExpr{ arg_, cast_ }; }
	
	const Expr* arg() const { return arg_; }
	const Conversion* cast() const { return cast_; }
	
	const Type* type() const override { return cast_->to->type; }
	
	void write(std::ostream& out, ASTNode::Print style) const override {
		arg_->write( out, style );
		if ( style != ASTNode::Print::InputStyle ) {
			out << " => ";
			type()->write( out, style );
		}
	}

protected:
	void trace(const GC& gc) const override { gc << arg_; }
};

/// A truncating cast expression (e.g concrete to void, or dropping a tuple suffix)
class TruncateExpr : public TypedExpr {
	const TypedExpr* arg_;  ///< Expression that is truncated
	const Type* target_;    ///< Type arg is trimmed to
public:
	typedef Expr Base;

	TruncateExpr(const TypedExpr* arg_, const Type* target_)
		: arg_(arg_), target_(target_) {}

	/// Trim TypedExpr type to n elements (n < arg_->type()->size())
	TruncateExpr(const TypedExpr* arg_, unsigned n) : arg_(arg_) {
		assume( n < arg_->type()->size(), "truncates shorter" );
		switch ( n ) {
		case 0:
			target_ = new VoidType;
			break;
		case 1:
			// target has more than 1 type, must be tuple
			target_ = as<TupleType>(arg_->type())->types()[0];
			break;
		default:
			// target has more than n > 1 types, must be tuple
			const List<Type>& argt = as<TupleType>(arg_->type())->types();
			target_ = new TupleType{ List<Type>( argt.begin(), argt.begin() + n ) };
		}
	}

	Expr* clone() const override { return new TruncateExpr{ arg_, target_ }; }

	const TypedExpr* arg() const { return arg_; }
	
	const Type* type() const override { return target_; }

	void write(std::ostream& out, ASTNode::Print style) const override {
		arg_->write( out, style );
		if ( style != ASTNode::Print::InputStyle ) {
			out << " => ";
			target_->write( out, style );
		}
	}

protected:
	void trace(const GC& gc) const override { gc << arg_ << target_; }
};

/// An untyped function call expression
class FuncExpr : public Expr {
	std::string name_;  ///< Name of function called
	List<Expr> args_;   ///< Arguments to call
public:
	typedef Expr Base;
	
	FuncExpr(const std::string& name_, List<Expr>&& args_)
		: name_( name_ ), args_( move(args_) ) {}
	
	Expr* clone() const override {
		return new FuncExpr( name_, copy( args_ ) );
	}
	
	const std::string& name() const { return name_; }
	const List<Expr>& args() const { return args_; }

	void write(std::ostream& out, ASTNode::Print style) const override {
		out << name_ << "(";
		for (auto& arg : args_) {
			out << " ";
			arg->write( out, style );
		} 
		out << " )";
	}

protected:
	void trace(const GC& gc) const override { gc << args_; }
};

/// A typed function call expression
class CallExpr : public TypedExpr {
	const FuncDecl* func_;       ///< Function called
	List<TypedExpr> args_;       ///< Function arguments
	unique_ptr<Forall> forall_;  ///< Type variables owned by this call.
	const Type* retType_;        ///< Return type of call, after type substitution

	/// Replaces type variables in the return type with type variables from the forall
	void fixRetType() {
		if ( forall_ ) {
			ForallSubstitutor{ forall_.get() }.mutate( retType_ );
		}
	}
public:
	typedef Expr Base;
	
	CallExpr( const FuncDecl* func_, unsigned& src, List<TypedExpr>&& args_ = List<TypedExpr>{} )
		: func_( func_ ), args_( move(args_) ), 
		  forall_( Forall::from( func_->forall(), src ) ), retType_( func_->returns() ) 
		{ fixRetType(); }

	CallExpr( const FuncDecl* func_, List<TypedExpr>&& args_, unique_ptr<Forall>&& forall_ )
		: func_( func_ ), args_( move(args_) ), forall_( move(forall_) ), 
		  retType_( func_->returns() ) { fixRetType(); }
	
	CallExpr( const FuncDecl* func_, List<TypedExpr>&& args_, unique_ptr<Forall>&& forall_, 
	          const Type* retType_ )
		: func_( func_ ), args_( move(args_) ), forall_( move(forall_) ), retType_( retType_ ) {}
	
	Expr* clone() const override {
		return new CallExpr( func_, copy(args_), copy(forall_), retType_ );
	}
	
	const FuncDecl* func() const { return func_; }
	const List<TypedExpr>& args() const { return args_; }
	const Forall* forall() const { return forall_.get(); }
	
	const Type* type() const override { return retType_; }

	void write(std::ostream& out, ASTNode::Print style) const override {
		out << func_->name();
		if ( style != ASTNode::Print::InputStyle && ! func_->tag().empty() ) {
				out << "-" << func_->tag();
		}
		if ( style == ASTNode::Print::Default ) {
			if ( forall_ ) { out << *forall_; }
		}
		out << "(";
		for (auto& arg : args_) {
			out << " "; 
			arg->write( out, style );
		}
		out << " )";
	}
	
protected:
	void trace(const GC& gc) const override {
		gc << func_ << args_ << forall_.get() << retType_;
	}
};

/// A single element of a tuple from an underlying expression
class TupleElementExpr : public TypedExpr {
	const TypedExpr* of_;
	unsigned ind_;
public:
	typedef Expr Base;
	
	TupleElementExpr( const TypedExpr* of_, unsigned ind_ ) : of_( of_ ), ind_( ind_ ) {
		assume( is<TupleType>( of_->type() ), "tuple element base valid" );
	}
	
	Expr* clone() const override { return new TupleElementExpr( of_, ind_ ); }
	
	const TypedExpr* of() const { return of_; }
	
	unsigned ind() const { return ind_; }
	
	const Type* type() const override {
		return as<TupleType>( of_->type() )->types()[ ind_ ];
	}

	void write(std::ostream& out, ASTNode::Print style) const override {
		if ( ind_ == 0 ) {
			of_->write( out, style );
			if ( style != ASTNode::Print::InputStyle ) out << "[0]";
		} else {
			if ( style != ASTNode::Print::InputStyle ) out << "[" << ind_ << "]";
		}
	}

protected:
	void trace(const GC& gc) const override { gc << of_; }
};

/// A tuple constituted of a list of underlying expressions
class TupleExpr : public TypedExpr {
	List<TypedExpr> els_;
	const TupleType* ty_;

	TupleExpr( const List<TypedExpr>& els_, const TupleType* ty_ ) 
		: els_( els_ ), ty_( ty_ ) {}
public:
	typedef Expr Base;

	TupleExpr( List<TypedExpr>&& els_ ) : els_( move(els_) ) {
		List<Type> tys;
		tys.reserve( this->els_.size() );
		for ( const TypedExpr* el : this->els_ ) { tys.push_back( el->type() ); }
		ty_ = new TupleType( move(tys) );
	}

	Expr* clone() const override { return new TupleExpr( els_, ty_ ); }

	const List<TypedExpr>& els() const { return els_; }

	const TupleType* ty() const { return ty_; }

	const Type* type() const override { return ty_; }

	void write(std::ostream& out, ASTNode::Print style) const override {
		out << "[";
		for ( const TypedExpr* el : els_ ) {
			out << " ";
			el->write( out, style );
		}
		out << " ]";
	}

protected:
	void trace(const GC& gc) const override { gc << els_ << ty_; }
};

/// An ambiguous interpretation with a given type
class AmbiguousExpr : public TypedExpr {
	const Expr* expr_;           ///< Expression that is ambiguously resolved
	const Type* type_;           ///< Type of ambiguous expression
	List<Interpretation> alts_;  ///< Equal-cost alternatives
public:
	typedef Expr Base;
	
	AmbiguousExpr( const Expr* expr_, const Type* type_, 
	               List<Interpretation>&& alts_ = {} )
		: expr_(expr_), type_( type_ ), alts_( move(alts_) ) {}
	
	Expr* clone() const override {
		return new AmbiguousExpr( expr_, type_, copy(alts_) );
	}

	const Expr* expr() const { return expr_; }
	
	const List<Interpretation>& alts() const { return alts_; }

	const Type* type() const override { return type_; }

	void write(std::ostream& out, ASTNode::Print style) const override {
		out << "<ambiguous resolution of type ";
		type_->write( out, style );
		out << " for ";
		expr_->write( out, style );
		out << ">";
	}

protected:
	void trace(const GC& gc) const override;
};
