#pragma once

#include <string>
#include <unordered_map>

#include "data.h"
#include "type.h"

class TypeBinding {
public:
    /// Underlying map type
    typedef std::unordered_map< std::string, const Type* > Map;
private:
    Map bindings_;           ///< Bindings from a named type variable to another type
    unsigned long unbound_;  ///< Count of unbound type variables
public:
    TypeBinding() = default;
    TypeBinding( const TypeBinding& ) = default;
    TypeBinding( TypeBinding&& ) = default;
    TypeBinding& operator= ( const TypeBinding& ) = default;
    TypeBinding& operator= ( TypeBinding&& ) = default;

    /// Constructor that takes a pair of iterators for a range of strings, the names 
    /// of the variables to be bound.
    template<typename It>
    TypeBinding( It begin, It end ) : bindings_(), unbound_(0) {
        while ( begin != end ) {
            bindings_.emplace( *begin, nullptr );
            ++unbound_;

            ++begin;
        }
    }

    /// Clones, changing one existing binding
    TypeBinding clone_and_rebind( const std::string& name, const Type* type ) const {
        TypeBinding tb{ *this };
        auto it = tb.bindings_.find( name );
        assert( it != tb.bindings_.end() && "type not in binding map" );
        assert( it->second && "no existing binding to overwrite" );
        it->second = type;
        return tb;
    }

    const Map& bindings() const { return bindings_; }
    unsigned long unbound() const { return unbound_; }

    /// Returns the type corresponding to the given name.
    /// Will fail if the key is not in the map.
    const Type* operator[] ( const std::string& name ) const {
        auto it = bindings_.find( name );
        assert( it != bindings_.end() && "type not in binding map" );
        return it->second;
    }

    /// Inserts a new binding into the map. The key should not already be present.
    void insert( const std::string& name, const Type* type = nullptr ) {
        auto inserted = bindings_.emplace( name, type );
        assert( inserted.second && "attempted to overwrite existing binding" );
        if ( ! type ) { ++unbound_; }
    }

    /// Will modify the binding map by replacing an unbound mapping for `name` with `type`. 
    /// Fails if `name` is not a key in the map. Returns the previous binding for `name` 
    /// (unchanged if not nullptr).
    const Type* bind( const std::string& name, const Type* type ) const {
        Map::iterator it = as_non_const(bindings_).find( name );
        assert( it != as_non_const(bindings_).end() && "type not in binding map" );
        // early return if name already bound 
        if ( it->second ) return it->second;
        // break const-ness to perform mutating update
        it->second = type;
        --as_non_const(unbound_);
        return nullptr;
    }
};
