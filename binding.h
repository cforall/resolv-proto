#pragma once

#include <cassert>
#include <ostream>
#include <string>
#include <unordered_map>

class GC;
class Type;

/// Binding for parameter type variables in a call expression.
class TypeBinding {
    friend std::ostream& operator<< (std::ostream&, const TypeBinding&);
    friend const GC& operator<< (const GC&, const TypeBinding*);

public:
    /// Underlying map type
    typedef std::unordered_map< std::string, const Type* > Map;

    std::string name;         ///< Name for this type binding; not required to be unique
private:
    Map bindings_;            ///< Bindings from a named type variable to another type
    unsigned long unbound_;   ///< Count of unbound type variables
    bool dirty_;              ///< Type binding has been changed since last check

public:
    TypeBinding() = default;

    /// Name-only constructor
    TypeBinding( const std::string& name ) : name(name), bindings_(), unbound_(0), dirty_(false) {}

    /// Constructor that takes a pair of iterators for a range of strings, the names 
    /// of the variables to be bound.
    template<typename It>
    TypeBinding( const std::string& name, It begin, It end ) 
        : name(name), bindings_(), unbound_(0), dirty_(true) {
        while ( begin != end ) {
            bindings_.emplace( *begin, nullptr );
            ++unbound_;

            ++begin;
        }
    }

    void set_name( const std::string& n ) { name = n; }

    unsigned long unbound() const { return unbound_; }

    /// true iff no bindings
    bool empty() const { return bindings_.empty(); }

    /// true iff the map contains a binding with the given name
    bool contains( const std::string& name ) const {
        return bindings_.count( name ) > 0;
    }

    /// Returns the type corresponding to the given name.
    /// Returns nullptr if the name has not yet been bound, fails if the key is not in the map.
    const Type* operator[] ( const std::string& name ) const {
        auto it = bindings_.find( name );
        assert( it != bindings_.end() && "type not in binding map" );
        return it->second;
    }

    /// Adds a name to the binding map; does nothing if name already present
    void add( const std::string& name ) {
        if ( contains( name ) ) return;
        bindings_.emplace( name, nullptr );
        ++unbound_;
        dirty_ = true;
    }

    /// Will modify the binding map by replacing an unbound mapping for `name` with `type`. 
    /// Fails if `name` is not a key in the map. Returns the previous binding for `name` 
    /// (unchanged if not nullptr).
    const Type* bind( const std::string& name, const Type* type ) {
        Map::iterator it = bindings_.find( name );
        assert( it != bindings_.end() && "type not in binding map" );
        // early return if name already bound 
        if ( it->second ) return it->second;
        if ( type ) {
            it->second = type;
            --unbound_;
            dirty_ = true;
        }
        return nullptr;
    }

    /// Modifies the binding map to replace a mapping for `name` with `type`.
    /// Returns previous binding for `name` (nullptr for none such). 
    /// Can be used to unbind name by updating to nullptr.
    const Type* update( const std::string& name, const Type* type ) {
        Map::iterator it = bindings_.find( name );
        assert( it != bindings_.end() && "type not in binding map" );
        if ( it->second == type ) return type;
        const Type* ret = it->second;
        it->second = type;
        if ( ! ret && type ) { --unbound_; } else if ( ret && ! type ) { ++unbound_; }
        dirty_ = true;
        return ret; 
    }

    /// Checks if the type binding is dirty; resets the dirty state to false regardless
    bool dirty() {
        if ( dirty_ ) {
            dirty_ = false;
            return true;
        } else return false;
    }
};
