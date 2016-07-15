#pragma once

#include <memory>
#include <ostream>
#include <string>
#include <utility>
#include <vector>

#include "conversion.h"
#include "decl.h"
#include "type.h"
#include "utility.h"

/// A resolvable expression
class Expr {
	friend std::ostream& operator<< (std::ostream&, const Expr&);
public:
	virtual ~Expr() = default;
	
	virtual Ptr<Expr> clone() const = 0;
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
	virtual Brw<Type> type() const = 0; 
};

/// A variable expression
class VarExpr : public TypedExpr {
	Brw<ConcType> ty_;  ///< Type of variable expression
public:
	typedef Expr Base;
	
	VarExpr(Brw<ConcType> ty_) : ty_( ty_ ) {}
	
	virtual Ptr<Expr> clone() const { return make<VarExpr>( ty_ ); }
	
	virtual Brw<ConcType> ty() const { return ty_; }
	
	virtual Brw<Type> type() const { return ty_; }

protected:
	virtual void write(std::ostream& out) const { out << *ty_; }
};

/// A cast expression
class CastExpr : public TypedExpr {
	Shared<Expr> arg_;      ///< Argument to the cast
	Brw<Conversion> cast_;  ///< Conversion applied
public:
	typedef Expr Base;
	
	CastExpr(const Shared<Expr>& arg_, Brw<Conversion> cast_)
		: arg_( arg_ ), cast_( cast_ ) {}
	
	virtual Ptr<Expr> clone() const { return make<CastExpr>( arg_, cast_ ); }
	
	const Shared<Expr>& arg() const { return arg_; }
	Brw<Conversion> cast() const { return cast_; }
	
	virtual Brw<Type> type() const { return cast_->to->type; }
	
protected:
	virtual void write(std::ostream& out) const {
		out << *arg_ << " => " << *type();
	}
};

/// An untyped function call expression
class FuncExpr : public Expr {
	std::string name_;           ///< Name of function called
	List<Expr, ByShared> args_;  ///< Arguments to call
public:
	typedef Expr Base;
	
	FuncExpr(const std::string& name_, List<Expr, ByShared>&& args_)
		: name_( name_ ), args_( move(args_) ) {}
	
	virtual Ptr<Expr> clone() const {
		return make<FuncExpr>( name_, copy( args_ ) );
	}
	
	const std::string& name() const { return name_; }
	const List<Expr, ByShared>& args() const { return args_; }

protected:
	virtual void write(std::ostream& out) const {
		out << name_ << "(";
		for (auto& arg : args_) { out << " " << *arg; } 
		out << " )";
	}
};

/// A typed function call expression
class CallExpr : public TypedExpr {
	Brw<FuncDecl> func_;         ///< Function called
	List<Expr, ByShared> args_;  ///< Function arguments
public:
	typedef Expr Base;
	
	CallExpr( Brw<FuncDecl> func_, 
	          List<Expr, ByShared>&& args_ = List<Expr, ByShared>{} )
		: func_( func_ ), args_( move(args_) ) {}
	
	virtual Ptr<Expr> clone() const {
		return make<CallExpr>( func_, copy( args_ ) );
	}
	
	Brw<FuncDecl> func() const { return func_; }
	const List<Expr, ByShared>& args() const { return args_; }
	
	virtual Brw<Type> type() const { return func_->returns(); }
	
protected:
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
	Shared<CallExpr> call_;
	unsigned ind_;
public:
	typedef Expr Base;
	
	TupleElementExpr( const Shared<CallExpr>& call_, unsigned ind_ )
		: call_( call_ ), ind_( ind_ ) {}
	
	virtual Ptr<Expr> clone() const { return make<TupleElementExpr>( call_, ind_ ); }
	
	const Shared<CallExpr>& call() const { return call_; }
	
	unsigned ind() const { return ind_; }
	
	virtual Brw<Type> type() const {
		return brw_as<TupleType>( call_->type() )->types()[ ind_ ];
	}

protected:
	virtual void write(std::ostream& out) const {
		if ( ind_ == 0 ) {
			out << *call_ << "[0]";
		} else {
			out << "[" << ind_ << "]";
		}
	}
};
