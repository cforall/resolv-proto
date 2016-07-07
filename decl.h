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
	template<typename T> friend Ptr<T> clone( const Ptr<T>& );
	template<typename T> friend Ptr<T> clone( const Shared<T>& );
	template<typename T> friend Ptr<T> clone( Ref<T> );
public:
	virtual ~Decl() = default;
protected:
	virtual void write(std::ostream& out) const = 0;
	virtual Ptr<Decl> clone() const = 0;
};

inline std::ostream& operator<< (std::ostream& out, const Decl& d) {
	d.write(out);
	return out;
}

/// A function declaration
class FuncDecl final : public Decl {
	std::string name_;          ///< Name of function
	std::string tag_;           ///< Disambiguating tag for function
	List<Type, ByRef> params_;  ///< Parameter types of function
	Ptr<Type> returns_;         ///< Return types of function
	
	/// Generate appropriate return type from return list
	static Ptr<Type> gen_returns(const List<Type, ByRef>& rs) {
		switch ( rs.size() ) {
		case 0:
			return make<VoidType>();
		case 1:
			return clone(rs.front());
		default:
			return make<TupleType>( rs );
		}
	}
	
public:
	typedef Decl Base;
	
	FuncDecl(const std::string& name_, const std::string& tag_, 
	         const List<Type, ByRef>& params_, Ref<Type> returns_)
		: name_(name_), tag_(tag_), params_(params_), 
		  returns_( clone( returns_ ) ) {}
	
	FuncDecl(const std::string& name_, const List<Type, ByRef>& params_, 
	         const List<Type, ByRef>& returns_)
		: name_(name_), tag_(), params_(params_), 
		  returns_( gen_returns( returns_ ) ) {}
	
	FuncDecl(const std::string& name_, const std::string& tag_, 
	         const List<Type, ByRef>& params_, const List<Type, ByRef>& returns_)
		: name_(name_), tag_(tag_), params_(params_), 
		  returns_( gen_returns( returns_ ) ) {}
	
/*	FuncDecl(const FuncDecl&) = default;
	FuncDecl(FuncDecl&&) = default;
	FuncDecl& operator= (const FuncDecl&) = default;
	FuncDecl& operator= (FuncDecl&&) = default;
	~FuncDecl() = default;
*/	
	bool operator== (const FuncDecl& that) const { 
		return name_ == that.name_ && tag_ == that.tag_;
	}
	bool operator!= (const FuncDecl& that) const { return !(*this == that); }
	
	const std::string& name() const { return name_; }
	const std::string& tag() const { return tag_; }
	const List<Type, ByRef>& params() const { return params_; }
	Ref<Type> returns() const { return ref(returns_); }

protected:
	virtual void write(std::ostream& out) const {
		out << *returns_ << " ";
		out << name_;
		if ( ! tag_.empty() ) { out << "-" << tag_; }
		for ( auto& t : params_ ) { out << " " << *t; }
	}
	
	virtual Ptr<Decl> clone() const {
		return make<FuncDecl>( name_, tag_, params_, ref(returns_) );
	}
};

/// A "variable" declaration
class VarDecl final : public Decl {
	Ref<ConcType> ty_;  ///< Type of variable
public:
	typedef Decl Base;
	
	VarDecl(Ref<ConcType> ty_) : ty_(ty_) {}
	
/*	VarDecl(const VarDecl&) = default;
	VarDecl(VarDecl&&) = default;
	VarDecl& operator= (const VarDecl&) = default;
	VarDecl& operator= (VarDecl&&) = default;
	~VarDecl() = default;
*/	
	bool operator== (const VarDecl& that) const { return ty_ == that.ty_; }
	bool operator!= (const VarDecl& that) const { return !(*this == that); }
	
	Ref<ConcType> ty() const { return ty_; }
protected:
	virtual void write(std::ostream& out) const { out << *ty_; }
	
	virtual Ptr<Decl> clone() const { return make<VarDecl>( ty_ ); }
};
