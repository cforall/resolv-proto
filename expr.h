#pragma once

#include <memory>
#include <ostream>
#include <string>
#include <utility>
#include <vector>

#include "ast.h"
#include "conversion.h"
#include "data.h"
#include "decl.h"
#include "type.h"

/// A resolvable expression
class Expr : public ASTNode {
	friend std::ostream& operator<< (std::ostream&, const Expr&);
public:
	virtual ~Expr() = default;
	
	virtual Expr* clone() const = 0;
protected:
	virtual void write(std::ostream& out) const = 0;
};

inline std::ostream& operator<< (std::ostream& out, const Expr& e) {
	e.write(out);
	return out;
};

/// An expression that is already typed
class TypedExpr : public Expr {
public:
	/// Get the resolved type for this expression
	virtual const Type* type() const = 0; 
};

/// A variable expression
class VarExpr : public TypedExpr {
	const ConcType* ty_;  ///< Type of variable expression
public:
	typedef Expr Base;
	
	VarExpr(const ConcType* ty_) : ty_( ty_ ) {}
	
	virtual Expr* clone() const { return new VarExpr( ty_ ); }

	virtual void accept( Visitor& v ) const { v.visit( this ); }
	
	virtual const ConcType* ty() const { return ty_; }
	
	virtual const Type* type() const { return ty_; }

protected:
	virtual void trace(const GC& gc) const { gc << ty_; }

	virtual void write(std::ostream& out) const { out << *ty_; }
};

/// A cast expression
class CastExpr : public TypedExpr {
	const Expr* arg_;         ///< Argument to the cast
	const Conversion* cast_;  ///< Conversion applied
public:
	typedef Expr Base;
	
	CastExpr(const Expr* arg_, const Conversion* cast_)
		: arg_( arg_ ), cast_( cast_ ) {}
	
	virtual Expr* clone() const { return new CastExpr( arg_, cast_ ); }

	virtual void accept( Visitor& v ) const { v.visit( this ); }
	
	const Expr* arg() const { return arg_; }
	const Conversion* cast() const { return cast_; }
	
	virtual const Type* type() const { return cast_->to->type; }
	
protected:
	virtual void trace(const GC& gc) const { gc << arg_; }

	virtual void write(std::ostream& out) const {
		out << *arg_ << " => " << *type();
	}
};

/// An untyped function call expression
class FuncExpr : public Expr {
	std::string name_;  ///< Name of function called
	List<Expr> args_;   ///< Arguments to call
public:
	typedef Expr Base;
	
	FuncExpr(const std::string& name_, List<Expr>&& args_)
		: name_( name_ ), args_( move(args_) ) {}
	
	virtual Expr* clone() const {
		return new FuncExpr( name_, copy( args_ ) );
	}

	virtual void accept( Visitor& v ) const { v.visit( this ); }
	
	const std::string& name() const { return name_; }
	const List<Expr>& args() const { return args_; }

protected:
	virtual void trace(const GC& gc) const { gc << args_; }

	virtual void write(std::ostream& out) const {
		out << name_ << "(";
		for (auto& arg : args_) { out << " " << *arg; } 
		out << " )";
	}
};

/// A typed function call expression
class CallExpr : public TypedExpr {
	const FuncDecl* func_;  ///< Function called
	List<Expr> args_;       ///< Function arguments
public:
	typedef Expr Base;
	
	CallExpr( const FuncDecl* func_, List<Expr>&& args_ = List<Expr>{} )
		: func_( func_ ), args_( move(args_) ) {}
	
	virtual Expr* clone() const {
		return new CallExpr( func_, copy(args_) );
	}

	virtual void accept( Visitor& v ) const { v.visit( this ); }
	
	const FuncDecl* func() const { return func_; }
	const List<Expr>& args() const { return args_; }
	
	virtual const Type* type() const { return func_->returns(); }
	
protected:
	virtual void trace(const GC& gc) const { gc << func_ << args_; }

	virtual void write(std::ostream& out) const {
		out << func_->name();
		if ( ! func_->tag().empty() ) { out << "-" << func_->tag(); }
		out << "(";
		for (auto& arg : args_) { out << " " << *arg; }
		out << " )"; 
	}
};

/// A single element of the returned tuple from a function expression
class TupleElementExpr : public TypedExpr {
	const CallExpr* call_;
	unsigned ind_;
public:
	typedef Expr Base;
	
	TupleElementExpr( const CallExpr* call_, unsigned ind_ )
		: call_( call_ ), ind_( ind_ ) {}
	
	virtual Expr* clone() const { return new TupleElementExpr( call_, ind_ ); }

	virtual void accept( Visitor& v ) const { v.visit( this ); }
	
	const CallExpr* call() const { return call_; }
	
	unsigned ind() const { return ind_; }
	
	virtual const Type* type() const {
		return static_cast<const TupleType*>( call_->type() )->types()[ ind_ ];
	}

protected:
	virtual void trace(const GC& gc) const { gc << call_; }

	virtual void write(std::ostream& out) const {
		if ( ind_ == 0 ) {
			out << *call_ << "[0]";
		} else {
			out << "[" << ind_ << "]";
		}
	}
};
