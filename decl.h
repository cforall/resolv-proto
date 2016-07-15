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
public:
	virtual Ptr<Decl> clone() const = 0;
	
	virtual ~Decl() = default;
protected:
	virtual void write(std::ostream& out) const = 0;
};

inline std::ostream& operator<< (std::ostream& out, const Decl& d) {
	d.write(out);
	return out;
}

/// A function declaration
class FuncDecl final : public Decl {
	std::string name_;          ///< Name of function
	std::string tag_;           ///< Disambiguating tag for function
	List<Type, ByBrw> params_;  ///< Parameter types of function
	Ptr<Type> returns_;         ///< Return types of function
	
	/// Generate appropriate return type from return list
	static Ptr<Type> gen_returns(const List<Type, ByBrw>& rs) {
		switch ( rs.size() ) {
		case 0:
			return make<VoidType>();
		case 1:
			return rs.front()->clone();
		default:
			return make<TupleType>( rs );
		}
	}
	
public:
	typedef Decl Base;
	
	FuncDecl(const std::string& name_, const std::string& tag_, 
	         const List<Type, ByBrw>& params_, Brw<Type> returns_)
		: name_(name_), tag_(tag_), params_(params_), 
		  returns_( returns_->clone() ) {}
	
	FuncDecl(const std::string& name_, const List<Type, ByBrw>& params_, 
	         const List<Type, ByBrw>& returns_)
		: name_(name_), tag_(), params_(params_), 
		  returns_( gen_returns( returns_ ) ) {}
	
	FuncDecl(const std::string& name_, const std::string& tag_, 
	         const List<Type, ByBrw>& params_, const List<Type, ByBrw>& returns_)
		: name_(name_), tag_(tag_), params_(params_), 
		  returns_( gen_returns( returns_ ) ) {}
	
	virtual Ptr<Decl> clone() const {
		return make<FuncDecl>( name_, tag_, params_, brw(returns_) );
	}
	
	bool operator== (const FuncDecl& that) const { 
		return name_ == that.name_ && tag_ == that.tag_;
	}
	bool operator!= (const FuncDecl& that) const { return !(*this == that); }
	
	const std::string& name() const { return name_; }
	const std::string& tag() const { return tag_; }
	const List<Type, ByBrw>& params() const { return params_; }
	Brw<Type> returns() const { return brw(returns_); }

protected:
	virtual void write(std::ostream& out) const {
		out << *returns_ << " ";
		out << name_;
		if ( ! tag_.empty() ) { out << "-" << tag_; }
		for ( auto& t : params_ ) { out << " " << *t; }
	}
};

/// A "variable" declaration
class VarDecl final : public Decl {
	Brw<ConcType> ty_;  ///< Type of variable
public:
	typedef Decl Base;
	
	VarDecl(Brw<ConcType> ty_) : ty_(ty_) {}
	
	virtual Ptr<Decl> clone() const { return make<VarDecl>( ty_ ); }
	
	bool operator== (const VarDecl& that) const { return ty_ == that.ty_; }
	bool operator!= (const VarDecl& that) const { return !(*this == that); }
	
	Brw<ConcType> ty() const { return ty_; }
protected:
	virtual void write(std::ostream& out) const { out << *ty_; }
};
