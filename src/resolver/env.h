#pragma once

#include <algorithm>
#include <cstddef>
#include <list>
#include <ostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "cost.h"

#include "data/cast.h"
#include "data/debug.h"
#include "data/gc.h"
#include "data/list.h"
#include "data/mem.h"
#include "data/persistent_disjoint_set.h"
#include "data/persistent_map.h"

#if defined RP_DEBUG && RP_DEBUG >= 1
	#define dbg_verify() verify()
#else
	#define dbg_verify()
#endif

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

	const Env* env;        ///< Containing environment
	const PolyType* root;  ///< Root member of this class
	
public:
	ClassRef() : env(nullptr), root(nullptr) {}
	ClassRef( const Env* env, const PolyType* root ) : env(env), root(root) {}

	/// Gets the root of the referenced type class
	const PolyType* get_root() const { return root; }

	/// Ensures that root is still the representative element of this typeclass;
	/// undefined behaviour if called without referenced typeclass
	inline void update_root();

	/// Gets the type variables of the referenced type class, empty list for none
	inline List<PolyType> get_vars() const;

	/// Gets the bound type of the referenced type class, nullptr for none such
	inline const Type* get_bound() const;

	/// Gets the referenced typeclass
	inline TypeClass get_class() const;

	// Get referenced typeclass
	inline TypeClass operator* () const { return get_class(); };

	// Check that there is a referenced typeclass
	explicit operator bool() const { return env != nullptr; }

	bool operator== (const ClassRef& o) const { return env == o.env && root == o.root; }
	bool operator!= (const ClassRef& o) const { return !(*this == o); }
};

/// Stores type-variable and assertion bindings
class Env final {
	friend ClassRef;
	friend const GC& operator<< (const GC&, const Env&);
	friend std::ostream& operator<< (std::ostream&, const Env&);

	/// Backing storage for typeclasses
	using Classes = persistent_disjoint_set<const PolyType*>;
	/// Type bindings included in this environment (from class root)
	using Bindings = persistent_map<const PolyType*, const Type*>;
	/// Assertions known by this environment
	using Assertions = persistent_map<const FuncDecl*, const TypedExpr*>;

	Classes* classes;        ///< Backing storage for typeclasses
	Bindings* bindings;      ///< Bindings from a named type variable to another type
	Assertions* assns;       ///< Backing storage for assertions

public:
	/// Finds the binding reference for this polytype, { nullptr, _ } for none such.
	/// Updates the local map with the binding, if found.
	ClassRef findRef( const PolyType* var ) const {
		const PolyType* root = classes->find_or_default( var, nullptr );
		if ( root ) return { this, root };
		else return {};
	}

	/// true iff t occurs in vars, recursively expanded according to env
	bool occursIn( const List<PolyType>& vars, const Type* t ) const;

	/// true iff t occurs in var
	bool occursIn( const PolyType* var, const Type* t ) const {
		return occursIn( List<PolyType>{ var }, t );
	}

private:
#if defined RP_DEBUG && RP_DEBUG >= 1
	void verify() const;
#endif

	/// Inserts a new typeclass, consisting of a single var (should not be present).
	void insert( const PolyType* v ) {
		classes = classes->add( v );
		bindings = bindings->set( v, nullptr );
	}

	/// Adds `v` to local class rooted at `root`; `v` should not be bound in this environment.
	void addToClass( const PolyType* root, const PolyType* v ) {
		dbg_verify();
		classes = classes->add( v )->merge( root, v );
		dbg_verify();
	}

	/// Makes cbound or a more specialized type the bound of r in this environment, 
	/// returning false if incompatible. Does not roll back intermediate changes.
	/// Will update r to final root
	bool mergeBound( ClassRef& r, const Type* cbound );

	/// Merges s into r; returns false if fails due to contradictory bindings.
	/// Will update r to final root, s will have no such change.
	bool mergeClasses( ClassRef& r, ClassRef s ) {
		// ensure bounds match
		if ( ! mergeBound( r, s.get_bound() ) ) return false;

		// ensure occurs check not violated
		if ( occursIn( s.get_vars(), r.get_bound() ) ) return false;

		// merge two classes
		s.update_root();  // in case mergeBound invalidated it
		const PolyType* rr = r.get_root();
		const PolyType* sr = s.get_root();
		Classes* new_classes = classes->merge( rr, sr );

		// update bindings
		assume(classes->get_mode() == Classes::REMFROM, "Classes updated to by merge");
		const PolyType* root = classes->get_root();
		if ( root == rr ) {
			// erase binding for sr if no longer root
			bindings = bindings->erase( sr );
		} else if ( root == sr ) {
			// update s binding to match merged r binding and then erase r
			bindings = bindings->set( sr, bindings->get( rr ) )->erase( rr );
			// reroot r
			r.root = sr;
		} else unreachable("root must be one of previous two class roots");

		// finalize classes
		classes = new_classes;
		dbg_verify();
		return true;
	}

public:
	Env() : classes( new Classes{} ), bindings( new Bindings{} ), assns( new Assertions{} ) {}

	/// Shallow copy, just sets copy of underlying maps
	Env( const Env& o ) = default;
	Env& operator= ( const Env& o ) = default;

	/// Rely on GC to clean up un-used version nodes
	~Env() = default;

	/// Swap with other environment
	void swap( Env& o ) {
		using std::swap;

		swap( classes, o.classes );
		swap( bindings, o.bindings );
		swap( assns, o.assns );
	}

	/// Check if this environment has no classes or assertions
	bool empty() const { return classes->empty() && assns->empty(); }

	/// Inserts a type variable if it doesn't already exist in the environment.
	/// Returns false if already present.
	bool insertVar( const PolyType* var ) {
		if ( classes->count( var ) ) return false;
		insert( var );
		return true;
	}

	/// Gets the type-class for a type variable, creating it if needed
	ClassRef getClass( const PolyType* var ) {
		ClassRef r = findRef( var );
		if ( ! r ) {
			insert( var );
			r = { this, var };
		}
		return r;
	}

	/// Finds an assertion in this environment, returns null if none
	const TypedExpr* findAssertion( const FuncDecl* f ) const {
		return assns->get_or_default( f, nullptr );
	}

	/// Binds this class to the given type; class should be currently unbound.
	/// Class should belong to this environment. Returns false if would create recursive loop.
	bool bindType( ClassRef& r, const Type* sub ) {
		assume( r.env == this, "invalid ref for environment" );
		if ( occursIn( r.get_vars(), sub ) ) return false;
		bindings = bindings->set( r.get_root(), sub );
		return true;
	}

	/// Binds the type variable into to the given class; returns false if the 
	/// type variable is incompatibly bound.
	bool bindVar( ClassRef& r, const PolyType* var ) {
		assume( r.env == this, "invalid ref for environment" );
		ClassRef vr = findRef( var );
		if ( vr == r ) return true;  // exit early if already bound
		if ( vr ) {
			// merge variable's existing class into this one
			return mergeClasses( r, vr );
		} else {
			// add unbound variable to class if no cycle
			if ( occursIn( var, r.get_bound() ) ) return false;
			addToClass( r.get_root(), var );
			return true;
		}
	}

	/// Binds an assertion in this environment. `f` should be currently unbound.
	void bindAssertion( const FuncDecl* f, const TypedExpr* assn ) {
		assns = assns->set( f, assn );
	}

private:
	/// Checks if the classes of another environment are compatible with those of this environment.
	/// Returns false if fails, but does NOT roll back partial changes.
	bool mergeAllClasses( const Env& o ) {
		/// Edit item for path state
		struct Edit {
			Classes::Mode mode;    ///< Type of change to a key
			const PolyType* root;  ///< New/Previous root, if addTo/remFrom node

			Edit(Classes::Mode m, const PolyType* r = nullptr) : mode(m), root(r) {}
		};

		// track class changes
		classes->reroot();

		using EditEntry = std::pair<const PolyType*, Edit>;
		using EditList = std::list<EditEntry>;
		using IndexedEdits = std::vector<EditList::iterator>;

		EditList edits;
		std::unordered_map<const PolyType*, IndexedEdits> editIndex;
		
		// trace path from other evironment
		const Classes* oclasses = o.classes;
		Classes::Mode omode = oclasses->get_mode();
		while ( omode != Classes::BASE ) {
			const PolyType* key = oclasses->get_key();
			IndexedEdits& forKey = editIndex[ key ];

			if ( forKey.empty() ) {
				// newly seen key, mark op
				const PolyType* root = ( omode == Classes::ADD || omode == Classes::REM ) ?
					nullptr : oclasses->get_root();
				auto it = edits.emplace( edits.begin(), key, Edit{ omode, root } );
				forKey.push_back( move(it) );
			} else {
				auto next = forKey.back();  // edit next applied to this key
				Classes::Mode nmode = next->second.mode; // next action applied

				switch ( omode ) {
					case Classes::ADD: {
						switch ( nmode ) {
							case Classes::REM: {
								// later removal, no net change to classes
								edits.erase( next );
								forKey.pop_back();
							} break;
							case Classes::ADDTO: {
								// later merge, prefix with this addition
								auto it = edits.emplace( edits.begin(), key, Edit{ omode } );
								forKey.push_back( move(it) );
							} break;
							default: unreachable("inconsistent mode");
						}
					} break;
					case Classes::REM: {
						// later addition, no net change to classes
						assume( nmode == Classes::ADD, "inconsistent mode" );
						edits.erase( next );
						forKey.pop_back();
					} break;
					case Classes::ADDTO: {
						// later unmerged from same class, no net change to classes
						assume( nmode == Classes::REMFROM, "inconsistent mode" );
						assume( oclasses->get_root() == next->second.root, "inconsistent tree" );
						edits.erase( next );
						forKey.pop_back();
					} break;
					case Classes::REMFROM: {
						const PolyType* root = oclasses->get_root();
						switch ( nmode ) {
							case Classes::REM: {
								// later removal, prefix with this unmerge
								auto it = edits.emplace( edits.begin(), key, Edit{ omode, root } );
								forKey.push_back( move(it) );
							} break;
							case Classes::ADDTO: {
								if ( root == next->second.root ) {
									// later merged back into same class, no net change
									edits.erase( next );
									forKey.pop_back();
								} else {
									// later merged into different class, prefix with this unmerge
									auto it = edits.emplace( 
										edits.begin(), key, Edit{ omode, root } );
									forKey.push_back( move(it) );
								}
							} break;
							default: unreachable("inconsistent mode");
						}
					} break;
					default: unreachable("invalid mode");
				}
			}

			oclasses = oclasses->get_base();
			omode = oclasses->get_mode();
		}
		assume( oclasses == classes, "classes must be versions of same map" );

		/// Edit item for binding changes
		struct BEdit {
			Bindings::Mode mode;  ///< Type of change to a key
			const Type* val;      ///< Updated key value, if applicable

			BEdit(Bindings::Mode m, const Type* v = nullptr) : mode(m), val(v) {}
		};

		// track binding merges
		bindings->reroot();
		std::unordered_map<const PolyType*, BEdit> bedits; // edits to base bindings
		
		// trace path from other environment
		const Bindings* bbindings = o.bindings;
		Bindings::Mode bmode = bbindings->get_mode();
		while ( bmode != Bindings::BASE ) {
			const PolyType* key = bbindings->get_key();
			auto it = bedits.find( key );

			if ( it == bedits.end() ) {
				// newly seen key, mark operation
				const Type* val = ( bmode == Bindings::REM ) ? nullptr : bbindings->get_val();
				bedits.emplace_hint( it, key, BEdit{ bmode, val } );
			} else {
				Bindings::Mode& smode = it->second.mode;
				switch ( bmode ) {
					case Bindings::REM: {
						// later insertion, change to update
						assume( smode == Bindings::INS, "inconsistent mode" );
						smode = Bindings::UPD;
					} break;
					case Bindings::INS: {
						switch ( smode ) {
							case Bindings::REM: {
								// later removal, no net change to map
								bedits.erase( it );
							} break;
							case Bindings::UPD: {
								// later update collapses to insertion
								smode = Bindings::INS;
							} break;
							default: unreachable("inconsistent mode");
						}
					} break;
					case Bindings::UPD: {
						// later removal or update overrides this update
						assume( smode == Bindings::REM || smode == Bindings::UPD, 
							"inconsistent mode");
					} break;
					default: unreachable("invalid mode");
				}
			}

			bbindings = bbindings->get_base();
			bmode = bbindings->get_mode();
		}
		assume( bbindings == bindings, "bindings must be versions of same map" );

		// merge typeclasses (always possible, can always merge all classes into one if the 
		// bindings unify)
		for ( const EditEntry& edit : edits ) {
			const PolyType* key = edit.first;
			const Edit& e = edit.second;
			auto bit = bedits.find( key );

			switch ( e.mode ) {
				case Classes::ADD: {
					// add new type variable
					classes = classes->add( key );
					// optionally add binding (none if later ADDTO)
					if ( bit != bedits.end() ) {
						const BEdit& be = bit->second;
						assume( be.mode == Bindings::INS, "inconsistent binding");
						bindings = bindings->set( key, be.val );
						bedits.erase( bit );
					}
				} break;
				case Classes::REM: {
					// do not remove type variable (merging)
					// if binding change (none if earlier REMFROM), check
					if ( bit != bedits.end() ) {
						const BEdit& be = bit->second;
						assume( be.mode == Bindings::REM, "inconsistent binding");
						// do not remove binding either
						bedits.erase( bit );
					}
				} break;
				case Classes::ADDTO: {
					// merge classes
					classes = classes->merge( e.root, key );
					// remove binding for new child (none if earlier ADD or REMFROM)
					if ( bit != bedits.end() ) {
						const BEdit& be = bit->second;
						assume( be.mode == Bindings::REM, "inconsistent binding");
						bindings = bindings->erase( key );
						bedits.erase( bit );
					}
				} break;
				case Classes::REMFROM: {
					// do not split classes
					// if binding for new root (none if later REM or ADDTO), leave in update list
					if ( bit != bedits.end() ) {
						const BEdit& be = bit->second;
						assume( be.mode == Bindings::INS, "inconsistent binding");
						// leave in update list to check later
					}
				} break;
				default: unreachable("invalid mode");
			}
		}

		// finish merging bindings -- all that should be left is updates on roots and inserts on 
		// new roots from REMFROM nodes. This may fail, or may merge further classes
		for ( const auto& entry : bedits ) {
			const PolyType* key = entry.first;
			const BEdit& e = entry.second;
			assume( e.mode == Bindings::UPD || e.mode == Bindings::INS, "inconsistent binding");
			
			// get reference for class, and attempt binding update
			ClassRef r{ this, classes->find( key ) };
			if ( ! mergeBound( r, e.val ) ) return false;
		}

		return true; // all edits applied successfully
	}

	/// Checks if the assertions of another environment are compatible with those of this. 
	/// Returns false if fails, but does NOT roll back partial changes.
	bool mergeAssns( const Env& o ) {
		/// Edit item for path state
		struct Edit {
			Assertions::Mode mode;  ///< Type of change to a key
			const TypedExpr* val;   ///< Updated key value, if applicable

			Edit(Assertions::Mode m, const TypedExpr* v = nullptr) : mode(m), val(v) {}
		};

		// track assertion changes
		assns->reroot();

		std::unordered_map<const FuncDecl*, Edit> edits;  // edits to base map
		unsigned n[] = { 0, 0, 0, 0 };  // number of nodes for each Mode
		
		// trace path from other environment
		const Assertions* oassns = o.assns;
		Assertions::Mode omode = oassns->get_mode();
		while ( omode != Assertions::BASE ) {
			const FuncDecl* key = oassns->get_key();
			auto it = edits.find( key );

			if ( it == edits.end() ) {
				// newly seen key, mark op
				const TypedExpr* val = ( omode == Assertions::REM ) ? nullptr : oassns->get_val();
				edits.emplace_hint( it, key, Edit{ omode, val } );
				++n[ omode ];
			} else {
				Assertions::Mode& smode = it->second.mode;
				switch ( omode ) {
					case Assertions::REM: {
						// later insertion, change to update
						assume( smode == Assertions::INS, "inconsistent mode" );
						--n[ smode ];
						smode = Assertions::UPD;
						++n[ smode ];
					} break;
					case Assertions::INS: {
						switch ( smode ) {
							case Assertions::REM: {
								// later removal, no net change to map
								--n[ smode ];
								edits.erase( it );
							} break;
							case Assertions::UPD: {
								// later update collapses to insertion
								--n[ smode ];
								smode = Assertions::INS;
								++n[ smode ];
							} break;
							default: unreachable("inconsistent mode");
						}
					} break;
					case Assertions::UPD: {
						// later removal or update overrides this update
						assume( smode == Assertions::REM || smode == Assertions::UPD, 
							"inconsistent mode");
					} break;
					default: unreachable("invalid mode");
				}
			}

			oassns = oassns->get_base();
			omode = oassns->get_mode();
		}
		assume( oassns == assns, "assertions must be versions of same map" );
		
		// apply edits
		if ( n[ Assertions::UPD ] == 0 ) {
			// can possibly short-circuit to one set or another if no updates to check

			// all changes are removal, no merge needed
			if ( n[ Assertions::INS ] == 0 ) return true;

			// all changes are insertions, just use other set
			if ( n[ Assertions::REM ] == 0 ) {
				assns = o.assns;
				return true;
			}
		}

		// apply insertions and check updates
		for ( const auto& e : edits ) {
			switch ( e.second.mode ) {
				case Assertions::INS: {
					// apply insertions
					assns = assns->set( e.first, e.second.val );
				} break;
				case Assertions::UPD: {
					// check updates
					if ( assns->get( e.first ) != e.second.val ) return false;
				} break;
				default: break; // ignore other cases
			}
		}

		return true; // all assertions sucessfully bound
	}

public:
	/// Merges the target environment with this one; both environments must be versions of the 
	/// same initial environment.
	/// Returns false if fails, but does NOT roll back partial changes.
	bool merge( const Env& o ) {
		// short-circuit for empty case
		if ( o.empty() ) return true;
		if ( empty() ) {
			*this = o; 
			return true;
		}
		// full merge
		return mergeAllClasses( o ) && mergeAssns( o );
	}

	/// Gets the list of unbound typeclasses in this environment
	std::vector<TypeClass> getUnbound() const {
		std::vector<TypeClass> out;
		for ( const auto& entry : *bindings ) {
			if ( entry.second == nullptr ) {
				out.push_back( *ClassRef{ this, entry.first } );
			}
		}
		return out;
	}

	/// Replaces type with bound value, if a substitution exists
	const Type* replace( const PolyType* pty ) const {
		const PolyType* root = classes->find_or_default( pty, nullptr );
		if ( ! root ) return as<Type>(pty);
		const Type* bound = bindings->get_or_default( root, nullptr );
		return bound ? bound : as<Type>(pty);
	}

	/// Replaces type with bound value, if type is a PolyType and a substitution exists.
	const Type* replace( const Type* ty ) const {
		if ( is<PolyType>(ty) ) return replace( as<PolyType>(ty) );
		else return ty;
	}

private:
	/// Writes out this environment
    std::ostream& write(std::ostream& out) const;
};

void ClassRef::update_root() {
	root = env->classes->find( root );
}

List<PolyType> ClassRef::get_vars() const {
	List<PolyType> vars;
	env->classes->find_class( root, std::back_inserter(vars) );
	return vars;
}

const Type* ClassRef::get_bound() const {
	return env->bindings->get_or_default( root, nullptr );
}

TypeClass ClassRef::get_class() const { return { get_vars(), get_bound() }; }

inline std::ostream& operator<< (std::ostream& out, const Env& env) {
    return env.write( out );
}

inline void swap( Env& a, Env& b ) { a.swap( b ); }
