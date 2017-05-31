#pragma once

#include <ostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "data/cast.h"
#include "data/cow.h"
#include "data/gc.h"
#include "data/list.h"
#include "data/mem.h"
#include "data/set.h"

class Type;
class PolyType;
class Interpretation;
class FuncDecl;

/// Stores type-variable and assertion bindings
class Env final : public GC_Traceable {
	friend std::ostream& operator<< (std::ostream&, const Env&);
	
	/// Class of equivalent type variables, along with optional concrete bound type
    struct TypeClass {
		List<PolyType> vars;  ///< Equivalent polymorphic types
        const Type* bound;    ///< Type which the class binds to

        TypeClass( List<PolyType>&& vars = List<PolyType>{}, const Type* bound = nullptr )
            : vars( move(vars) ), bound(bound) {}
        
        TypeClass( const PolyType* orig, const Type* bound = nullptr )
            : vars{ orig }, bound(bound) {}
        
        TypeClass( const PolyType* orig, const PolyType* added )
            : vars{ orig, added }, bound(nullptr) {}
        
        TypeClass( const PolyType* orig, std::nullptr_t )
            : vars{ orig }, bound(nullptr) {}
    };

	/// Backing storage for typeclasses
	using Storage = std::vector<TypeClass>;
	/// Reference to typeclass storage
	using StorageRef = std::pair<const Env*, Storage::size_type>;
	/// Underlying map type
	using Map = std::unordered_map<const PolyType*, StorageRef>;
	/// Assertions known by this environment
	using AssertionMap = std::unordered_map<const FuncDecl*, const Interpretation*>;

	Storage classes;         ///< Backing storage for typeclasses
	Map bindings;            ///< Bindings from a named type variable to another type
	AssertionMap assns;      ///< Backing storage for assertions
	const Env* parent;       ///< Environment this inherits bindings from

	static inline const TypeClass& classOf( StorageRef r ) { return r.first->classes[ r.second ]; }

	/// Inserts a new typeclass, mapping `orig` to `sub`; 
    /// environment should be initialized and `orig` should not be present.
    void insert( const PolyType* orig, const Type* sub = nullptr ) {
        bindings[ orig ] = { this, classes.size() };
        classes.emplace_back( orig, sub );
    }

	/// Inserts a new typeclass, mapping `orig` and `added` to null.
    /// Environment should be initialized and neither `orig` nor `added` should be present.
    void insert( const PolyType* orig, const PolyType* added ) {
        bindings[ orig ] = bindings[ added ] = { this, classes.size() };
        classes.emplace_back( orig, added );
    }

	/// Copies a typeclass from a parent; r.first should not be this or null, and none 
	/// of the members of the class should be bound directly in this.
	Storage::size_type copyClass( StorageRef r ) {
		Storage::size_type i = classes.size();
		classes.push_back( classOf(r) );
		for ( const PolyType* v : classes[i].vars ) { bindings[v] = { this, i }; }	
		return i;
	}

	/// Finds the binding reference for this polytype, { nullptr, _ } for none such.
	/// Updates the local map with the binding, if found.
	StorageRef findRef( const PolyType* orig ) const {
		// search self
		auto it = bindings.find( orig );
		if ( it != bindings.end() ) return it->second;
		// lookup in parent, adding to local binding if found
		for ( const Env* crnt = parent; crnt; crnt = crnt->parent ) {
			it = crnt->bindings.find( orig );
			if ( it != crnt->bindings.end() ) {
				as_non_const(this)->bindings[ orig ] = it->second;
				return it->second;
			}
		}
		return { nullptr, 0 };
	}

	/// Makes cbound the bound of r in this environment, returning false if incompatible.
	/// May mutate r to copy into local scope.
	bool mergeBound( StorageRef& r, const Type* cbound );

	/// Adds v to local class with id rid; v should not be bound in this environment.
	void addToClass( Storage::size_type rid, const PolyType* v ) {
		classes[ rid ].vars.push_back( v );
		bindings[ v ] = StorageRef{ this, rid };
	}

	/// Merges s into r; returns false if fails due to contradictory bindings.
	/// r should be local, but may be moved by the merging process; s should be non-null
	bool mergeClasses( StorageRef& r, StorageRef s ) {
		TypeClass& rc = classes[ r.second ];
		const TypeClass& sc = classOf( s );

		// ensure bounds match
		if ( ! mergeBound( r, sc.bound ) ) return false;

		if ( s.first == this ) {
			// need to merge two existing local classes -- guaranteed that vars don't overlap
			rc.vars.insert( rc.vars.end(), sc.vars.begin(), sc.vars.end() );
			for ( const PolyType* v : sc.vars ) { bindings[v] = r; }
			// remove s from local class-list
			Storage::size_type victim = classes.size() - 1;
			if ( s.second != victim ) {
				classes[ s.second ] = move(classes[victim]);
				for ( const PolyType* v : classes[ s.second ].vars ) { bindings[v] = s; }
			}
			classes.pop_back();
			if ( r.second == victim ) { r.second = s.second; }
			return true;
		}
		
		// merge in variables
		rc.vars.reserve( rc.vars.size() + sc.vars.size() );
		for ( const PolyType* v : sc.vars ) {
			StorageRef rr = findRef( v );
			if ( rr.first == nullptr ) {
				// not bound
				addToClass( r.second, v );
				continue;
			} else if ( rr == r ) {
				// already in class
				continue;
			} else {
				if ( ! mergeClasses( r, rr ) ) return false;
			}
		}
		return true;
	}

	/// Is this environment descended from the other?
	bool inherits_from( const Env& o ) const {
		const Env* crnt = this;
		do {
			if ( crnt == &o ) return true;
			crnt = crnt->parent;
		} while ( crnt );
		return false;
	}

public:
	Env() = default;

    /// Constructs a brand new environment with a single binding
    Env( const PolyType* orig, const Type* sub = nullptr ) 
		: classes(), bindings(), assns(), parent(nullptr) { insert( orig, sub ); }

    /// Constructs a brand new environment with two poly types bound together
    Env( const PolyType* orig, const PolyType* added ) 
		: classes(), bindings(), assns(), parent(nullptr) { insert( orig, added ); }

    Env( const PolyType* orig, std::nullptr_t ) 
		: classes(), bindings(), assns(), parent(nullptr) { insert( orig ); }

	/// Shallow copy, just sets parent of new environment
	Env( const Env& o ) : classes(), bindings(), assns(), parent(&o) {}

	/// Deleted to avoid the possibility of environment cycles.
	Env& operator= ( const Env& o ) = delete;

	/// Query for assertion map
	const AssertionMap& assertions() const { return assns; }

	/// Checks if this environment contains a typeclass (possibly unbound) for a polytype
	bool contains( const PolyType* orig ) const {
		return findRef( orig ).first != nullptr;
	}

	/// Finds a mapping in this environment, returns null if none
    const Type* find( const PolyType* orig ) const {
		StorageRef r = findRef( orig );
		return r.first == nullptr ? nullptr : classOf( r ).bound;
    }

	/// Finds an assertion in this environment, returns null if none
	const Interpretation* findAssertion( const FuncDecl* f ) const {
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

    /// Adds the given substitution to this binding. `orig` should be currently unbound.
    void bind( const PolyType* orig, const Type* sub ) {
		StorageRef r = findRef( orig );
		if ( r.first == nullptr ) {
			insert( orig, sub );
		} else {
			if ( r.first != this ) { r.second = copyClass( r ); }
			classes[ r.second ].bound = sub;
		}
    }

    /// Adds the second PolyType to the class of the first; the second PolyType should not 
    /// currently exist in the binding table.
    void bindClass( const PolyType* orig, const PolyType* added ) {
		StorageRef r = findRef( orig );
		if ( r.first == nullptr ) {
			insert( orig, added );
		} else {
			if ( r.first != this ) { r.second = copyClass( r ); }
			addToClass( r.second, added );
		}
    }

	/// Binds an assertion in this environment. `f` should be currently unbound.
	void bindAssertion( const FuncDecl* f, const Interpretation* assn ) {
		assns.emplace( f, assn );
	}

	/// Merges the target environment with this one.
	/// Returns false if fails, but does NOT roll back partial changes.
	/// seen makes sure work isn't doubled for inherited bindings
	bool merge( const Env& o, 
			std::unordered_set<const PolyType*>&& seen = std::unordered_set<const PolyType*>{} ) {
		// Already compatible with o, by definition.
		// The "o inherits from this" case shouldn't happen, but should be properly handled by below.
		if ( inherits_from( o ) ) return true;
		
		// merge classes
		for ( Storage::size_type ci = 0; ci < o.classes.size(); ++ci ) {
			const TypeClass& c = o.classes[ ci ];
			unsigned n = c.vars.size();
			unsigned i;
			StorageRef r{ nullptr, 0 };              // typeclass in this environment

			for ( i = 0; i < n; ++i ) {
				// mark variable as seen
				if ( seen.count( c.vars[i] ) ) continue;
				else seen.insert( c.vars[i] );
				// look for first existing bound variable
				r = findRef( c.vars[i] );
				if ( r.first != nullptr ) break;
			}

			if ( i < n ) {
				// attempt to merge bound into r
				if ( ! mergeBound( r, c.bound ) ) return false;
				// merge previous variables into this class
				if ( r.first != this ) { r = { this, copyClass( r ) }; }
				for ( unsigned j = 0; j <= i; ++j ) addToClass( r.second, c.vars[j] );
				// merge subsequent variables into this class
				while ( ++i < n ) {
					// mark variable as seen
					if ( seen.count( c.vars[i] ) ) continue;
					else seen.insert( c.vars[i] );
					// copy unbound variables into this class, skipping ones already bound to it
					StorageRef rr = findRef( c.vars[i] );
					if ( rr.first == nullptr ) {
						addToClass( r.second, c.vars[i] );
						continue;
					} else if ( rr == r ) continue;

					// merge new class into existing
					if ( ! mergeClasses( r, rr ) ) return false;
				}
			} else {
				// no variables in this typeclass bound; just copy up
				copyClass( StorageRef{ &o, ci } );
				continue;
			}
		}

		// merge assertions
		for ( const auto& a : o.assns ) {
			auto it = assns.find( a.first );
			if ( it == assns.end() ) {
				assns.insert( a );
			} else {
				if ( it->second != a.second ) return false;
			}
		}

		// merge parents
		return o.parent ? merge( *o.parent, move(seen) ) : true;
	}

protected:
	void trace(const GC& gc) const override;

private:
	/// Writes out this environment
    std::ostream& write(std::ostream& out) const;
};

/// Returns true iff the type is contained.
/// Handles null environment correctly.
inline const bool contains( const cow_ptr<Env>& env, const PolyType* orig ) {
	return env ? env->contains( orig ) : false;
}

/// Returns nullptr for unbound type, contained type otherwise.
/// Handles null environment correctly.
inline const Type* find( const cow_ptr<Env>& env, const PolyType* orig ) {
    return env ? env->find( orig ) : nullptr;
}

/// Replaces type with bound value in the environment, if type is a PolyType with a 
/// substitution in the environment. Handles null environment correctly.
inline const Type* replace( const cow_ptr<Env>& env, const Type* ty ) {
    if ( ! env ) return ty;
    if ( const PolyType* pty = as_safe<PolyType>(ty) ) {
        const Type* sub = env->find( pty );
        if ( sub ) return sub;
    }
    return ty;
}

/// Replaces type with bound value in environment, if substitution exists.
/// Handles null environment correctly.
inline const Type* replace( const cow_ptr<Env>& env, const PolyType* pty ) {
    if ( ! env ) return as<Type>(pty);
    const Type* sub = env->find( pty );
    if ( sub ) return sub;
    return as<Type>(pty);
}

/// Adds the given substitution to this binding. `orig` should be currently unbound.
/// Creates new environment if `env == null` 
inline void bind( cow_ptr<Env>& env, const PolyType* orig, const Type* sub ) {
    if ( ! env ) {
        env.reset( new Env(orig, sub) );
    } else {
        env.mut().bind( orig, sub );
    }
}

/// Adds the second PolyType to the class of the first; the second PolyType should not 
/// currently exist in the binding table. Creates new environment if null
inline void bindClass( cow_ptr<Env>& env, const PolyType* orig, const PolyType* added ) {
    if ( ! env ) {
        env.reset( new Env( orig, added ) );
    } else {
        env.mut().bindClass( orig, added );
    }
}

/// Merges b into a, returning false and nulling a on failure
inline bool merge( cow_ptr<Env>& a, const cow_ptr<Env>& b ) {
    if ( ! b ) return true;
    if ( ! a ) {
        a = b;
        return true;
    }
    if ( ! a.mut().merge( *b ) ) {
        a.reset();
        return false;
    }
    return true;
}

inline std::ostream& operator<< (std::ostream& out, const Env& env) {
    return env.write( out );
}
