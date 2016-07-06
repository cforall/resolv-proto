#pragma once

#include <memory>
#include <ostream>
#include <string>
#include <utility>
#include <vector>

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
}

/// A variable expression
class VarExpr : public Expr {
	Ref<ConcType> ty_;  ///< Type of variable expression
public:
	typedef Expr Base;
	
	VarExpr(const Ref<ConcType>& ty_) : ty_( ty_ ) {}
	
	VarExpr(const VarExpr&) = default;
	VarExpr(VarExpr&&) = default;
	VarExpr& operator= (const VarExpr&) = default;
	VarExpr& operator= (VarExpr&&) = default;
	virtual ~VarExpr() = default;
	
	Ref<ConcType> ty() const { return ty_; }

protected:
	virtual void write(std::ostream& out) const { out << *ty_; }
	
	virtual Ptr<Expr> clone() const { return make<VarExpr>( ty_ ); }
};

class FuncExpr : public Expr {
	std::string name_;           ///< Name of function called
	List<Expr, ByShared> args_;  ///< Arguments to call
public:
	typedef Expr Base;
	
	FuncExpr(const std::string& name_, List<Expr, ByShared>&& args_)
		: name_( name_ ), args_( move(args_) ) {}
	
	FuncExpr(const FuncExpr&) = default;
	FuncExpr(FuncExpr&&) = default;
	FuncExpr& operator= (const FuncExpr&) = default;
	FuncExpr& operator= (FuncExpr&&) = default;
	virtual ~FuncExpr() = default;
	
	const std::string& name() const { return name_; }
	const List<Expr, ByShared>& args() const { return args_; }

protected:
	virtual void write(std::ostream& out) const {
		out << name_ << "(";
		for (auto& arg : args_) { out << " " << *arg; } 
		out << " )";
	}
	
	virtual Ptr<Expr> clone() const {
		return make<FuncExpr>( name_, cloneAll( args_ ) );
	}
};
