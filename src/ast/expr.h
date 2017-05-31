#pragma once

#include <cassert>
#include <ostream>
#include <string>
#include <utility>
#include <vector>

#include "ast.h"
#include "decl.h"
#include "type.h"

#include "data/cast.h"
#include "data/list.h"
#include "data/mem.h"
#include "resolver/conversion.h"
#include "resolver/forall.h"
#include "resolver/forall_substitutor.h"

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

/// A variable expression
class VarExpr : public TypedExpr {
	const Type* ty_;  ///< Type of variable expression
public:
	typedef Expr Base;
	
	VarExpr(const Type* ty_) : ty_( ty_ ) { assert( ty_ ); }
	
	Expr* clone() const override { return new VarExpr( ty_ ); }
	
	const Type* type() const override { return ty_; }

	void write(std::ostream& out, ASTNode::Print style) const override {
		ty_->write( out, style );
	}

protected:
	void trace(const GC& gc) const override { gc << ty_; }
};

/// A cast expression
class CastExpr : public TypedExpr {
	const Expr* arg_;         ///< Argument to the cast
	const Conversion* cast_;  ///< Conversion applied
public:
	typedef Expr Base;
	
	CastExpr(const Expr* arg_, const Conversion* cast_)
		: arg_( arg_ ), cast_( cast_ ) {}
	
	Expr* clone() const override { return new CastExpr( arg_, cast_ ); }
	
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

	void fixRetType( const Forall* oldForall ) {
		if ( forall_ ) { ForallSubstitutor{ oldForall, forall_.get() }.mutate( retType_ ); }
	}
public:
	typedef Expr Base;
	
	CallExpr( const FuncDecl* func_, List<TypedExpr>&& args_ = List<TypedExpr>{} )
			: func_( func_ ), args_( move(args_) ), 
			  forall_( func_->forall() ? new Forall{ *func_->forall() } : nullptr ),
			  retType_( func_->returns() ) { fixRetType( func_->forall() ); }

	CallExpr( const FuncDecl* func_, const List<TypedExpr>& args_, 
	          const unique_ptr<Forall>& forall_ )
			: func_( func_ ), args_( args_ ), 
		 	  forall_( forall_ ? new Forall{ *forall_ } : nullptr ),
			  retType_( func_->returns() ) { fixRetType( forall_.get() ); }

	CallExpr( const FuncDecl* func_, List<TypedExpr>&& args_, 
	          unique_ptr<Forall>&& forall_ )
		: func_( func_ ), args_( move(args_) ), forall_( move(forall_) ), 
		  retType_( func_->returns() ) {}
	
	Expr* clone() const override { return new CallExpr( func_, args_, forall_ ); }
	
	const FuncDecl* func() const { return func_; }
	const List<TypedExpr>& args() const { return args_; }
	const Forall* forall() const { return forall_.get(); }
	
	const Type* type() const override { return retType_; }

	void write(std::ostream& out, ASTNode::Print style) const override {
		out << func_->name();
		if ( style != ASTNode::Print::InputStyle ) {
			if ( ! func_->tag().empty() ) { out << "-" << func_->tag(); }
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
	void trace(const GC& gc) const override { gc << func_ << args_ << forall_.get() << retType_; }
};

/// A single element of a tuple from an underlying expression
class TupleElementExpr : public TypedExpr {
	const TypedExpr* of_;
	unsigned ind_;
public:
	typedef Expr Base;
	
	TupleElementExpr( const TypedExpr* of_, unsigned ind_ )
		: of_( of_ ), ind_( ind_ ) { assert( is<TupleType>( of_->type() ) ); }
	
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

	TupleExpr( const List<TypedExpr>& els_, const TupleType* ty_ ) : els_( els_ ), ty_( ty_ ) {}
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
	const Expr* expr_;      ///< Expression that is ambiguously resolved
	const Type* type_;      ///< Type of ambiguous expression
	List<TypedExpr> alts_;  ///< Equal-cost alternatives
public:
	typedef Expr Base;
	
	AmbiguousExpr( const Expr* expr_, const Type* type_, List<TypedExpr>&& alts_ = {} )
		: expr_(expr_), type_( type_ ), alts_( move(alts_) ) {}
	
	Expr* clone() const override { return new AmbiguousExpr( expr_, type_, copy(alts_) ); }

	const Expr* expr() const { return expr_; }
	
	const List<TypedExpr>& alts() const { return alts_; }

	const Type* type() const override { return type_; }

	void write(std::ostream& out, ASTNode::Print style) const override {
		out << "<ambiguous resolution of type " << *type_ << " for ";
		expr_->write( out, style );
		out << ">";
	}

protected:
	void trace(const GC& gc) const override { gc << expr_ << type_ << alts_; }
};
