#pragma once

#include <cstddef>
#include <functional>
#include <memory>
#include <ostream>
#include <set>
#include <string>

#include "ast.h"
#include "forall.h"

#include "data/cast.h"
#include "data/debug.h"
#include "data/list.h"
#include "data/mem.h"

/// A type declaration
class Type : public ASTNode {
	friend bool operator== (const Type&, const Type&);
	friend std::hash<Type>;
public:
	/// Create a fresh copy of this object with the same runtime type
	virtual Type* clone() const = 0;
	
	/// How many elemental types are represented by this type
	virtual unsigned size() const = 0;

	/// Creates a type of appropriate arity from the list of types
	static inline const Type* from(const List<Type>& ts);
	static inline const Type* from(List<Type>&& ts);
protected:
	/// Check this type for equality with other types
	virtual bool equals(const Type&) const = 0; 
	/// Hash this type
	virtual std::size_t hash() const = 0;
};

inline bool operator== (const Type& a, const Type& b) {
	return a.equals(b);
}

inline bool operator!= (const Type& a, const Type& b) { return !(a == b); }

namespace std {
	template<> struct hash<Type> {
		typedef Type argument_type;
		typedef std::size_t result_type;
		result_type operator() (const argument_type& t) { return t.hash(); }
	};
}

/// Represents a concrete type in the simplified resolver
class ConcType : public Type {
	friend std::hash<ConcType>;
	
	int id_;  ///< Numeric ID for type
public:
	typedef Type Base;
	
	ConcType(int id_) : id_(id_) {}
	
	Type* clone() const override { return new ConcType( id_ ); }

	bool operator== (const ConcType& that) const { return id_ == that.id_; }
	bool operator!= (const ConcType& that) const { return !(*this == that); }
	bool operator< (const ConcType& that) const { return id_ < that.id_; }
	
	int id() const { return id_; }
	
	unsigned size() const override { return 1; }

	void write(std::ostream& out, ASTNode::Print) const override { out << id_; }
	
protected:
	bool equals(const Type& obj) const override {
		const ConcType* that = as_safe<ConcType>(&obj);
		return that && (*this == *that);
	}
	
	std::size_t hash() const override { return std::hash<int>{}( id_ ); }
};

namespace std {
	template<> struct hash<ConcType> {
		typedef ConcType argument_type;
		typedef std::size_t result_type;
		result_type operator() (const argument_type& t) { return t.hash(); }
	};
}

/// Represents a named type; may be generic with parameters
class NamedType : public Type {
	std::string name_;
	List<Type> params_;

public:
	typedef Type Base;

	NamedType(const std::string& name_)	: name_(name_), params_() {}
	NamedType(const std::string& name_, List<Type>&& params_) 
		: name_(name_), params_(params_) {}

	Type* clone() const override { return new NamedType( name_, copy(params_) ); }

	bool operator== (const NamedType& that) const {
		if ( name_ != that.name_ ) return false;
		if ( params_.size() != that.params_.size() ) return false;
		for ( unsigned i = 0; i < params_.size(); ++i ) {
			if ( *params_[i] != *that.params_[i] ) return false;
		}
		return true;
	}
	bool operator!= (const NamedType& that) const { return !(*this == that); }

	const std::string& name() const { return name_; }

	const List<Type>& params() const { return params_; }

	const Type* params(unsigned i) const { return params_[i]; }
	
	unsigned size() const override { return 1; }

	void write(std::ostream& out, ASTNode::Print style) const override {
		out << "#" << name_;
		if ( ! params_.empty() ) {
			out << "<";
			auto it = params_.begin();
			(*it)->write( out, style );
			while ( ++it != params_.end() ) {
				out << " ";
				(*it)->write( out, style );
			}
			out << ">";
		}
	}

protected:
	void trace(const GC& gc) const override { gc << params_; }

	bool equals(const Type& obj) const override {
		const NamedType* that = as_safe<NamedType>(&obj);
		return that && *this == *that;
	}

	std::size_t hash() const override {
		auto h = std::hash<std::string>{}( name_ );
		for ( const Type* p : params_ ) {
			h <<= 1;
			h |= std::hash<Type>{}( *p );
		}
		return h;
	}
};

/// Representation of a polymorphic type
class PolyType : public Type {
	/// Name of the polymorphic type variable
	std::string name_;
	/// Unique ID of this type variable instantiation; 0 for type variable declaration.
	/// IDs are unique within the context of a top-level resolution call
	unsigned id_;

public:
	typedef Type Base;

	PolyType( const std::string& name_, unsigned id_ = 0 ) : name_(name_), id_(id_) {}

	Type* clone() const override { return new PolyType( name_, id_ ); }

	bool operator== (const PolyType& that) const {
		return id_ == 0 ? name_ == that.name_ : id_ == that.id_;
	}
	bool operator!= (const PolyType& that) const { return !(*this == that); }
	
	const std::string& name() const { return name_; }
	unsigned id() const { return id_; }

	unsigned size() const override { return 1; }

	void write(std::ostream& out, ASTNode::Print style) const override {
		out << name_;
		if ( id_ != 0 && style == ASTNode::Print::Default ) {
			out << "." << id_;
		}
	}

protected:
	bool equals(const Type& obj) const override {
		const PolyType* that = as_safe<PolyType>(&obj);
		return that && *this == *that; 
	}

	std::size_t hash() const override {
		return (std::hash<std::string>{}( name_ ) << 1) ^ id_;
	}
};

/// Represents the lack of a return type
class VoidType : public Type {
public:
	typedef Type Base;
	
	Type* clone() const override { return new VoidType; }
	
	unsigned size() const override { return 0; }
	
	void write(std::ostream& out, ASTNode::Print style) const override {
		switch ( style ) {
		case ASTNode::Print::InputStyle: return;
		default: out << "Void"; return;
		}
	}

protected:
	bool equals(const Type& obj) const override {
		return is<VoidType>(&obj);
	}
	
	std::size_t hash() const override { return 0; }
};

/// Represents multiple return types; must be more than one
class TupleType : public Type {
	List<Type> types_;
public:
	typedef Type Base;
	
	TupleType(const List<Type>& types_) : types_(types_) {
		assume( this->types_.size() > 1, "tuple contains multiple types" );
	}
	TupleType(List<Type>&& types_) : types_( move(types_) ) {
		assume( this->types_.size() > 1, "tuple contains multiple types" );
	}
	
	Type* clone() const override { return new TupleType( types_ ); }
	
	const List<Type>& types() const { return types_; }
	
	unsigned size() const override { return types_.size(); }

	void write(std::ostream& out, ASTNode::Print style) const override {
		auto it = types_.begin();
		(*it)->write( out, style );
		for(++it; it != types_.end(); ++it) {
			out << " ";
			(*it)->write( out, style );
		}
	}
	
protected:
	void trace(const GC& gc) const override { gc << types_; }
	
	bool equals(const Type& obj) const override {
		const TupleType* that = as_safe<TupleType>(&obj);
		if ( ! that ) return false;
		
		if ( types_.size() != that->types_.size() ) return false;
		for (unsigned i = 0; i < types_.size(); ++i) {
			if ( *types_[i] != *that->types_[i] ) return false;
		}
		return true;
	}
	
	std::size_t hash() const override {
		std::size_t r = types_.size();
		for (const auto& ty : types_) {
			r <<= 1;
			r ^= std::hash<Type>{}( *ty );
		}
		return r;
	}
};

inline const Type* Type::from(const List<Type>& ts) {
	switch ( ts.size() ) {
	case 0:  return new VoidType{};
	case 1:  return ts.front();
	default: return new TupleType{ ts };
	}
}

inline const Type* Type::from(List<Type>&& ts) {
	switch ( ts.size() ) {
	case 0:  return new VoidType{};
	case 1:  return ts.front();
	default: return new TupleType{ move(ts) };
	}
}

/// Represents a function type
class FuncType : public Type {
	List<Type> params_;   ///< Parameter types of function
	const Type* returns_; ///< Return types of function
	
public:
	typedef Type Base;

	FuncType( List<Type>&& params_, List<Type>&& returns_ )
		: params_(move(params_)), returns_( Type::from( move(returns_) ) ) {}
	
	FuncType( List<Type>&& params_, const Type* returns_ )
		: params_(move(params_)), returns_(returns_) {}
	
	Type* clone() const override {
		return new FuncType{ copy(params_), returns_ };
	}

	const List<Type>& params() const { return params_; }
	const Type* returns() const { return returns_; }

	unsigned size() const override { return 1; }

	void write(std::ostream& out, ASTNode::Print style) const override {
		out << "[ ";
		if ( returns_->size() > 0 ) {
			returns_->write( out, style );
			out << " ";
		}
		out << ":";
		for ( const Type* pType : params_ ) {
			out << " ";
			pType->write( out, style );
		}
		out << " ]";
	}
	
protected:
	void trace(const GC& gc) const override { gc << params_ << returns_; }
	
	bool equals(const Type& obj) const override {
		const FuncType* that = as_safe<FuncType>(&obj);
		if ( ! that ) return false;
		
		if ( params_.size() != that->params_.size() ) return false;
		for (unsigned i = 0; i < params_.size(); ++i) {
			if ( *params_[i] != *that->params_[i] ) return false;
		}
		if ( *returns_ != *that->returns_ ) return false;
		return true;
	}
	
	std::size_t hash() const override {
		std::size_t r = params_.size();
		for (const auto& ty : params_) {
			r <<= 1;
			r ^= std::hash<Type>{}( *ty );
		}
		r <<= 1;
		r ^= std::hash<Type>{}( *returns_ );
		return r;
	}
};
