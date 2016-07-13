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
	template<typename T> friend Ptr<T> clone( const Ptr<T>& );
	template<typename T> friend Ptr<T> clone( const Shared<T>& );
	template<typename T> friend Ptr<T> clone( Ref<T> );
public:
	virtual ~Expr() = default;
protected:
	virtual void write(std::ostream& out) const = 0;
	virtual Ptr<Expr> clone() const = 0;
};

inline std::ostream& operator<< (std::ostream& out, const Expr& e) {
	e.write(out);
	return out;
};

/// An expression that is already typed
class TypedExpr : public Expr {
public:
	/// Get the resolved type for this expression
	virtual Ref<Type> type() const = 0; 
};

/// A variable expression
class VarExpr : public TypedExpr {
	Ref<ConcType> ty_;  ///< Type of variable expression
public:
	typedef Expr Base;
	
	VarExpr(Ref<ConcType> ty_) : ty_( ty_ ) {}
	
/*	VarExpr(const VarExpr&) = default;
	VarExpr(VarExpr&&) = default;
	VarExpr& operator= (const VarExpr&) = default;
	VarExpr& operator= (VarExpr&&) = default;
	virtual ~VarExpr() = default;
*/	
	virtual Ref<ConcType> ty() const { return ty_; }
	
	virtual Ref<Type> type() const { return ty_; }

protected:
	virtual void write(std::ostream& out) const { out << *ty_; }
	
	virtual Ptr<Expr> clone() const { return make<VarExpr>( ty_ ); }
};

/// A cast expression
class CastExpr : public TypedExpr {
	Shared<Expr> arg_;      ///< Argument to the cast
	Ref<Conversion> cast_;  ///< Conversion applied
public:
	typedef Expr Base;
	
	CastExpr(const Shared<Expr>& arg_, Ref<Conversion> cast_)
		: arg_( arg_ ), cast_( cast_ ) {}
	
/*	CastExpr(const CastExpr&) = default;
	CastExpr(CastExpr&&) = default;
	CastExpr& operator= (const CastExpr&) = default;
	CastExpr& operator= (CastExpr&&) = default;
	virtual ~CastExpr() = default;
*/	
	const Shared<Expr>& arg() const { return arg_; }
	Ref<Conversion> cast() const { return cast_; }
	
	virtual Ref<Type> type() const { return cast->to->type; }
	
protected:
	virtual void write(std::ostream& out) const {
		out << *arg_ << " => " << *type();
	}
	
	virtual Ptr<Expr> clone() const { return make<CastExpr>( arg_, cast_ ); }
};

/// An untyped function call expression
class FuncExpr : public Expr {
	std::string name_;           ///< Name of function called
	List<Expr, ByShared> args_;  ///< Arguments to call
public:
	typedef Expr Base;
	
	FuncExpr(const std::string& name_, List<Expr, ByShared>&& args_)
		: name_( name_ ), args_( move(args_) ) {}
	
/*	FuncExpr(const FuncExpr&) = default;
	FuncExpr(FuncExpr&&) = default;
	FuncExpr& operator= (const FuncExpr&) = default;
	FuncExpr& operator= (FuncExpr&&) = default;
	virtual ~FuncExpr() = default;
*/	
	const std::string& name() const { return name_; }
	const List<Expr, ByShared>& args() const { return args_; }

protected:
	virtual void write(std::ostream& out) const {
		out << name_ << "(";
		for (auto& arg : args_) { out << " " << *arg; } 
		out << " )";
	}
	
	virtual Ptr<Expr> clone() const {
		return make<FuncExpr>( name_, copy( args_ ) );
	}
};

/// A typed function call expression
class CallExpr : public TypedExpr {
	Ref<FuncDecl> func_;         ///< Function called
	List<Expr, ByShared> args_;  ///< Function arguments
public:
	typedef Expr Base;
	
	CallExpr(Ref<FuncDecl> func_, List<Expr, ByShared>&& args_)
		: func_( func_ ), args_( move(args_) ) {}
	
	Ref<FuncDecl> func() const { return func_; }
	const List<Expr, ByShared>& args() const { return args_; }
	
	virtual Ref<Type> type() const { return func_->returns(); }
	
protected:
	virtual void write(std::ostream& out) const {
		out << func_->name();
		if ( ! func_->tag().empty() ) { out << "-" << func_->tag(); }
		out << "(";
		for (auto& arg : args_) { out << " " << *arg; }
		out << " )"; 
	}
	
	virtual Ptr<Expr> clone() const {
		return make<CallExpr>( func_, copy( args_ ) );
	}
};

/// An ambiguous interpretation with a given type
class AmbiguousExpr : public TypedExpr {
	Ref<Type> type_;
public:
	typedef Expr Base;
	
	AmbiguousExpr( Ref<Type> type_ ) : type_( type_ ) {}
	
	virtual Ref<Type> type() const { return type_; }

protected:
	 virtual void write(std::ostream& out) const {
		 out << "<ambiguous expression of type " << *type_ << ">";
	 }
	 
	 virtual Ptr<Expr> clone() const { return make<AmbiguousExpr>( type_ ); }
};

/// A single element of the returned tuple from a function expression
class TupleElementExpr : public TypedExpr {
	Shared<CallExpr> call_;
	unsigned ind_;
public:
	typedef Expr Base;
	
	TupleElementExpr( const Shared<CallExpr>& call_, unsigned ind_ )
		: call_( call_ ), ind_( ind_ ) {}
	
	const Shared<CallExpr>& call() const { return call_; }
	
	unsigned ind() const { return ind; }
	
	virtual Ref<Type> type() const {
		return ref_as<TupleType>( call_->type() )->types[ ind_ ];
	}

protected:
	virtual void write(std::ostream& out) const {
		if ( ind_ == 0 ) {
			out << *call_ << "[0]";
		} else {
			out << "[" << ind_ << "]";
		}
	}
	
	virtual Ptr<Expr> clone() const { return make<TupleElementExpr>( call_, ind_ ); }
};
