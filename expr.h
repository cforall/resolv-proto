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
protected:
	virtual void write(std::ostream& out) const = 0;
public:
	virtual ~Expr() = default;
};

inline std::ostream& operator<< (std::ostream& out, const Expr& e) {
	e.write(out);
	return out;
}

/// A variable expression
class VarExpr : public Expr {
	Ref<ConcType> ty_;  ///< Type of variable expression
protected:
	virtual void write(std::ostream& out) const { out << *ty_; }
public:
	typedef Expr Base;
	
	VarExpr(const Ref<ConcType>& ty_) : ty_(ty_) {}
	
	VarExpr(const VarExpr&) = default;
	VarExpr(VarExpr&&) = default;
	VarExpr& operator= (const VarExpr&) = default;
	VarExpr& operator= (VarExpr&&) = default;
	virtual ~VarExpr() = default;
	
	Ref<ConcType> ty() const { return ty_; }
};

class FuncExpr : public Expr {
	std::string name_;  ///< Name of function called
	List<Expr> args_;   ///< Arguments to call
protected:
	virtual void write(std::ostream& out) const {
		out << name_ << "(";
		for (auto& arg : args_) { out << " " << *arg; } 
		out << " )";
	}
public:
	typedef Expr Base;
	
	FuncExpr(const std::string& name_, List<Expr>&& args_)
		: name_(name_), args_(std::move(args_)) {}
	
	FuncExpr(const FuncExpr&) = default;
	FuncExpr(FuncExpr&&) = default;
	FuncExpr& operator= (const FuncExpr&) = default;
	FuncExpr& operator= (FuncExpr&&) = default;
	virtual ~FuncExpr() = default;
	
	const std::string& name() const { return name_; }
	const List<Expr>& args() const { return args_; }
};
