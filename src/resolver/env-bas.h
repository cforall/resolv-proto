#include <ostream>
#include <vector>
#include <unordered_map>
#include <utility>

#include "data/cast.h"
#include "data/gc.h"
#include "data/mem.h"
#include "data/set.h"

class Decl;
class Env;
class PolyType;
class Type;
class TypedExpr;

/// Class of equivalent type variables, along with optional concrete bound type
struct TypeClass {
	Set<PolyType> vars;  ///< Equivalent polymorphic types
	const Type* bound;   ///< Type which the class binds to

	TypeClass( Set<PolyType>&& vars = Set<PolyType>{}, const Type* bound = nullptr )
		: vars(move(vars)), bound(bound) {}
};

std::ostream& operator<< (std::ostream&, const TypeClass&);

class ClassRef {
	friend Env;

	const Env* env;   ///< Containing environment
	std::size_t ind;  ///< Index into that environment's class storage
public:
	ClassRef() : env(nullptr), ind(0) {}
	ClassRef( const Env* env, std::size_t ind ) : env(env), ind(ind) {}

	/// Gets the representative element of the referenced typeclass
	inline const PolyType* get_root() const;

	/// Ensures root is still representative element of this typeclass.
	/// Not implemented for this variant.
	void update_root() {}

	/// Gets the bound type of the referenced typeclass, nullptr for none such.
	inline const Type* get_bound() const;

	/// Get referenced typeclass
	inline const TypeClass& operator* () const;
	const TypeClass* operator-> () const { return &(this->operator*()); }

	/// Check that there is a referenced typeclass
	explicit operator bool() const { return env != nullptr; }

	bool operator== (const ClassRef& o) const { return env == o.env && ind == o.ind; }
	bool operator!= (const ClassRef& o) const { return !(*this == o); }
};

class Env final {
	friend ClassRef;
	friend const GC& operator<< (const GC&, const Env&);
	friend std::ostream& operator<< (std::ostream&, const Env&);

public:
	/// Backing storage for typeclasses
	using Storage = std::vector<TypeClass>;
	/// Assertions known by this environment
	using AssertionMap = std::unordered_map<const Decl*, const TypedExpr*>;

private:
	Storage classes;     ///< Backing storage for typeclasses
	AssertionMap assns;  ///< Backing storage for assertions
	bool is_valid;       ///< Is this environment valid?

#if defined RP_DEBUG && RP_DEBUG >= 1
	void verify() const;
#endif

	Env(bool v) : classes(), assns(), is_valid(v) {}

	/// Inserts a new typeclass, consisting of a single var (should not be present)
	void insert( const PolyType* v ) {
		classes.emplace_back( Set<PolyType>{ v } );
	}

	/// Makes cboudn the bound of r in this environment, returning false if incompatible
	bool mergeBound( ClassRef& r, const Type* cbound );

	/// Merges s into r; returns false if fails due to contradictory bindings
	bool mergeClasses( ClassRef& r, ClassRef s );

public:
	const Storage& getClasses() const { return classes; }
	const AssertionMap& getAssertions() const { return assns; }

	/// true iff t occurs in vars, recursively expanded according to this environment
	bool occursIn( const Set<PolyType>& vars, const Type* t ) const;

	/// true iff t occurs in var, recursively expanded according to this environment
	bool occursIn( const PolyType* var, const Type* t ) const {
		return occursIn( Set<PolyType>{ var }, t );
	}

	/// Finds the binding reference for this polytype, { nullptr } for none such.
	ClassRef findRef( const PolyType* var ) const {
		for ( unsigned i = 0; i < classes.size(); ++i ) {
			if ( classes[i].vars.count( var ) ) return { this, i };
		}
		return {};
	}

	Env() : classes(), assns(), is_valid(true) {}
	Env( const Env& ) = default;
	Env& operator= ( const Env& ) = default;
	~Env() = default;

	/// Builds an empty environment
	static Env invalid() { return Env{false}; }

	/// Swap with other environment
	void swap( Env& o ) {
		std::swap(classes, o.classes);
		std::swap(assns, o.assns);
	}

	/// true if this environment is valid
	explicit operator bool() const { return is_valid; }
	bool valid() const { return is_valid; }
	
	/// Check if this environment is empty
	bool empty() const { return classes.empty() && assns.empty(); }

	/// Inserts a type variable if it doesn't already exist in the environment
	/// Returns false if already present
	bool insertVar( const PolyType* var ) {
		if ( findRef( var ) ) return false;
		insert( var );
		return true;
	}

	/// Gets the type-class for a variable, creating it if needed
	ClassRef getClass( const PolyType* var ) {
		ClassRef r = findRef( var );
		if ( r ) return r;
		
		insert( var );
		return { this, classes.size() - 1 };
	}

	/// Finds an assertion in this environment, returns null if none
	const TypedExpr* findAssertion( const Decl* f ) const {
		auto it = assns.find( f );
		return it == assns.end() ? nullptr : it->second;
	}

	/// Binds this class to the given type; class should be currently unbound
	/// Returns false if would create recursive loop.
	bool bindType( ClassRef& r, const Type* sub ) {
		if ( occursIn( r->vars, sub ) ) return false;
		TypeClass& rb = as_non_const(*r);
		rb.bound = sub;
		return true;
	}

	/// Binds the type variable into the given class; returns false if the type variable is 
	/// incompatibly bound.
	bool bindVar( ClassRef& r, const PolyType* var ) {
		ClassRef vr = findRef( var );
		if ( vr == r ) return true;  // exit early if already bound
		if ( vr ) {
			// merge variable's existing class into this one
			return mergeClasses( r, vr );
		} else {
			// add unbound variable to class if no cycle
			if ( occursIn( var, r->bound ) ) return false;
			as_non_const(r->vars).insert( var );
			return true;
		}
	}

	/// Binds an assertion in this environment. `f` should be currently unbound
	void bindAssertion( const Decl* f, const TypedExpr* assn ) {
		assns.emplace( f, assn );
	}

	/// Merges the target environment with this one.
	/// Returns false if fails, but does NOT roll back partial changes.
	bool merge( const Env& o );

	/// Gets a list of unbound type classes in this environment
	std::vector<TypeClass> getUnbound() const {
		std::vector<TypeClass> out;
		for ( const TypeClass& c : classes ) if ( ! c.bound ) out.push_back( c );
		return out;
	}

	/// Replaces type with bound value, if a substitution exists
	const Type* replace( const PolyType* pty ) const {
		ClassRef r = findRef( pty );
		if ( r && r->bound ) return r->bound;
		return as<Type>(pty);
	}

	/// Replaces type with bound value, if type is a PolyType and a substitution exists
	const Type* replace( const Type* ty ) const {
		if ( is<PolyType>(ty) ) {
			ClassRef r = findRef( as<PolyType>(ty) );
			if ( r && r->bound ) return r->bound;
		}
		return ty;
	}

private:
	std::ostream& write(std::ostream& out) const;
};

inline std::ostream& operator<< (std::ostream& out, const Env& env) {
	return env.write( out );
}

const PolyType* ClassRef::get_root() const { return *env->classes[ind].vars.begin(); }

const Type* ClassRef::get_bound() const { return env->classes[ind].bound; }

const TypeClass& ClassRef::operator* () const { return env->classes[ind]; }

inline void swap( Env& a, Env& b ) { a.swap( b ); }