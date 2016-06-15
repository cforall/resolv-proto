#include <memory>
#include <ostream>
#include <sstream>
#include <string>
#include <utility>
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
	
	bool operator== (const TypeVar& that) const { return id_ == that.id_; }
	bool operator!= (const TypeVar& that) const { return !(*this == that); }
	
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
		for ( auto& t : params_ ) { ss << "." << t; }
		ss << ":r";
		for ( auto& t : returns_ ) { ss << "." << t; }
		tag_ = ss.str();
	}
	FuncDecl(const std::string& name_, const std::string& tag_, const TypeList& params_, const TypeList& returns_)
		: name_(name_), tag_(tag_), params_(params_), returns_(returns_) {}
	
	FuncDecl(const FuncDecl&) = default;
	FuncDecl(FuncDecl&&) = default;
	FuncDecl& operator= (const FuncDecl&) = default;
	FuncDecl& operator= (FuncDecl&&) = default;
	
	bool operator== (const FuncDecl& that) const { 
		return name_ == that.name_ && tag_ == that.tag_;
	}
	bool operator!= (const FuncDecl& that) const { return !(*this == that); }
	
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

/// Make an expression pointer; T must be a subclass of Expr
template<typename T, typename... Args>
inline ExprPtr make_ptr( Args&&... args ) {
	return ExprPtr{ dynamic_cast<Expr*>( new T( std::forward<Args>(args)... ) ) };
}

/// A list of expressions
typedef std::vector<ExprPtr> ExprList;

/// A variable expression
class VarExpr : public Expr {
	TypeVar ty_;
protected:
	virtual void write(std::ostream& out) const { out << ty_; }
public:
	VarExpr(TypeVar&& ty_) : ty_(std::move(ty_)) {}
	
	VarExpr(const VarExpr&) = default;
	VarExpr(VarExpr&&) = default;
	VarExpr& operator= (const VarExpr&) = default;
	VarExpr& operator= (VarExpr&&) = default;
	virtual ~VarExpr() = default;
	
	TypeVar ty() const { return ty_; }
};

class FuncExpr : public Expr {
	std::string name_;
	ExprList args_;
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
