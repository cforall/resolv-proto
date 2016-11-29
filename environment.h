#pragma once

#include <cstddef>
#include <ostream>
#include <unordered_map>
#include <vector>

#include "cow.h"
#include "data.h"
#include "gc.h"
#include "type.h"

/// Binding for otherwise-unbound return type variables in an interpretation
class Environment : public GC_Traceable {
    friend std::ostream& operator<< (std::ostream&, const Environment&);

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
        
        TypeClass( const PolyType* orig, nullptr_t )
            : vars{ orig }, bound(nullptr) {}
        
        /// Sets bound to o.bound if bound is unset.
        /// Returns false if both bound and o.bound are set and unequal 
        bool mergeBound( const TypeClass& o ) {
            if ( o.bound ) {
                if ( bound ) {
                    // class bound incompatible with existing bound
                    if ( *bound != *o.bound ) return false;
                } else {
                    bound = o.bound;
                }
            }
        }
    };

    /// Backing storage
    typedef std::vector<TypeClass> Storage;
    /// Underlying map type 
    typedef std::unordered_map< const PolyType*, Storage::size_type > Map;

    Storage classes;          ///< Backing storage for type classes
    Map bindings;             ///< Bindings from a named type variable to another type
 
    /// Inserts a new typeclass, mapping `orig` to `sub`; 
    /// environment should be initialized and `orig` should not be present.
    void insert( const PolyType* orig, const Type* sub = nullptr ) {
        bindings[ orig ] = classes.size();
        classes.emplace_back( orig, sub );
    }

    /// Inserts a new typeclass, mapping `orig` and `added` to null.
    /// Environment should be initialized and neither `orig` nor `added` should be present.
    void insert( const PolyType* orig, const PolyType* added ) {
        bindings[ orig ] = bindings[ added ] = classes.size();
        classes.emplace_back( orig, added );
    }

    /// Inserts a new typeclass as  copy of another; none of the contained types should be 
    /// in the map already
    void insertClass( const TypeClass& c ) {
        Storage::size_type cid = classes.size();
        
        classes.push_back( c );
    }

public:
    /// Constructs a brand new environment with a single binding
    Environment( const PolyType* orig, const Type* sub = nullptr ) : classes(), bindings() {
        insert( orig, sub );
    }

    /// Constructs a brand new environment with two poly types bound together
    Environment( const PolyType* orig, const PolyType* added ) : classes(), bindings() {
        insert( orig, added );
    }

    Environment( const PolyType* orig, nullptr_t ) : classes(), bindings() {
        insert( orig );
    }
    
    /// Finds a mapping in this environment, returns nullptr if none
    const Type* find( const PolyType* orig ) const {
        auto it = bindings.find( orig );
        return it == bindings.end() ? nullptr : classes[ it->second ].bound;
    }

    /// Adds the given substitution to this binding. `orig` should be currently unbound.
    void bind( const PolyType* orig, const Type* sub ) {
        auto it = bindings.find( orig );
        if ( it == bindings.end() ) {
            insert( orig, sub );
        } else {
            classes[ it->second ].bound = sub;
        }
    }

    /// Adds the second PolyType to the class of the first; the second PolyType should not 
    /// currently exist in the binding table.
    void bindClass( const PolyType* orig, const PolyType* added ) {
        auto it = bindings.find( orig );
        if ( it == bindings.end() ) {
            insert( orig, added );
        } else {
            classes[ it->second ].vars.push_back( added );
        }
    }

    /// Merges the target environment with this one; returns false if fails, but does 
    /// not roll back partial changes.
    bool merge( const Environment& o ) {
        for ( const TypeClass& c : o.classes ) {
            Storage::size_type cid = classes.size(); // ID of matching class

            Storage::size_type n = c.vars.size();
            Storage::size_type i;
            for ( i = 0; i < n; ++i ) {
                // look for first existing bound variable
                auto it = bindings.find( c.vars[i] );
                if ( it == bindings.end() ) continue;
                
                // merge binding into class
                if ( ! classes[ it->second ].mergeBound( c ) ) return false;
                break;
            }
            for ( ; i < n; ++i ) {  // only executes if class found above
                // scan for conflicting bound variables
                auto it = bindings.find( c.vars[i] );
                if ( it == bindings.end() || it->second == cid ) continue;

                // merge new class into existing
                Storage::size_type c2id = it->second;
                TypeClass& d = classes[cid];
                TypeClass& d2 = classes[c2id];
                // merge bounds
                if ( ! d.mergeBound( d2 ) ) return false;
                // merge vars
                d.vars.reserve( d.vars.size() + d2.vars.size() );
                for ( const PolyType* v : d2.vars ) {
                    d.vars.push_back( v );
                    bindings[v] = cid;
                }
                // erase second class
                Storage::size_type victim = classes.size()-1;
                if ( c2id != victim ) {
                    classes[c2id] = move(classes[victim]);
                    for ( const PolyType* v : classes[c2id].vars ) {
                        bindings[v] = c2id;
                    }
                }
                classes.pop_back();
            }

            if ( cid == classes.size() ) {
                // no bound variables, insert new class at clif ( ! env )asses.size() [== cid]
                classes.push_back( c );
            }

            // set bindings for added class
            for ( const PolyType* var : c.vars ) {
                bindings[ var ] = cid;
            }
        }
    }

    /// Applies the bindings in this environment to their local type bindings
    /// Warning: breaks constness on source bindings
    void apply() const {
        for ( const TypeClass& entry : classes ) {
            if ( ! entry.bound ) continue;
            for ( const PolyType* ty : entry.vars ) {
                as_non_const( ty->src() )->bind( ty->name(), entry.bound );
            }
        }
    }

protected:
    virtual void trace(const GC& gc) const {
        for ( const TypeClass& entry : classes ) {
            gc << entry.vars << entry.bound;
        }
    }

private:
    /// Writes out this environment
    std::ostream& write(std::ostream& out) const {
        out << "{";
        auto it = classes.begin();
        while (true) {
            if ( it->vars.size() == 1 ) {
                out << *it->vars.front();
            } else {
                out << "[";
                auto jt = it->vars.begin();
                while (true) {
                    out << *jt;
                    if ( ++jt == it->vars.end() ) break;
                    out << ", ";
                }
                out << "]";
            }
            out << " => ";
            
            if ( it->bound ) { out << *it->bound; }
            else { out << "???"; }

            if ( ++it == classes.end() ) break;
            out << ", ";
        }
        return out << "}";
    } 
};

/// Returns nullptr for unbound type, contained type otherwise.
/// Handles `env == null` correctly
inline const Type* find( const cow_ptr<Environment>& env, const PolyType* orig ) {
    return env ? env->find( orig ) : nullptr;
}

/// Adds the given substitution to this binding. `orig` should be currently unbound.
/// Creates new environment if `env == null` 
inline void bind( cow_ptr<Environment>& env, const PolyType* orig, const Type* sub ) {
    if ( ! env ) {
        env.reset( new Environment(orig, sub) );
    } else {
        env.mut().bind( orig, sub );
    }
}

/// Adds the second PolyType to the class of the first; the second PolyType should not 
/// currently exist in the binding table. Creates new environment if `env == null`
inline void bindClass( cow_ptr<Environment>& env, const PolyType* orig, const PolyType* added ) {
    if ( ! env ) {
        env.reset( new Environment( orig, added ) );
    } else {
        env.mut().bindClass( orig, added );
    }
}

/// Merges b into a, returning false and nulling a on failure
inline bool merge( cow_ptr<Environment>& a, const cow_ptr<Environment>& b ) {
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

// Applies an environment down to its local type bindings (will modify const bindings)
inline void apply( const cow_ptr<Environment>& env ) {
    if ( env ) env->apply();
}

inline std::ostream& operator<< (std::ostream& out, const Environment& env) {
    return env.write( out );
}
