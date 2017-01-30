#pragma once

#include <cassert>
#include <cstddef>
#include <functional>
#include <memory>
#include <ostream>
#include <set>
#include <string>

#include "ast.h"

#include "binding.h"
#include "data.h"

/// A type declaration
class Type : public ASTNode {
	friend std::ostream& operator<< (std::ostream&, const Type&);
	friend bool operator== (const Type&, const Type&);
	friend std::hash<Type>;
public:
	/// Create a fresh copy of this object with the same runtime type
	virtual Type* clone() const = 0;
	
	/// How many elemental types are represented by this type
	virtual unsigned size() const = 0;
protected:
	/// Print this object to the output stream
	virtual void write(std::ostream &) const = 0;
	/// Check this type for equality with other types
	virtual bool equals(const Type&) const = 0; 
	/// Hash this type
	virtual std::size_t hash() const = 0;
};

inline std::ostream& operator<< (std::ostream& out, const Type& t) {
	t.write(out);
	return out;
}

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
	
	virtual Type* clone() const { return new ConcType( id_ ); }

	bool operator== (const ConcType& that) const { return id_ == that.id_; }
	bool operator!= (const ConcType& that) const { return !(*this == that); }
	bool operator< (const ConcType& that) const { return id_ < that.id_; }
	
	int id() const { return id_; }
	
	virtual unsigned size() const { return 1; }
	
protected:
	virtual void write(std::ostream& out) const { out << id_; }
	
	virtual bool equals(const Type& obj) const {
		const ConcType* that = as_safe<ConcType>(&obj);
		return that && (*this == *that);
	}
	
	virtual std::size_t hash() const { return std::hash<int>{}( id_ ); }
};

namespace std {
	template<> struct hash<ConcType> {
		typedef ConcType argument_type;
		typedef std::size_t result_type;
		result_type operator() (const argument_type& t) { return t.hash(); }
	};
}

/// Represents a named concrete type
class NamedType : public Type {
	std::string name_;

public:
	typedef Type Base;

	NamedType(const std::string& name_)	: name_(name_) {}

	virtual Type* clone() const { return new NamedType( name_ ); }

	bool operator== (const NamedType& that) const {
		return name_ == that.name_;
	}
	bool operator!= (const NamedType& that) const { return !(*this == that); }

	const std::string& name() const { return name_; }
	
	virtual unsigned size() const { return 1; }

protected:
	virtual void write(std::ostream& out) const {
		out << "#" << name_;
	}

	virtual bool equals(const Type& obj) const {
		const NamedType* that = as_safe<NamedType>(&obj);
		return that && *this == *that;
	}

	virtual std::size_t hash() const {
		return std::hash<std::string>{}( name_ );
	}
};

class TypeBinding;

/// Representation of a polymorphic type
class PolyType : public Type {
	std::string name_;        ///< Name of the polymorphic type variable
	const TypeBinding* src_;  ///< Type binding this type belongs to
public:
	typedef Type Base;

	PolyType( const std::string& name_, const TypeBinding* src_ = nullptr )
		: name_(name_), src_(src_) {}

	virtual Type* clone() const { return new PolyType( name_, src_ ); }

	/// Clones bound to a new type binding
	PolyType* clone_bound( const TypeBinding* new_src ) const {
		return new PolyType( name_, new_src );
	}

	bool operator== (const PolyType& that) const {
		return src_ == that.src_ && name_ == that.name_;
	}
	bool operator!= (const PolyType& that) const { return !(*this == that); }
	
	const std::string& name() const { return name_; }
	const TypeBinding* src() const { return src_; }

	virtual unsigned size() const { return 1; }

protected:
	virtual void trace(const GC& gc) const { gc << src_; }

	virtual void write(std::ostream& out) const {
		out << name_;
		if ( src_ ) { out << "@" << src_->name; }
	}

	virtual bool equals(const Type& obj) const {
		const PolyType* that = as_safe<PolyType>(&obj);
		return that && *this == *that; 
	}

	virtual std::size_t hash() const {
		return (std::hash<std::string>{}( name_ ) << 1) ^ std::hash<const TypeBinding*>{}( src_ );
	}
};

/// Represents the lack of a return type
class VoidType : public Type {
public:
	typedef Type Base;
	
	virtual Type* clone() const { return new VoidType; }
	
	virtual unsigned size() const { return 0; }
	
protected:
	virtual void write(std::ostream& out) const { out << "Void"; }
	
	virtual bool equals(const Type& obj) const {
		return is<VoidType>(&obj);
	}
	
	virtual std::size_t hash() const { return 0; }
};

/// Represents multiple return types; must be more than one
class TupleType : public Type {
	List<Type> types_;
public:
	typedef Type Base;
	
	TupleType(const List<Type>& types_)
		: types_(types_) { assert( this->types_.size() > 1 ); }
	TupleType(List<Type>&& types_)
		: types_( move(types_) ) { assert( this->types_.size() > 1 ); }
	
	virtual Type* clone() const { return new TupleType( types_ ); }
	
	const List<Type>& types() const { return types_; }
	
	virtual unsigned size() const { return types_.size(); }
	
protected:
	virtual void trace(const GC& gc) const { gc << types_; }

	virtual void write(std::ostream& out) const {
		auto it = types_.begin();
		out << **it;
		for(++it; it != types_.end(); ++it) { out << " " << **it; }
	}
	
	virtual bool equals(const Type& obj) const {
		const TupleType* that = as_safe<TupleType>(&obj);
		if ( ! that ) return false;
		
		if ( types_.size() != that->types_.size() ) return false;
		for (unsigned i = 0; i < types_.size(); ++i) {
			if ( *types_[i] != *that->types_[i] ) return false;
		}
		return true;
	}
	
	virtual std::size_t hash() const {
		std::size_t r = types_.size();
		for (const auto& ty : types_) {
			r <<= 1;
			r ^= std::hash<Type>{}( *ty );
		}
		return r;
	}
};
