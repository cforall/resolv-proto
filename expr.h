#pragma once

#include <memory>
#include <ostream>
#include <string>
#include <utility>
#include <vector>

#include "type.h"

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

/// A pointer to an expression
typedef std::unique_ptr<Expr> ExprPtr;

/// A list of expressions
typedef std::vector<ExprPtr> ExprList;

/// A variable expression
class VarExpr : public Expr {
	Type ty_;  ///< Type of variable expression
protected:
	virtual void write(std::ostream& out) const { out << ty_; }
public:
	VarExpr(Type&& ty_) : ty_(std::move(ty_)) {}
	
	VarExpr(const VarExpr&) = default;
	VarExpr(VarExpr&&) = default;
	VarExpr& operator= (const VarExpr&) = default;
	VarExpr& operator= (VarExpr&&) = default;
	virtual ~VarExpr() = default;
	
	Type ty() const { return ty_; }
};

class FuncExpr : public Expr {
	std::string name_;  ///< Name of function called
	ExprList args_;     ///< Arguments to call
protected:
	virtual void write(std::ostream& out) const {
		out << name_ << "(";
		for (auto& arg : args_) { out << " " << *arg; } 
		out << " )";
	}
public:
	FuncExpr(std::string&& name_, ExprList&& args_)
		: name_(std::move(name_)), args_(std::move(args_)) {}
	
	FuncExpr(const FuncExpr&) = default;
	FuncExpr(FuncExpr&&) = default;
	FuncExpr& operator= (const FuncExpr&) = default;
	FuncExpr& operator= (FuncExpr&&) = default;
	virtual ~FuncExpr() = default;
	
	const std::string& name() const { return name_; }
	const ExprList& args() const { return args_; }
};
