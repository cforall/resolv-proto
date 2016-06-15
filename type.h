#pragma once

#include <vector>

/// Represents a type in the simplified resolver
class Type {
	int id_;  ///< Numeric ID for type
public:
	Type(int id_) : id_(id_) {}
	
	Type(const Type&) = default;
	Type(Type&&) = default;
	Type& operator= (const Type&) = default;
	Type& operator= (Type&&) = default;
	
	bool operator== (const Type& that) const { return id_ == that.id_; }
	bool operator!= (const Type& that) const { return !(*this == that); }
	
	int id() const { return id_; }
};

inline std::ostream& operator<< (std::ostream& out, const Type& t) {
	return out << t.id();
}

/// A list of types
typedef std::vector<Type> TypeList;
