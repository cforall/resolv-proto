#pragma once

#include <functional>
#include <memory>
#include <ostream>
#include <set>

#include "utility.h"

/// A type declaration
class Type {
	friend std::ostream& operator<< (std::ostream&, const Type&);
	template<typename T> friend Ptr<T> clone( const Ptr<T>& );
	template<typename T> friend Ptr<T> clone( const Shared<T>& );
	template<typename T> friend Ptr<T> clone( Ref<T> );
	friend bool operator== (const Type&, const Type&);
	friend std::hash<Type>;
public:
	virtual ~Type() = default;
	
	/// How many elemental types are represented by this type
	virtual unsigned n_types() const = 0;
protected:
	/// Print this object to the output stream
	virtual void write(std::ostream &) const = 0;
	/// Create a fresh copy of this object with the same runtime type
	virtual Ptr<Type> clone() const = 0;
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
	
/*	ConcType(const ConcType&) = default;
	ConcType(ConcType&&) = default;
	ConcType& operator= (const ConcType&) = default;
	ConcType& operator= (ConcType&&) = default;
	virtual ~ConcType() = default;
*/	
	bool operator== (const ConcType& that) const { return id_ == that.id_; }
	bool operator!= (const ConcType& that) const { return !(*this == that); }
	bool operator< (const ConcType& that) const { return id_ < that.id_; }
	
	int id() const { return id_; }
	
	virtual unsigned n_types() const { return 1; }
	
protected:
	virtual void write(std::ostream& out) const { out << id_; }
	
	virtual Ptr<Type> clone() const { return make<ConcType>( id_ ); }
	
	virtual bool equals(const Type& obj) const {
		const ConcType* that = dynamic_cast<const ConcType*>(&obj);
		return that && (*this == *that);
	}
	
	virtual std::size_t hash() const { return std::hash<int>()(id_); }
};

namespace std {
	template<> struct hash<ConcType> {
		typedef ConcType argument_type;
		typedef std::size_t result_type;
		result_type operator() (const argument_type& t) { return t.hash(); }
	};
}

/// Represents the lack of a return type
class VoidType : public Type {
public:
	typedef Type Base;
	
	virtual unsigned n_types() const { return 0; }
	
protected:
	virtual void write(std::ostream& out) const { out << "Void"; }
	
	virtual Ptr<Type> clone() const { return make<VoidType>(); }
	
	virtual bool equals(const Type& obj) const {
		return dynamic_cast<const VoidType*>(&obj) != nullptr;
	}
	
	virtual std::size_t hash() const { return 0; }
};

/// Represents multiple return types; must be more than one
class TupleType : public Type {
	List<Type, ByRef> types_;
public:
	typedef Type Base;
	
	TupleType(const List<Type, ByRef& types_)
		: types_(types_) { assert( types_.size() > 1 ); }
	
	const List<Type, ByRef>& types() const { return types_; }
	
	virtual unsigned n_types() const { return types_.size(); }
	
protected:
	 virtual void write(std::ostream& out) const {
		 auto it = types_.begin();
		 out << **it;
		 for(; it != types_.end(); ++it) { out << " " << **it; }
	 }
	 
	 virtual Ptr<Type> clone() const { return make<TupleType>( types_ ); }
	 
	 virtual bool equals(const Type& obj) const {
		 const TupleType* that = dynamic_cast<const TupleType*>(&obj);
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
}
