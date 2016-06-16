#pragma once

#include <functional>
#include <memory>
#include <ostream>
#include <set>

/// A type declaration
class Type {
	friend std::ostream& operator<< (std::ostream&, const Type&);
protected:
	/// Print this object to the output stream
	virtual void write(std::ostream &) const = 0;
public:
	virtual ~Type() = default;
	
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

/// Represents a concrete type in the simplified resolver
class ConcType : public Type {
	int id_;  ///< Numeric ID for type
protected:
	virtual void write(std::ostream& out) const { out << id_; }
public:
	typedef Type Base;
	
	ConcType(int id_) : id_(id_) {}
	
	ConcType(const ConcType&) = default;
	ConcType(ConcType&&) = default;
	ConcType& operator= (const ConcType&) = default;
	ConcType& operator= (ConcType&&) = default;
	virtual ~ConcType() = default;
	
	bool operator== (const ConcType& that) const { return id_ == that.id_; }
	bool operator!= (const ConcType& that) const { return !(*this == that); }
	
	virtual bool equals(const Type& obj) const {
		const ConcType* that = dynamic_cast<const ConcType*>(&obj);
		return that && (*this == *that);
	}
	
	virtual std::size_t hash() const { return std::hash<int>()(id_); }
	
	int id() const { return id_; }
};
