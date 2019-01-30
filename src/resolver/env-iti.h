#include <algorithm>
#include <cstddef>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "cost.h"

#include "data/cast.h"
#include "data/gc.h"

class Decl;
class Env;
class EnvGen;
class TypedExpr;

/// Reference to typeclass, noting containing environment
class ClassRef {
	friend Env;
	friend EnvGen;

	const EnvGen* env;  ///< Containing environment generation
	std::size_t ind;    ///< Index in that environment's class storage
public:
	ClassRef() : env(nullptr), ind(0) {}
	ClassRef( const EnvGen* env, std::size_t ind ) : env(env), ind(ind) {}

	/// Gets the representative element of the referenced type class
	inline const PolyType* get_root() const;

	/// Ensures root is still representative element of this typeclass.
	/// Not implemented for this variant
	void update_root() {}

	/// Gets the bound type of the referenced type class, nullptr for none such
	inline const Type* get_bound() const;

	/// Get referenced typeclass
	inline const TypeClass& operator* () const;
	const TypeClass* operator-> () const { return &(this->operator*()); }

	/// Check that there is a referenced typeclass
	explicit operator bool() const { return env != nullptr; }

	bool operator== (const ClassRef& o) const { return env == o.env && ind == o.ind; }
	bool operator!= (const ClassRef& o) const { return !(*this == o); }
};

/// Actual storage for type-variable and assertion bindings.
/// Stores a single generation of iterated inheritance
class EnvGen final : public GC_Object {
	friend ClassRef;
	friend Env;

	/// Underlying map type
	using Map = std::unordered_map<const PolyType*, ClassRef>;
public:
	/// Backing storage for typeclasses
	using Storage = std::vector<TypeClass>;
	/// Assertions known by this environment
	using AssertionMap = std::unordered_map<const Decl*, const TypedExpr*>;
	/// Set of seen types for recursion on classes
	using SeenTypes = std::unordered_set<const PolyType*>;

private:
	Storage classes;         ///< Backing storage for typeclasses
	Map bindings;            ///< Bindings from a named type variable to another type
	AssertionMap assns;      ///< Backing storage for assertions
	std::size_t localAssns;  ///< Count of local assertions

public:
	const EnvGen* parent;    ///< Environment this inherits bindings from

	const Storage& getClasses() const { return classes; }

	const AssertionMap& getAssertions() const { return assns; }

private:
#if defined RP_DEBUG && RP_DEBUG >= 1
	void verify() const;
#endif

	/// True if this environment does not store anything unique locally
	bool empty() const { return localAssns == 0 && classes.empty(); }

	/// Returns this or the first non-empty parent environment (null for empty)
	const EnvGen* getNonempty() const {
		// rely on the invariant that if this is empty, its parent will be either null or non-empty
		return empty() ? parent : this;
	}

	EnvGen() : classes(), bindings(), assns(), localAssns(0), parent(nullptr) {}

	/// Shallow copy, just sets parent to non-empty parent
	EnvGen(const EnvGen& o) 
		: classes(), bindings(), assns(), localAssns(0), parent(o.getNonempty()) {}
	
	/// Inserts a new typeclass, consisting of a single var (should not be present)
	void insert( const PolyType* v ) {
		bindings[ v ] = { this, classes.size() };
		classes.emplace_back( List<PolyType>{ v } );
	}

	/// Copies a typeclass from a parent; r.env should not be this or null, and none of the members 
	/// of the class should be bound directly in this. Updates r with the new location
	void copyClass( ClassRef& r ) {
		dbg_verify();
		std::size_t i = classes.size();
		classes.push_back( *r );
		r = ClassRef{ this, i };
		for ( const PolyType* v : classes[i].vars ) { bindings[v] = r; }
		dbg_verify();
	}

	void copyClass( ClassRef&& r ) { copyClass(r); }

	/// Adds v to local class with id rid; v should not be bound in this environment
	void addToClass( std::size_t rid, const PolyType* v ) {
		dbg_verify();
		classes[ rid ].vars.push_back( v );
		bindings[ v ] = { this, rid };
		dbg_verify();
	}

	/// Is this environment generation descended from the other?
	bool inheritsFrom( const EnvGen* o ) const {
		const EnvGen* crnt = this;
		do {
			if ( crnt == o ) return true;
			crnt = crnt->parent;
		} while( crnt );
		return false;
	}

	/// Makes cbound the bound of r in this environment, returning false if incompatible.
	/// May mutate r to copy into local scope.
	bool mergeBound( Env& env, ClassRef& r, const Type* cbound );

	/// Merges s into r; returns false if fails due to contradictory bindings.
	/// r should be local, but may be moved by the merging process; s should be non-null
	bool mergeClasses( Env& env, ClassRef& r, ClassRef s );

	/// Merges the target environment wiht this one. 
	/// Returns false if fails, but does NOT roll back partial changes.
	/// `seen` ensures work isn't doubled for inherited bindings
	bool merge( Env& env, const EnvGen& o, SeenTypes& seen );

	/// Recursively places references to unbound typeclasses in the output list
	void getUnbound( SeenTypes& seen, std::vector<TypeClass>& out ) const;

protected:
	void trace(const GC& gc) const override;
};

/// Wrapper to perform storage management for environment
class Env final {
	friend EnvGen;
	friend const GC& operator<< (const GC&, const Env&);
	friend std::ostream& operator<< (std::ostream&, const Env&);

	EnvGen* self;  ///< Base generation of this environment

	Env( EnvGen* self ) : self(self) {}

public:
	/// true iff t occurs in vars, recursively expanded according to this environment
	bool occursIn( const List<PolyType>& vars, const Type* t ) const;

	/// true iff t occurs in var, recursively expanded according to this environment
	bool occursIn( const PolyType* var, const Type* t ) const {
		return occursIn( List<PolyType>{ var }, t );
	}

	/// Finds the binding reference for this polytype, { nullptr, _ } for none such.
	/// Updates the local index with the binding, if found
	ClassRef findRef( const PolyType* var ) const {
		for ( const EnvGen* crnt = self; crnt; crnt = crnt->parent ) {
			auto it = crnt->bindings.find( var );
			if ( it != crnt->bindings.end() ) {
				if ( crnt != self ) {
					as_non_const(self)->bindings[ var ] = it->second;
				}
				return it->second;
			}
		}
		return {};
	}

	Env() : self(new EnvGen) {}

	/// Shallow copy
	Env( const Env& o ) : self(o.self ? new EnvGen{*o.self} : new EnvGen) {}
	Env& operator= ( const Env& o ) {
		if ( self != o.self ) {
			self = o.self ? new EnvGen{*o.self} : new EnvGen;
		}
		return *this;
	}

	/// Rely on GC to clean up un-used EnvGen
	~Env() = default;

	/// Gets an invalid (always empty) environment
	static Env invalid() { return Env{nullptr}; }

	/// Swap with other environment
	void swap( Env& o ) { std::swap( self, o.self ); }

	/// true if this environment is valid
	explicit operator bool() const { return self != nullptr; }

	/// true if this environment is valid
	bool valid() const { return (bool)*this; }

	/// Check if this environment is empty
	bool empty() const { return self->getNonempty() == nullptr; }

	/// Inserts a type variable if it doesn't already exist in the environment
	/// Returns false if already present
	bool insertVar( const PolyType* var ) {
		if ( findRef( var ) ) return false;
		self->insert( var );
		return true;
	}

	/// Gets the type-class for a type variable, creating it if needed
	ClassRef getClass( const PolyType* var ) {
		ClassRef r = findRef( var );
		if ( ! r ) {
			r = { self, self->classes.size() };
			self->insert( var );
		}
		return r;
	}

	/// Finds an assertion in this environment, returns null if none
	const TypedExpr* findAssertion( const Decl* f ) const {
		for ( const EnvGen* crnt = self; crnt; crnt = crnt->parent ) {
			auto it = crnt->assns.find( f );
			if ( it != crnt->assns.end() ) {
				// add to local binding if not present
				if ( crnt != self ) {
					as_non_const(self)->assns[ f ] = it->second;
				}
				return it->second;
			}
		}
		return nullptr;
	}

	/// Binds this class to the given type; class should be currently unbound.
	/// Class should belong to this environment. Returns false if would create recursive loop.
	bool bindType( ClassRef& r, const Type* sub ) {
		if ( occursIn( r->vars, sub ) ) return false;
		if ( r.env != self ) { self->copyClass( r ); }
		self->classes[ r.ind ].bound = sub;
		return true;
	}

	/// Binds the type variable into the given class; returns false if the type variable is 
	/// incompatibly bound.
	bool bindVar( ClassRef& r, const PolyType* var ) {
		ClassRef vr = findRef( var );
		if ( vr == r ) return true; // exit early if already bound
		if ( r.env != self ) { self->copyClass(r); }  // ensure r is local
		if ( vr ) {
			// merge variable's existing class into this one
			self->mergeClasses( *this, r, vr );
		} else {
			// add unbound variable to class if no cycle
			if ( occursIn( var, self->classes[r.ind].bound ) ) return false;
			self->addToClass( r.ind, var );
		}
		return true;
	}

	/// Binds an assertion in this environment. `f` should be currently unbound
	void bindAssertion( const Decl* f, const TypedExpr* assn ) {
		self->assns.emplace( f, assn );
		++self->localAssns;
	}

	/// Merges the target environment with this one. 
	/// Returns false if fails, but does NOT roll back partial changes.
	bool merge( const Env& o ) {
		// short-circuits for empty cases
		if ( o.empty() ) return true;
		if ( empty() ) {
			self = new EnvGen{ *o.self };
			return true;
		}
		// already compatible with `o`, by definition
		if ( self->inheritsFrom( o.self ) ) return true;
		if ( o.self->inheritsFrom( self ) ) {
			self = new EnvGen{ *o.self };
			return true;
		}

		EnvGen::SeenTypes seen;
		return self->merge( *this, *o.self, seen );
	}

	/// Gets the list of unbound type classes in this environment
	std::vector<TypeClass> getUnbound() const {
		EnvGen::SeenTypes seen;
		std::vector<TypeClass> out;
		self->getUnbound( seen, out );
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

const PolyType* ClassRef::get_root() const {
	return env->classes[ind].vars.front();
}

const Type* ClassRef::get_bound() const {
	return env->classes[ind].bound;
}

const TypeClass& ClassRef::operator* () const {
	return env->classes[ind];
}

inline std::ostream& operator<< (std::ostream& out, const Env& env) {
	return env.write( out );
}

inline const GC& operator<< (const GC& gc, const Env& env) {
	return gc << env.self;
}

inline void swap( Env& a, Env& b ) { a.swap( b ); }

