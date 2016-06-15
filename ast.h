#include <ostream>
#include <sstream>
#include <string>
#include <vector>

/// Represents a type (or a variable of that type)
class TypeVar {
	int id_;  ///< Numeric ID for type
public:
	TypeVar(int id_) : id_(id_) {}
	TypeVar(const TypeVar&) = default;
	TypeVar(TypeVar&&) = default;
	TypeVar& operator= (const TypeVar&) = default;
	TypeVar& operator= (TypeVar&&) = default;
	
	bool operator== (const TypeVar& that) { return id_ == that.id_; }
	bool operator!= (const TypeVar& that) { return !(*this == that); }
	
	int id() const { return id_; }
};

inline std::ostream& operator<< (std::ostream& out, const TypeVar& t) {
	return out << t.id();
}

/// A list of types
typedef std::vector<TypeVar> TypeList;

/// A function declaration
class FuncDecl {
	std::string name_;  ///< Name of function
	std::string tag_;   ///< Disambiguating tag for function
	TypeList params_;   ///< Parameter types of function
	TypeList returns_;  ///< Return types of function
public:
	FuncDecl(const std::string& name_, const TypeList& params_, const TypeList& returns_)
		: name_(name_), params_(params_), returns_(returns_) {
		std::stringstream ss;
		ss << "p";
		for ( auto& t : params_ ) { ss << "_" << t; }
		ss << "_r";
		for ( auto& t : returns_ ) { ss << "_" << t; }
		tag_ = ss.str();
	}
	FuncDecl(const std::string& name_, const std::string& tag_, const TypeList& params_, const TypeList& returns_)
		: name_(name_), tag_(tag_), params_(params_), returns_(returns_) {}
	FuncDecl(const FuncDecl&) = default;
	FuncDecl(FuncDecl&&) = default;
	FuncDecl& operator= (const FuncDecl&) = default;
	FuncDecl& operator= (FuncDecl&&) = default;
	
	bool operator== (const FuncDecl& that) { return name_ == that.name_ && tag_ == that.tag_; }
	bool operator!= (const FuncDecl& that) { return !(*this == that); }
	
	const std::string& name() const { return name_; }
	const std::string& tag() const { return tag_; }
	const TypeList& params() const { return params_; }
	const TypeList& returns() const { return returns_; }
};

inline std::ostream& operator<< (std::ostream& out, const FuncDecl& f) {
	for ( auto& t : f.returns() ) { out << t << " "; }
	out << f.name() << "-" << f.tag();
	for ( auto& t : f.params() ) { out << " " << t; }
	return out;
}

/// A list of function declarations
typedef std::vector<FuncDecl> FuncList;
