#pragma once

#include <memory>
#include <ostream>
#include <string>
#include <vector>

#include "type.h"
#include "utility.h"

/// A resolver declaration
class Decl {
	friend std::ostream& operator<< (std::ostream&, const Decl&);
protected:
	virtual void write(std::ostream& out) const = 0;
public:
	virtual ~Decl() = default;
};

inline std::ostream& operator<< (std::ostream& out, const Decl& d) {
	d.write(out);
	return out;
}

/// A function declaration
class FuncDecl : public Decl {
	std::string name_;           ///< Name of function
	std::string tag_;            ///< Disambiguating tag for function
	List<Type, ByRef> params_;   ///< Parameter types of function
	List<Type, ByRef> returns_;  ///< Return types of function
protected:
	virtual void write(std::ostream& out) const {
		for ( auto& t : returns_ ) { out << *t << " "; }
		out << name_;
		if ( ! tag_.empty() ) { out << "-" << tag_; }
		for ( auto& t : params_ ) { out << " " << *t; }
	}
public:
	typedef Decl Base;
	
	FuncDecl(const std::string& name_, const List<Type, ByRef>& params_, 
	         const List<Type, ByRef>& returns_)
		: name_(name_), tag_(), params_(params_), returns_(returns_) {}
	FuncDecl(const std::string& name_, const std::string& tag_, 
	         const List<Type, ByRef>& params_, const List<Type, ByRef>& returns_)
		: name_(name_), tag_(tag_), params_(params_), returns_(returns_) {}
	
	FuncDecl(const FuncDecl&) = default;
	FuncDecl(FuncDecl&&) = default;
	FuncDecl& operator= (const FuncDecl&) = default;
	FuncDecl& operator= (FuncDecl&&) = default;
	virtual ~FuncDecl() = default;
	
	bool operator== (const FuncDecl& that) const { 
		return name_ == that.name_ && tag_ == that.tag_;
	}
	bool operator!= (const FuncDecl& that) const { return !(*this == that); }
	
	const std::string& name() const { return name_; }
	const std::string& tag() const { return tag_; }
	const List<Type, ByRef>& params() const { return params_; }
	const List<Type, ByRef>& returns() const { return returns_; }
};

/// A "variable" declaration
class VarDecl : public Decl {
	Ref<ConcType> ty_;  ///< Type of variable
protected:
	virtual void write(std::ostream& out) const { out << *ty_; }
public:
	typedef Decl Base;
	
	VarDecl(Ref<ConcType> ty_) : ty_(ty_) {}
	
	VarDecl(const VarDecl&) = default;
	VarDecl(VarDecl&&) = default;
	VarDecl& operator= (const VarDecl&) = default;
	VarDecl& operator= (VarDecl&&) = default;
	
	bool operator== (const VarDecl& that) const { return ty_ == that.ty_; }
	bool operator!= (const VarDecl& that) const { return !(*this == that); }
	
	Ref<ConcType> ty() const { return ty_; }
};
