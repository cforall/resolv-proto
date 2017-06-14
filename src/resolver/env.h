#pragma once

#include <cstddef>
#include <ostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "data/cast.h"
#include "data/gc.h"
#include "data/list.h"
#include "data/mem.h"

class Env;
class FuncDecl;
class PolyType;
class Type;
class TypedExpr;

/// Class of equivalent type variables, along with optional concrete bound type
struct TypeClass {
	List<PolyType> vars;  ///< Equivalent polymorphic types
	const Type* bound;    ///< Type which the class binds to

	TypeClass( List<PolyType>&& vars = List<PolyType>{}, const Type* bound = nullptr )
		: vars( move(vars) ), bound(bound) {}
};

std::ostream& operator<< (std::ostream&, const TypeClass&);

/// Reference to typeclass, noting containing environment
class ClassRef {
	friend Env;

	const Env* env;   ///< Containing environment
	std::size_t ind;  ///< Index in that environment's class storage
public:
	ClassRef() : env(nullptr), ind(0) {}
	ClassRef( const Env* env, std::size_t ind ) : env(env), ind(ind) {}

	// Get referenced typeclass
	const TypeClass& operator* () const;
	const TypeClass* operator-> () const { return &(this->operator*()); }

	// Check that there is a referenced typeclass
	explicit operator bool() const { return env != nullptr; }

	bool operator== (const ClassRef& o) const { return env == o.env && ind == o.ind; }
	bool operator!= (const ClassRef& o) const { return !(*this == o); }
};

/// Stores type-variable and assertion bindings
class Env final : public GC_Traceable {
	friend ClassRef;
	friend std::ostream& operator<< (std::ostream&, const Env&);

	/// Backing storage for typeclasses
	using Storage = std::vector<TypeClass>;
	/// Underlying map type
	using Map = std::unordered_map<const PolyType*, ClassRef>;
	/// Assertions known by this environment
	using AssertionMap = std::unordered_map<const FuncDecl*, const TypedExpr*>;

	Storage classes;         ///< Backing storage for typeclasses
	Map bindings;            ///< Bindings from a named type variable to another type
	AssertionMap assns;      ///< Backing storage for assertions
	std::size_t localAssns;  ///< Count of local assertions
	const Env* parent;       ///< Environment this inherits bindings from

public:
	/// Finds the binding reference for this polytype, { nullptr, _ } for none such.
	/// Updates the local map with the binding, if found.
	ClassRef findRef( const PolyType* var ) const {
		// search self
		auto it = bindings.find( var );
		if ( it != bindings.end() ) return it->second;
		// lookup in parent, adding to local binding if found
		for ( const Env* crnt = parent; crnt; crnt = crnt->parent ) {
			it = crnt->bindings.find( var );
			if ( it != crnt->bindings.end() ) {
				as_non_const(this)->bindings[ var ] = it->second;
				return it->second;
			}
		}
		return {};
	}

private:
	/// Inserts a new typeclass, consisting of a single var (should not be present).
	void insert( const PolyType* var ) {
		bindings[ var ] = { this, classes.size() };
		classes.emplace_back( List<PolyType>{ var } );
	}

	/// Copies a typeclass from a parent; r.first should not be this or null, and none 
	/// of the members of the class should be bound directly in this.
	std::size_t copyClass( ClassRef r ) {
		std::size_t i = classes.size();
		classes.push_back( *r );
		for ( const PolyType* v : classes[i].vars ) { bindings[v] = { this, i }; }	
		return i;
	}

	/// Adds v to local class with id rid; v should not be bound in this environment.
	void addToClass( std::size_t rid, const PolyType* v ) {
		classes[ rid ].vars.push_back( v );
		bindings[ v ] = { this, rid };
	}

	/// Makes cbound the bound of r in this environment, returning false if incompatible.
	/// May mutate r to copy into local scope.
	bool mergeBound( ClassRef& r, const Type* cbound );

	/// Merges s into r; returns false if fails due to contradictory bindings.
	/// r should be local, but may be moved by the merging process; s should be non-null
	bool mergeClasses( ClassRef& r, ClassRef s ) {
		TypeClass& rc = classes[ r.ind ];
		const TypeClass& sc = *s;

		// ensure bounds match
		if ( ! mergeBound( r, sc.bound ) ) return false;

		if ( s.env == this ) {
			// need to merge two existing local classes -- guaranteed that vars don't 
			// overlap
			rc.vars.insert( rc.vars.end(), sc.vars.begin(), sc.vars.end() );
			for ( const PolyType* v : sc.vars ) { bindings[v] = r; }
			// remove s from local class-list
			std::size_t victim = classes.size() - 1;
			if ( s.ind != victim ) {
				classes[ s.ind ] = move(classes[victim]);
				for ( const PolyType* v : classes[ s.ind ].vars ) { bindings[v] = s; }
			}
			classes.pop_back();
			if ( r.ind == victim ) {
				// note that r has been moved to the old position of s
				r.ind = s.ind;
			}
			return true;
		}
		
		// merge in variables
		rc.vars.reserve( rc.vars.size() + sc.vars.size() );
		for ( const PolyType* v : sc.vars ) {
			ClassRef rr = findRef( v );
			if ( ! rr || rr == s ) {
				// not bound or bound in target class in parent; can safely add to r
				addToClass( r.ind, v );
			} else if ( rr == r ) {
				// already in class; do nothing
			} else {
				// new victim class; needs to merge successfully as well
				if ( ! mergeClasses( r, rr ) ) return false;
			}
		}
		return true;
	}

	/// Recursively places references to unbound typeclasses in the output list
	void getUnbound( std::unordered_set<const PolyType*>& seen, 
	                 List<TypeClass>& out ) const {
		for ( const TypeClass& c : classes ) {
			for ( const PolyType* v : c.vars ) {
				// skip classes containing vars we've seen before
				if ( ! seen.insert( v ).second ) goto nextClass;
			}

			// report unbound classes
			if ( ! c.bound ) { out.push_back( &c ); }
		nextClass:; }

		// recurse to parent
		if ( parent ) { parent->getUnbound( seen, out ); }
	}

	/// Is this environment descended from the other?
	bool inheritsFrom( const Env& o ) const {
		const Env* crnt = this;
		do {
			if ( crnt == &o ) return true;
			crnt = crnt->parent;
		} while ( crnt );
		return false;
	}

	/// Returns this or its first non-empty parent environment (null for empty env).
	const Env* getNonempty() const {
		// rely on the invariant that if this is empty, its parent will be 
		// either null or non-empty
		return ( localAssns > 0 || ! classes.empty() ) ? this : parent;
	}

public:
	Env() = default;

	/// Constructs a brand new environment with a single class containing only var
	Env( const PolyType* var ) 
		: classes(), bindings(), assns(), localAssns(0), parent(nullptr)
		{ insert( var ); }

	/// Constructs a brand new environment with a single bound class
	Env( ClassRef r, const Type* sub = nullptr )
		: classes(), bindings(), assns(), localAssns(0), parent(nullptr)
		{ copyClass(r); classes.front().bound = sub; }
	
	/// Constructs a brand new environment with a single class with an added type variable
	Env( ClassRef r, const PolyType* var )
		: classes(), bindings(), assns(), localAssns(0), parent(nullptr) {
		copyClass(r);
		if ( ! bindings.count( var ) ) { addToClass(0, var); }
	}
	
	/// Constructs a brand new environment with a single class
	Env( ClassRef r, std::nullptr_t )
		: classes(), bindings(), assns(), localAssns(0), parent(nullptr) { copyClass(r); }

	/// Shallow copy, just sets parent of new environment
	Env( const Env& o )
		: classes(), bindings(), assns(), localAssns(0), parent(o.getNonempty()) {}

	/// Deleted to avoid the possibility of environment cycles.
	Env& operator= ( const Env& o ) = delete;

	/// Makes a new environment with the given environment as parent.
	/// If the given environment is null, so will be the new one.
	static unique_ptr<Env> from( const Env* env ) {
		return unique_ptr<Env>{ env ? new Env{ *env } : nullptr };
	}

	/// Inserts a type variable if it doesn't already exist in the environment.
	/// Returns false if already present.
	bool insertVar( const PolyType* orig ) {
		if ( findRef(orig) ) return false;
		insert( orig );
		return true;
	}

	/// Gets the type-class for a type variable, creating it if needed
	ClassRef getClass( const PolyType* orig ) {
		ClassRef r = findRef( orig );
		if ( ! r ) {
			r = { this, classes.size() };
			insert( orig );
		}
		return r;
	}

	/// Finds an assertion in this environment, returns null if none
	const TypedExpr* findAssertion( const FuncDecl* f ) const {
		// search self
		auto it = assns.find( f );
		if ( it != assns.end() ) return it->second;
		// lookup in parent, adding to local binding if found
		for ( const Env* crnt = parent; crnt; crnt = crnt->parent ) {
			it = crnt->assns.find( f );
			if ( it != crnt->assns.end() ) {
				as_non_const(this)->assns[ f ] = it->second;
				return it->second;
			}
		}
		return nullptr;
	}

	/// Binds this class to the given type; class should be currently unbound.
	/// May copy class into local scope; class should belong to this or a parent.
	void bindType( ClassRef r, const Type* sub ) {
		if ( r.env != this ) { r.ind = copyClass( r ); }
		classes[ r.ind ].bound = sub;
	}

	/// Binds the type variable into to the given class; returns false if the 
	/// type variable is incompatibly bound.
	bool bindVar( ClassRef r, const PolyType* var ) {
		ClassRef vr = findRef( var );
		if ( vr == r ) return true;  // exit early if already bound
		if ( r.env != this ) { r = { this, copyClass(r) }; }  // ensure r is local
		if ( vr ) {
			// merge variable's existing class into this one
			return mergeClasses( r, vr );
		} else {
			// add unbound variable to class
			addToClass( r.ind, var );
			return true;
		}
	}

	/// Binds an assertion in this environment. `f` should be currently unbound.
	void bindAssertion( const FuncDecl* f, const TypedExpr* assn ) {
		assns.emplace( f, assn );
		++localAssns;
	}

	/// Merges the target environment with this one.
	/// Returns false if fails, but does NOT roll back partial changes.
	/// seen makes sure work isn't doubled for inherited bindings
	bool merge( const Env& o, 
			std::unordered_set<const PolyType*>&& seen 
				= std::unordered_set<const PolyType*>{} ) {
		// Already compatible with o, by definition.
		if ( inheritsFrom( o ) ) return true;
		
		// merge classes
		for ( std::size_t ci = 0; ci < o.classes.size(); ++ci ) {
			const TypeClass& c = o.classes[ ci ];
			std::size_t n = c.vars.size();
			std::size_t i;
			ClassRef r{};  // typeclass in this environment

			for ( i = 0; i < n; ++i ) {
				// mark variable as seen; skip if already seen
				if ( seen.insert( c.vars[i] ).second == false ) continue;
				// look for first existing bound variable
				r = findRef( c.vars[i] );
				if ( r ) break;
			}

			if ( i < n ) {
				// attempt to merge bound into r
				if ( ! mergeBound( r, c.bound ) ) return false;
				// merge previous variables into this class
				if ( r.env != this ) { r = { this, copyClass( r ) }; }
				for ( std::size_t j = 0; j < i; ++j ) addToClass( r.ind, c.vars[j] );
				// merge subsequent variables into this class
				while ( ++i < n ) {
					// mark variable as seen; skip if already seen
					if ( seen.insert( c.vars[i] ).second == false ) continue;
					// copy unbound variables into this class, skipping ones already 
					// bound to it
					ClassRef rr = findRef( c.vars[i] );
					if ( ! rr ) {
						// unbound; safe to add
						addToClass( r.ind, c.vars[i] );
					} else if ( rr == r ) {
						// bound to target class; already added, do nothing
					} else {
						// merge new class into existing
						if ( ! mergeClasses( r, rr ) ) return false;
					}
				}
			} else {
				// no variables in this typeclass bound; just copy up
				copyClass( ClassRef{ &o, ci } );
				continue;
			}
		}

		// merge assertions
		for ( const auto& a : o.assns ) {
			auto it = assns.find( a.first );
			if ( it == assns.end() ) {
				assns.insert( a );
				++localAssns;
			} else {
				if ( it->second != a.second ) return false;
			}
		}

		// merge parents
		return o.parent ? merge( *o.parent, move(seen) ) : true;
	}

	/// Gets the list of unbound typeclasses in this environment
	List<TypeClass> getUnbound() const {
		std::unordered_set<const PolyType*> seen;
		List<TypeClass> out;
		getUnbound( seen, out );
		return out;
	}

protected:
	void trace(const GC& gc) const override;

private:
	/// Writes out this environment
    std::ostream& write(std::ostream& out) const;
};

inline const TypeClass& ClassRef::operator* () const {
	return env->classes[ ind ];
}

/// Gets the typeclass for a given type variable, inserting it if necessary.
inline ClassRef getClass( unique_ptr<Env>& env, const PolyType* orig ) {
	if ( ! env ) { env.reset( new Env{} ); }
	return env->getClass( orig );
}

// TODO consider having replace return the first PolyType in a class as a unique representative

/// Replaces type with bound value in environment, if substitution exists.
/// Treats null environment as empty.
inline const Type* replace( const Env* env, const PolyType* pty ) {
    if ( ! env ) return as<Type>(pty);
    ClassRef r = env->findRef( pty );
	if ( r && r->bound ) return r->bound;
    return as<Type>(pty);
}

/// Replaces type with bound value in the environment, if type is a PolyType with a 
/// substitution in the environment. Treats null environment as empty.
inline const Type* replace( const Env* env, const Type* ty ) {
	if ( ! env ) return ty;
	if ( is<PolyType>(ty) ) {
		ClassRef r = env->findRef( as<PolyType>(ty) );
		if ( r && r->bound ) return r->bound;
	}
	return ty;
}

/// Inserts the given type variable into this environment if it is not currently present.
/// Returns false if the variable was already there.
inline bool insertVar( unique_ptr<Env>& env, const PolyType* orig ) {
	if ( ! env ) {
		env.reset( new Env( orig ) );
		return true;
	} else {
		return env->insertVar( orig );
	}
}

/// Adds the given substitution to this environment. 
/// `r` should be currently unbound and belong to `env` or a parent.
/// Creates a new environment if `env == null`
inline void bindType( unique_ptr<Env>& env, ClassRef r, const Type* sub ) {
	if ( ! env ) {
		env.reset( new Env(r, sub) );
	} else {
		env->bindType( r, sub );
	}
}

/// Adds the type variable to the given class. Creates new environment if null, 
/// returns false if incompatible binding.
inline bool bindVar( unique_ptr<Env>& env, ClassRef r, const PolyType* var ) {
	if ( ! env ) {
		env.reset( new Env(r, var) );
		return true;
	} else {
		return env->bindVar( r, var );
	}
}

/// Finds the given assertion in the environment; nullptr if unbound. 
/// Null environment treated as empty.
inline const TypedExpr* findAssertion( const Env* env, const FuncDecl* f ) {
	return env ? env->findAssertion( f ) : nullptr;
}

/// Adds the given assertion to the envrionment; the assertion should not currently exist 
/// in the environment. Creates new envrionment if null.
inline void bindAssertion( unique_ptr<Env>& env, const FuncDecl* f, 
                           const TypedExpr* assn ) {
	if ( ! env ) { env.reset( new Env{} ); }
	env->bindAssertion( f, assn );
}

/// Merges b into a, returning false and nulling a on failure
inline bool merge( unique_ptr<Env>& a, const Env* b ) {
    if ( ! b ) return true;
    if ( ! a ) {
        a.reset( new Env{ *b } );
        return true;
    }
    if ( ! a->merge( *b ) ) {
        a.reset();
        return false;
    }
    return true;
}

/// Gets the list of unbound typeclasses in an environment
inline List<TypeClass> getUnbound( const Env* env ) {
	return env ? env->getUnbound() : List<TypeClass>{};
}

inline std::ostream& operator<< (std::ostream& out, const Env& env) {
    return env.write( out );
}
