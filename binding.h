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

    const std::string& name;  ///< Name for this type binding; not required to be unique
private:
    Map bindings_;            ///< Bindings from a named type variable to another type
    unsigned long unbound_;   ///< Count of unbound type variables

public:
    /// Name-only constructor
    TypeBinding( const std::string& name ) : name(name), bindings_(), unbound_(0) {}

    /// Constructor that takes a pair of iterators for a range of strings, the names 
    /// of the variables to be bound.
    template<typename It>
    TypeBinding( const std::string& name, It begin, It end ) 
        : name(name), bindings_(), unbound_(0) {
        while ( begin != end ) {
            bindings_.emplace( *begin, nullptr );
            ++unbound_;

            ++begin;
        }
    }

    unsigned long unbound() const { return unbound_; }

    /// true iff no bindings
    bool empty() const { return bindings_.empty(); }

    /// Returns the type corresponding to the given name.
    /// Returns nullptr if the name has not yet been bound, fails if the key is not in the map.
    const Type* operator[] ( const std::string& name ) const {
        auto it = bindings_.find( name );
        assert( it != bindings_.end() && "type not in binding map" );
        return it->second;
    }

    /// Will modify the binding map by replacing an unbound mapping for `name` with `type`. 
    /// Fails if `name` is not a key in the map. Returns the previous binding for `name` 
    /// (unchanged if not nullptr).
    const Type* bind( const std::string& name, const Type* type ) {
        Map::iterator it = bindings_.find( name );
        assert( it != bindings_.end() && "type not in binding map" );
        // early return if name already bound 
        if ( it->second ) return it->second;
        // break const-ness to perform mutating update
        it->second = type;
        --unbound_;
        return nullptr;
    }
};