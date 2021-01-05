#pragma once

// Copyright (c) 2015 University of Waterloo
//
// The contents of this file are covered under the licence agreement in 
// the file "LICENCE" distributed with this repository.

#include <cstddef>
#include <functional>
#include <iterator>
#include <map>
#include <memory>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include "ast/forall.h"
#include "ast/type.h"
#include "ast/type_visitor.h"
#include "data/cast.h"
#include "data/collections.h"
#include "data/debug.h"
#include "data/list.h"
#include "data/mem.h"
#include "data/option.h"
#include "data/range.h"

/// Key type for ConcType
struct ConcKey {
    int id;

    ConcKey( int id ) : id(id) {}
    ConcKey( const ConcType* ct ) : id( ct->id() ) {}

    const Type* value() const { return new ConcType{ id }; }

    bool operator== (const ConcKey& o) const { return id == o.id; }
    bool operator< (const ConcKey& o) const { return id < o.id; }

    std::size_t hash() const { return std::hash<int>{}(id); }
};

/// Key type for NamedType
struct NamedKey {
    std::string name;
    unsigned params;
    
    NamedKey( const std::string& name, List<Type>::size_type params )
        : name(name), params(params) {}
    NamedKey( const NamedType* nt ) : name( nt->name() ), params( nt->params().size() ) {}

    const Type* value() const { return new NamedType{ name, List<Type>( params, nullptr ) }; }

    bool operator== (const NamedKey& o) const { return name == o.name && params == o.params; }
    bool operator< (const NamedKey& o) const {
        int c = name.compare(o.name);
        return c < 0 || (c == 0 && params < o.params);
    }

    std::size_t hash() const { return (std::hash<std::string>{}(name) << 1) ^ params; }
};

/// Key type for PolyType
struct PolyKey {
    std::string name;
    unsigned id;

    PolyKey( const std::string& name, unsigned id ) : name(name), id(id) {}
    PolyKey( const PolyType* pt ) : name( pt->name() ), id( pt->id() ) {}

    const Type* value() const { return new PolyType{ name, id }; }

    bool operator== (const PolyKey& o) const {
        return id == 0 ? o.id == 0 : name == o.name;
    }
    bool operator< (const PolyKey& o) const {
        return id == 0 
            ? o.id > 0 || name < o.name
            : id < o.id;
    }

    std::size_t hash() const {
        return (std::hash<std::string>{}( name ) << 1) ^ id;
    }
};

/// Key type for FuncType
struct FuncKey {
    unsigned params;

    FuncKey( List<Type>::size_type params ) : params(params) {}
    FuncKey( const FuncType* ft ) : params( ft->params().size() ) {}

    const Type* value() const { return new FuncType{ List<Type>( params, nullptr ), nullptr }; }

    bool operator== (const FuncKey& o) const { return params == o.params; }
    bool operator< (const FuncKey& o) const { return params < o.params; }

    std::size_t hash() const { return params; }
};

/// Variants of a key
enum class KeyMode { Conc, Named, Poly, Func };

/// Associates key type and enum with node type
template<typename Ty>
struct key_info {};

template<>
struct key_info<ConcType> {
    typedef ConcKey type;
    static constexpr KeyMode key = KeyMode::Conc;
};

template<>
struct key_info<NamedType> {
    typedef NamedKey type;
    static constexpr KeyMode key = KeyMode::Named;
};

template<>
struct key_info<PolyType> {
    typedef PolyKey type;
    static constexpr KeyMode key = KeyMode::Poly;
};

template<>
struct key_info<FuncType> {
    typedef FuncKey type;
    static constexpr KeyMode key = KeyMode::Func;
};

/// Variant key type for map
class TypeKey {
    KeyMode key_type;  ///< Variant discriminator
    //std::aligned_union_t<0, ConcKey, NamedKey, PolyKey> data;  ///< Variant storage
    union data_t {
        char def;
        ConcKey conc;
        NamedKey named;
        PolyKey poly;
        FuncKey func;

        data_t() : def('\0') {}
        ~data_t() {}
    } data;

    template<typename K>
    K& get() { return reinterpret_cast<K&>(data); }
    template<typename K>
    const K& get() const { return reinterpret_cast<const K&>(data); }
    template<typename K>
    K&& take() { return move(get<K>()); }

    template<typename T, typename... Args>
    void init( Args&&... args ) {
        using KT = typename key_info<T>::type;
        key_type = key_info<T>::key;
        new( &get<KT>() ) KT { forward<Args>(args)... };
    }

    void init_from( const TypeKey& o ) {
        switch ( o.key_type ) {
        case KeyMode::Conc:
            init<ConcType>( o.get<ConcKey>() );
            break;
        case KeyMode::Named:
            init<NamedType>( o.get<NamedKey>() );
            break;
        case KeyMode::Poly:
            init<PolyType>( o.get<PolyKey>() );
            break;
        case KeyMode::Func:
            init<FuncType>( o.get<FuncKey>() );
            break;
        }
    }

    void init_from( TypeKey&& o ) {
        switch ( o.key_type ) {
        case KeyMode::Conc:
            init<ConcType>( o.take<ConcKey>() );
            break;
        case KeyMode::Named:
            init<NamedType>( o.take<NamedKey>() );
            break;
        case KeyMode::Poly:
            init<PolyType>( o.take<PolyKey>() );
            break;
        case KeyMode::Func:
            init<FuncType>( o.take<FuncKey>() );
            break;
        }
    }

    /// Clears variant fields
    void reset() {
        switch ( key_type ) {
        case KeyMode::Conc:
            get<ConcKey>().~ConcKey();
            break;
        case KeyMode::Named:
            get<NamedKey>().~NamedKey();
            break;
        case KeyMode::Poly:
            get<PolyKey>().~PolyKey();
            break;
        case KeyMode::Func:
            get<FuncKey>().~FuncKey();
            break;
        }
    }

    /// Default constructor only for internal use
    TypeKey() {}

public:
    /// Constructs key from type; type should be ConcType, NamedType, PolyType, or FuncType
    TypeKey( const Type* t ) {
        auto tid = typeof(t);
        if ( tid == typeof<ConcType>() ) {
            init<ConcType>( as<ConcType>(t) );
        } else if ( tid == typeof<NamedType>() ) {
            init<NamedType>( as<NamedType>(t) );
        } else if ( tid == typeof<PolyType>() ) {
            init<PolyType>( as<PolyType>(t) );
        } else if ( tid == typeof<FuncType>() ) {
            init<FuncType>( as<FuncType>(t) );
        } else unreachable("Invalid key type");
    }

    TypeKey( const ConcType* t ) { init<ConcType>( t ); }

    TypeKey( const NamedType* t ) { init<NamedType>( t ); }

    TypeKey( const PolyType* t ) { init<PolyType>( t ); }

    TypeKey( const FuncType* t ) { init<FuncType>( t ); }

    // deliberately no nullptr_t implmentation; no meaningful value

    TypeKey( const TypeKey& o ) { init_from( o ); }

    TypeKey( TypeKey&& o ) { init_from( move(o) ); }

    template<typename T, typename... Args>
    TypeKey( Args&&... args ) { init<T>( forward<Args>(args)... ); }

    /// makes a key for a Type T
    template<typename T, typename... Args>
    static TypeKey make( Args&&... args ) {
        TypeKey k;
        k.init<T>( forward<Args>(args)... );
        return k;
    }

    TypeKey& operator= ( const TypeKey& o ) {
        if ( this == &o ) return *this;
        if ( key_type == o.key_type ) {
            switch( key_type ) {
            case KeyMode::Conc:
                get<ConcKey>() = o.get<ConcKey>();
                break;
            case KeyMode::Named:
                get<NamedKey>() = o.get<NamedKey>();
                break;
            case KeyMode::Poly:
                get<PolyKey>() = o.get<PolyKey>();
                break;
            case KeyMode::Func:
                get<FuncKey>() = o.get<FuncKey>();
                break;
            }
        } else {
            reset();
            init_from( o );
        }

        return *this;
    }

    TypeKey& operator= ( TypeKey&& o ) {
        if ( this == &o ) return *this;
        if ( key_type == o.key_type ) {
            switch( key_type ) {
            case KeyMode::Conc:
                get<ConcKey>() = o.take<ConcKey>();
                break;
            case KeyMode::Named:
                get<NamedKey>() = o.take<NamedKey>();
                break;
            case KeyMode::Poly:
                get<PolyKey>() = o.take<PolyKey>();
                break;
            case KeyMode::Func:
                get<FuncKey>() = o.take<FuncKey>();
                break;
            }
        } else {
            reset();
            init_from( move(o) );
        }

        return *this;
    }

    ~TypeKey() { reset(); }

    /// Retrieves the underlying key
    const Type* get() const {
        switch ( key_type ) {
        case KeyMode::Conc:
            return get<ConcKey>().value();
        case KeyMode::Named:
            return get<NamedKey>().value();
        case KeyMode::Poly:
            return get<PolyKey>().value();
        case KeyMode::Func:
            return get<FuncKey>().value();
        default: unreachable("Invalid key type"); return nullptr;
        }
    } 

    bool operator== ( const TypeKey& o ) const {
        if ( key_type != o.key_type ) return false;
        switch ( key_type ) {
        case KeyMode::Conc:
            return get<ConcKey>() == o.get<ConcKey>();
        case KeyMode::Named:
            return get<NamedKey>() == o.get<NamedKey>();
        case KeyMode::Poly:
            return get<PolyKey>() == o.get<PolyKey>();
        case KeyMode::Func:
            return get<FuncKey>() == o.get<FuncKey>();
        default: unreachable("Invalid key type"); return false;
        }
    }

    bool operator< ( const TypeKey& o ) const {
        if ( key_type != o.key_type ) return (int)key_type < (int)o.key_type;
        switch ( key_type ) {
        case KeyMode::Conc:
            return get<ConcKey>() < o.get<ConcKey>();
        case KeyMode::Named:
            return get<NamedKey>() < o.get<NamedKey>();
        case KeyMode::Poly:
            return get<PolyKey>() < o.get<PolyKey>();
        case KeyMode::Func:
            return get<FuncKey>() < o.get<FuncKey>();
        default: unreachable("Invalid key type"); return false;
        }
    }

    std::size_t hash() const {
        std::size_t h;
        switch ( key_type ) {
        case KeyMode::Conc:
            h = get<ConcKey>().hash();
            break;
        case KeyMode::Named:
            h = get<NamedKey>().hash();
            break;
        case KeyMode::Poly:
            h = get<PolyKey>().hash();
            break;
        case KeyMode::Func:
            h = get<FuncKey>().hash();
            break;
        default: unreachable("Invalid key type"); return 0;
        }
        return (h << 2) | (std::size_t)key_type;
    }

    /// Gets the key mode
    KeyMode mode() const { return key_type; }

    /// Gets the number of children used by this key to complete the type
    unsigned children() const {
        if ( key_type == KeyMode::Named ) return get<NamedKey>().params;
        if ( key_type == KeyMode::Func ) return get<FuncKey>().params + 1;
        return 0;
    }

    /// Gets the underlying key variant, or empty for mismatch
    template<typename T>
    option<typename key_info<T>::type> key_for() const {
        if ( key_type != key_info<T>::key ) return {};
        return { get<typename key_info<T>::type>() };
    }
};

struct KeyHash {
    std::size_t operator() ( const TypeKey& k ) const { return k.hash(); }
};

/// Key sequence representing a type
using AtomList = std::vector<TypeKey>;

/// Generates key sequence for a type
template< typename OutIter = std::back_insert_iterator<AtomList> >
class TypeAtoms : public TypeVisitor<TypeAtoms<OutIter>, OutIter> {
public:
	using Super = TypeVisitor<TypeAtoms<OutIter>, OutIter>;
	using Super::visit;
    using Super::visitChildren;

	bool visit( const ConcType* t, OutIter& out ) {
		*out++ = TypeKey{ t };
		return true;
	}

	bool visit( const NamedType* t, OutIter& out ) {
		*out++ = TypeKey{ t };
		return visitChildren( t, out );
	}

	bool visit( const PolyType* t, OutIter& out ) {
        *out++ = TypeKey{ t };
        return true;
    }

    bool visit( const FuncType* t, OutIter& out ) {
        *out++ = TypeKey{ t };
        return visitChildren( t, out );
    }

    /// Visits the iterator provided by reference
    OutIter& operator() ( const Type* t, OutIter& out ) {
        visit( t, out );
        return out;
    }

    /// Visits the iterator provided by move
    OutIter operator() ( const Type* t, OutIter&& out ) {
        visit( t, out );
        return out;
    }
};

/// Generates key sequence for a type
static inline AtomList type_atoms( const Type* t ) {
    AtomList out;
    TypeAtoms<>{}( t, std::back_inserter( out ) );
    return out;
}

/// A map from types to some value; lookup is done by structural decomposition on types.
template<typename Value>
class TypeMap {
public:
    using key_type = const Type*;
    using mapped_type = Value;
    using value_type = std::pair<const Type* const, Value>;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using key_compare = ByValueCompare<Type>;
    using reference = value_type&;
    using const_reference = const value_type&;
    using pointer = value_type*;
    using const_pointer = const value_type*;

private:
    /// Underlying map type
    template<typename V>
    using KeyMap =
    #ifdef RP_SORTED
         std::map<TypeKey, V>;
    #else
        std::unordered_map<TypeKey, V, KeyHash>;
    #endif

    /// Optional leaf value for map
    using Leaf = std::unique_ptr<value_type>;
    /// Map type for child types
    using ConcMap = KeyMap< std::unique_ptr<TypeMap<Value>> >;
    /// Secondary index of polymorphic children
    using PolyList = std::vector< std::pair<PolyKey, TypeMap<Value>*> >;

    Leaf leaf;       ///< Storage for leaf values
    ConcMap nodes;   ///< Storage for child types
    PolyList polys;  ///< Secondary index of polymorphic children

    struct Iter {
        using Base = TypeMap<Value>;         ///< Underlying type map
        using ConcIter = typename ConcMap::iterator;  ///< Concrete map iterator

        /// Backtracking information for iteration
        struct Backtrack {
            ConcIter it;  ///< Iterator for concrete map
            Base* base;   ///< TypeMap containing concrete map

            Backtrack() = default;
            Backtrack( const ConcIter& i, Base* b ) : it( i ), base( b ) {}
            Backtrack( ConcIter&& i, Base* b ) : it( move(i) ), base( b ) {}
            
            bool operator== ( const Backtrack& o ) const { return it == o.it && base == o.base; }
            bool operator!= ( const Backtrack& that ) const { return !( *this == that ); }
        };
        /// Stack of backtrack information
        using BacktrackList = std::vector<Backtrack>;
        
        Base* base;            ///< Current concrete map; null if none such
        BacktrackList prefix;  ///< Prefix of tuple types

        /// Steps into the children of the current node
        inline void push_prefix() {
            prefix.emplace_back( base->nodes.begin(), base );
            base = prefix.back().it->second.get();
        }

        /// Default constructor; use for end-of-map iterator
        Iter() = default;

        /// End-of-map constructor
        Iter( std::nullptr_t b ) : base(b), prefix() {}

        /// Initialize to begin() value for base [should not be nullptr]
        Iter( Base* b ) : base(b), prefix() {
            // if base has leaf, is valid begin iterator
            if ( base->leaf ) return;

            // only an empty map has neither leaf nor nodes;
            // guaranteed that all empty maps will be top-level
            if ( base->nodes.empty() ) {
                base = nullptr;
                return;
            }

            // iterate down to first leaf-containing node (guaranteed to exist)
            do { push_prefix(); } while ( ! base->leaf ); 
        }

        /// Unchecked field-wise constructor
        Iter( Base* b, BacktrackList&& p ) : base(b), prefix( move(p) ) {}

        /// Get the key stored in the map at this iterator (should not be end())
        const Type* key() const { return base->leaf->first; }

        /// Get the value stored in the map at this iterator (should not be end())
        Value& get() { return base->leaf->second; }
        const Value& get() const { return &base->leaf->second; }

        reference operator* () { return *base->leaf; }
        const_reference operator* () const { return *base->leaf; }

        pointer operator-> () { return base->leaf.get(); }
        const_pointer operator-> () const { return base->leaf.get(); }

        Iter& operator++ () {
            // will crash if called on end iterator
            if ( ! base->nodes.empty() ) {
                // move to child type (guaranteed to either have leaf or nodes)
                do { push_prefix(); } while ( ! base->leaf );
                return *this;
            } else while ( ! prefix.empty() ) {
                // move to sibling type
                auto& crnt = prefix.back();
                ++crnt.it;

                if ( crnt.it == crnt.base->nodes.end() ) {
                    // back out to parent type if no sibling
                    prefix.pop_back();
                    continue;
                }

                base = crnt.it->second.get();
                while ( ! base->leaf ) { push_prefix(); }
                return *this;
            }

            base = nullptr;
            return *this;
        }

        bool operator== ( const Iter& o ) const {
            return base == o.base && prefix == o.prefix;
        }
        bool operator!= ( const Iter& that ) const { return !(*this == that); }

        explicit operator bool () const { return base != nullptr; }
    };
    using Backtrack = typename Iter::Backtrack;
    using BacktrackList = typename Iter::BacktrackList;

public:
    class iterator : public std::iterator<
            std::input_iterator_tag,
            value_type,
            long,
            pointer,
            reference> {
    friend TypeMap;
    friend class const_iterator;
        Iter i;
    
        iterator( Iter&& i ) : i( move(i) ) {}
        iterator( std::nullptr_t p ) : i( p ) {}
        iterator( TypeMap<Value>* p ) : i( p ) {}
    public:
        iterator() = default;

        const Type* key() { return i.key(); }
        Value& get() { return i.get(); }

        reference operator* () { return *i; }
        pointer operator-> () { return i.operator->(); }

        iterator& operator++ () { ++i; return *this; }
        iterator& operator++ (int) { iterator tmp = *this; ++i; return tmp; }

        bool operator== ( const iterator& o ) const { return i == o.i; }
        bool operator!= ( const iterator& o ) const { return i != o.i; }

        explicit operator bool () const { return (bool)i; }
    };

    class const_iterator : public std::iterator<
            std::input_iterator_tag,
            value_type,
            long,
            const_pointer,
            const_reference> {
    friend TypeMap;
        Iter i;
    
        const_iterator( Iter&& i ) : i( move(i) ) {}
        const_iterator( std::nullptr_t ) : i( nullptr ) {}
        const_iterator( const TypeMap<Value>* p ) : i( as_non_const(p) ) {}
    public:
        const_iterator() = default;
        const_iterator( const iterator& o ) : i(o.i) {}
        const_iterator( iterator&& o ) : i( move(o.i) ) {}

        const Type* key() { return i.key(); }
        const Value& get() { return i.get(); }

        const_reference operator* () { return *i; }
        const_pointer operator-> () { return i.operator->(); }

        const_iterator& operator++ () { ++i; return *this; }
        const_iterator& operator++ (int) { const_iterator tmp = *this; ++i; return tmp; }

        bool operator== ( const const_iterator& o ) const { return i == o.i; }
        bool operator!= ( const const_iterator& o ) const { return !(*this == o); }

        explicit operator bool () const { return (bool)i; }
    };

    iterator begin() { return iterator{ this }; }
    const_iterator begin() const { return const_iterator{ this }; }
    const_iterator cbegin() const { return const_iterator{ this }; }
    iterator end() { return iterator{}; }
    const_iterator end() const { return const_iterator{}; }
    const_iterator cend() const { return const_iterator{}; }

    bool empty() const { return (!leaf) && nodes.empty(); }

    void clear() {
        leaf.reset();
        nodes.clear();
        polys.clear();
    }

    /// Gets the key for this map, or nullptr if no node stored at that key
    const Type* key() const { return leaf ? leaf->first : nullptr; }

    /// Gets the leaf value, or nullptr for none such. 
    Value* get() { return leaf ? &leaf->second : nullptr; }
    const Value* get() const { return leaf ? &leaf->second : nullptr; }

    /// Gets the TypeMap for types starting with this type, or nullptr for none such.
    TypeMap<Value>* get( std::nullptr_t ) { return nullptr; }
    const TypeMap<Value>* get( std::nullptr_t ) const { return nullptr; }

    TypeMap<Value>* get( const VoidType* ) { return this; }
    const TypeMap<Value>* get( const VoidType* ) const { return this; }

    TypeMap<Value>* get( const ConcType* ty ) {
        auto it = nodes.find( TypeKey{ ty } );
        return it == nodes.end() ? nullptr : it->second.get();
    }
    const TypeMap<Value>* get( const ConcType* ty ) const {
        return as_non_const(this)->get( ty );
    }

    TypeMap<Value>* get( const NamedType* ty ) {
        auto it = nodes.find( TypeKey{ ty } );
        return it == nodes.end() ? nullptr : it->second->get( ty->params() );
    }
    const TypeMap<Value>* get( const NamedType* ty ) const {
        return as_non_const(this)->get( ty );
    }

    TypeMap<Value>* get( const PolyType* ty ) {
        auto it = nodes.find( TypeKey{ ty } );
        return it == nodes.end() ? nullptr : it->second.get();
    }
    const TypeMap<Value>* get( const PolyType* ty ) const {
        return as_non_const(this)->get( ty );
    }

    TypeMap<Value>* get( const TupleType* ty ) {
        return get( ty->types() );
    }
    const TypeMap<Value>* get( const TupleType* ty ) const {
        return as_non_const(this)->get( ty );
    }

    TypeMap<Value>* get( const FuncType* ty ) {
        auto it = nodes.find( TypeKey{ ty } );
        if ( it == nodes.end() ) return nullptr;
        TypeMap<Value>* tm = it->second->get( ty->params() );
        return tm ? tm->get( ty->returns() ) : nullptr;
    }
    const TypeMap<Value>* get( const FuncType* ty ) const {
        return as_non_const(this)->get( ty );
    }

    TypeMap<Value>* get( const Type* ty ) {
        if ( ! ty ) return nullptr;

        auto tid = typeof(ty);
        if ( tid == typeof<ConcType>() ) return get( as<ConcType>(ty) );
        else if ( tid == typeof<NamedType>() ) return get( as<NamedType>(ty) );
        else if ( tid == typeof<TupleType>() ) return get( as<TupleType>(ty) );
        else if ( tid == typeof<VoidType>() ) return get( as<VoidType>(ty) );
        else if ( tid == typeof<PolyType>() ) return get( as<PolyType>(ty) );
        else if ( tid == typeof<FuncType>() ) return get( as<FuncType>(ty) );
        
        unreachable("invalid type kind");
        return nullptr;
    }
    const TypeMap<Value>* get( const Type* ty ) const {
        return as_non_const(this)->get( ty );
    }

    TypeMap<Value>* get( const List<Type>& tys ) {
        // scan down trie as far as it matches
        TypeMap<Value>* tm = this;
        for ( unsigned i = 0; i < tys.size(); ++i ) {
            tm = tm->get( tys[i] );
            if ( ! tm ) return nullptr;
        }
        return tm;
    }
    const TypeMap<Value>* get( const List<Type>& tys ) const {
        return as_non_const(this)->get( tys );
    }

    /// Gets the value for the given type key
    TypeMap<Value>* get( const TypeKey& key ) {
        auto it = nodes.find( key );
        return it == nodes.end() ? nullptr : it->second.get();
    }
    const TypeMap<Value>* get( const TypeKey& key ) const {
        return as_non_const(this)->get( key );
    }

    /// Gets the value for the key for type T constructed from the given arguments
    template<typename T, typename... Args>
    TypeMap<Value>* get_as( Args&&... args ) {
        return get( TypeKey::make<T>( forward<Args>(args)... ) );
    }
    template<typename T, typename... Args>
    const TypeMap<Value>* get_as( Args&&... args ) const {
        return as_non_const(this)->template get_as<T>( forward<Args>(args)... );
    }

    /// Iterator for all non-exact matches of a type
    class PolyIter : public std::iterator<
            std::forward_iterator_tag,
            TypeMap<Value>,
            long,
            const TypeMap<Value>*,
            const TypeMap<Value>&> {
        using Base = TypeMap<Value>;                         ///< Underlying type map
        using ListIter = typename PolyList::const_iterator;  ///< Poly-list iterator

        /// Backtracking information for this iteration
        struct Backtrack {
            ListIter next;     ///< Iterator pointing to next polymorphic type
            const Base* base;  ///< Map containing poly-list

            Backtrack() = default;
            Backtrack( const ListIter& i, const Base* b ) : next(i), base(b) {}
            Backtrack( ListIter&& i, const Base* b ) : next(move(i)), base(b) {}

            bool operator== ( const Backtrack& o ) const {
                return next == o.next && base == o.base;
            }
            bool operator!= ( const Backtrack& o ) const { return !(*this == o); }
        };
        /// Stack of backtrack information
        using BacktrackList = std::vector<Backtrack>;

        const Base* m;         ///< Current type map
        BacktrackList prefix;  ///< Backtracking stack for search
        const AtomList atoms;  ///< Type keys to search

        /// Steps further into the type; atoms should not be fully consumed
        inline bool push_prefix() {
            ListIter it = m->polys.begin();
            if ( const Base* nm = m->get( atoms[prefix.size()] ) ) {
                // look at concrete expansions of the current type
                prefix.emplace_back( move(it), m );
                m = nm;
                return true;
            } else if ( it != m->polys.end() ) {
                // look at polymorphic expansions of the current type
                const Base* nm = it->second;
                prefix.emplace_back( move(++it), m );
                m = nm;
                return true;
            } else return false;  // no elements for this type
        }

        /// Gets the next valid prefix state (or empty)
        inline void next_valid() {
            restart:
            // back out to parent type if no sibling
            while ( ! prefix.empty() && prefix.back().next == prefix.back().base->polys.end() ) {
                prefix.pop_back();
            }
            // check for end of iteration
            if ( prefix.empty() ) {
                m = nullptr;
                return;
            }
            // go to sibling (guaranteed present here)
            m = prefix.back().next->second;
            ++prefix.back().next;
            // fill tail back to end of prefix
            while ( prefix.size() < atoms.size() ) {
                if ( push_prefix() ) continue;
                // backtrack again on dead-end prefix
                prefix.pop_back();
                goto restart;
            }
        }

    public:
        PolyIter() : m(nullptr), prefix(), atoms() {}

        PolyIter(const Base* b, const Type* t) : m(b), prefix(), atoms(type_atoms(t)) {
            // scan initial prefix until it matches the target type
            while ( prefix.size() < atoms.size() ) {
                if ( push_prefix() ) continue; 
                
                // quit on dead-end base
                if ( prefix.empty() ) {
                    m = nullptr;
                    return;
                }

                // backtrack again on dead-end prefix
                prefix.pop_back();
                next_valid();
                return;
            }
        }

        const Base& operator* () { return *m; }
        const Base* operator-> () { return m; }

        PolyIter& operator++ () { next_valid(); return *this; }
        PolyIter operator++ (int) { auto tmp = *this; next_valid(); return tmp; }
        
        /// true if not an end iterator
        explicit operator bool() const { return m != nullptr; }

        bool operator== (const PolyIter& o) const { return m == o.m; }
        bool operator!= (const PolyIter& o) const { return m != o.m; }

        /// true if currrent state only uses concrete types
        bool is_concrete() const {
            for ( const auto& b : prefix ) {
                if ( b.next != b.base->polys.begin() ) return false;
            }
            return true;
        }
    };

    /// Gets type maps for a given type, where one or more of the subtypes 
    /// have been switched with polymorphic bindings.
    range<PolyIter> get_poly_maps(const Type* ty) const {
        return { PolyIter{ this, ty }, PolyIter{} };
    }

    class MatchIter : public std::iterator<
            std::forward_iterator_tag,
            TypeMap<Value>,
            long,
            const TypeMap<Value>*,
            const TypeMap<Value>&> {
        using Base = TypeMap<Value>;                         ///< Underlying type map
        using NodeIter = typename ConcMap::const_iterator;   ///< Conc-map iterator
        using ListIter = typename PolyList::const_iterator;  ///< Poly-list iterator

        /// Backtracking information for this iteration
        struct Backtrack {
            NodeIter nextConc;        ///< Next map key [end() for none such]
            ListIter nextPoly;        ///< Next polymorphic type in this map [end() for none such]
            const Base* base;         ///< Map containing current nodes
            unsigned char atomsLeft;  ///< map atoms left in current search atom

            Backtrack() = default;
            Backtrack( ListIter&& it, const Base* b )
                : nextConc(b->nodes.end()), nextPoly(move(it)), base(b), atomsLeft(0) {}
            Backtrack( NodeIter&& it, const Base* b, unsigned char n ) 
                : nextConc(move(it)), nextPoly(b->polys.end()), base(b), atomsLeft(n) {}
            
            bool operator== ( const Backtrack& o ) const {
                return base == o.base && nextConc == o.nextConc && nextPoly == o.nextPoly;
            }
            bool operator!= ( const Backtrack& o ) const { return !(*this == o); }

            /// true if there is a next sibling for this backtrack
            bool has_next() const {
                return nextConc != base->nodes.end() || nextPoly != base->polys.end();
            }

            /// get next base; requires a next sibling exists
            const Base* get_next() const {
                if ( nextConc != base->nodes.end() ) return nextConc->second.get();
                else return nextPoly->second;
            }

            /// go to next sibling
            void operator++ () {
                if ( nextConc != base->nodes.end() ) ++nextConc;
                else ++nextPoly;
            }
        };
        using BacktrackList = std::vector<Backtrack>;

        const Base* m;          ///< Current type map
        BacktrackList prefix;   ///< Backtracking stack for search
        const AtomList atoms;   ///< Type keys to search
        AtomList::size_type i;  ///< Current atom

        /// map atoms left to match search atom at i
        unsigned char atoms_left() const { return prefix.empty() ? 0 : prefix.back().atomsLeft; }

        /// Pops one atom from the backtracking stack
        void pop_prefix() {
            // pop backtracking node
            prefix.pop_back();
            // decrement search atom if at a boundary now
            if ( ! atoms_left() ) --i;
        }

        /// Steps further into the type; search keys should not be fully consumed
        bool push_prefix() {
            unsigned char atomsLeft = atoms_left();

            if ( atomsLeft == 0 ) {
                // find node matching current atom
                ++i;
                ListIter it = m->polys.begin();
                if ( atoms[i-1].mode() == KeyMode::Poly ) {
                    // handle as incomplete atom
                    atomsLeft = 1;
                } else if ( const Base* nm = m->get( atoms[i-1] ) ) {
                    // look at concrete expansions of the current atom
                    prefix.emplace_back( move(it), m );
                    m = nm;
                    return true;
                } else if ( it != m->polys.end() ) {
                    // look at polymorphic expansions of current atom
                    const Base* nm = it->second;
                    prefix.emplace_back( move(++it), m );
                    m = nm;
                    return true;
                } else return false;  // no elements matching this type
            }
            
            // take any node to fill out atom (if exists)
            if ( m->nodes.empty() ) return false;
            NodeIter it = m->nodes.begin();
            const Base* nm = it->second.get();
            unsigned char na = atomsLeft + it->first.children() - 1;
            prefix.emplace_back( move(++it), m, na );
            m = nm;
            return true;
        }

        /// Gets the next valid prefix state (or empty)
        void next_valid() {
            restart:
            // back out to parent type if no sibling
            while ( ! prefix.empty() && ! prefix.back().has_next() ) pop_prefix();
            // check for end of iteration
            if ( prefix.empty() ) {
                m = nullptr;
                return;
            }
            // go to sibling (guaranteed present)
            m = prefix.back().get_next();
            ++prefix.back();
            // fill tail back to end of prefix
            while ( i < atoms.size() ) {
                if ( push_prefix() ) continue;
                // backtrack again on dead-end prefix
                pop_prefix();
                goto restart;
            }
        }
        
    public:
        MatchIter() : m(nullptr), prefix(), atoms(), i(0) {}

        MatchIter(const Base* b, const Type* t) : m(b), prefix(), atoms(type_atoms(t)), i(0) {
            // scan initial prefix until it matches the target type
            while ( i < atoms.size() ) {
                if ( push_prefix() ) continue;

                // quit on dead-end base
                if ( prefix.empty() ) {
                    m = nullptr;
                    return;
                }

                // backtrack again on dead-end prefix
                pop_prefix();
                next_valid();
                return;
            }
        }

        const Base& operator* () { return *m; }
        const Base* operator-> () { return m; }

        MatchIter& operator++ () { next_valid(); return *this; }
        MatchIter operator++ (int) { auto tmp = *this; next_valid(); return tmp; }

        /// true if not an end iterator
        explicit operator bool() const { return m != nullptr; }

        bool operator== (const MatchIter& o) const { return m == o.m; }
        bool operator!= (const MatchIter& o) const { return m != o.m; }
    };

    /// Gets all type maps matching a given type, possibly with polymorphic replacement.
    range<MatchIter> get_matching(const Type* ty) const {
        return { MatchIter{ this, ty }, MatchIter{} };
    }

    /// Stores v in the leaf pointer
    template<typename V>
    void set( const Type* ty, V&& v ) {
        leaf = make_unique<value_type>( ty, forward<V>(v) );
    }

    template<typename V>
    std::pair<iterator, bool> insert( std::nullptr_t, V&& ) { return { end(), false }; }

    template<typename V>
    std::pair<iterator, bool> insert( const VoidType* ty, V&& v ) {
        bool r = false;
        
        if ( ! leaf ) {
            set( ty, forward<V>(v) );
            r = true;
        }

        return { iterator{ Iter{ this, BacktrackList{} } }, r };
    }

    template<typename V>
    std::pair<iterator, bool> insert( const ConcType* ty, V&& v ) {
        bool r = false;

        TypeKey k = TypeKey{ ty };
        auto it = nodes.find( k );
        if ( it == nodes.end() ) {
            it = nodes.emplace_hint( it, k, make_unique<TypeMap<Value>>() );
        }
        if ( ! it->second->leaf ) {
            it->second->set( ty, forward<V>(v) );
            r = true;
        }

        return { 
            iterator{ Iter{ it->second.get(), BacktrackList{ Backtrack{ it, this } } } }, 
            r };
    }

private:
    /// Fills in backtrack list for a single node (should not be VoidType or TupleType)
    TypeMap<Value>* fill_step( const Type* ty, BacktrackList& rpre ) {
        TypeKey k{ ty };
        auto it = nodes.find( k );
        if ( it == nodes.end() ) {
            // add new node as needed
            if ( k.mode() == KeyMode::Poly ) {
                it = nodes.emplace_hint( it, copy(k), make_unique<TypeMap<Value>>() );
                polys.emplace_back( *k.key_for<PolyType>(), it->second.get() );
            } else {
                it = nodes.emplace_hint( it, move(k), make_unique<TypeMap<Value>>() );
            }
        }

        rpre.emplace_back( it, this );
        return it->second.get();
    }

    /// Finds or fills type maps down a list of types, filling in backtrack list as it goes
    TypeMap<Value>* fill_to( const List<Type>& ts, BacktrackList& rpre ) {
        TypeMap<Value>* tm = this;
        for ( unsigned i = 0; i < ts.size(); ++i ) {
            tm = tm->fill_step( ts[i], rpre );
        }
        return tm;
    }

public:
    template<typename V>
    std::pair<iterator, bool> insert( const NamedType* ty, V&& v ) {
        BacktrackList rpre;
        bool r = false;

        TypeKey k = TypeKey{ ty };
        auto it = nodes.find( k );
        if ( it == nodes.end() ) {
            it = nodes.emplace_hint( it, k, make_unique<TypeMap<Value>>() );
        }

        rpre.emplace_back( it, this );
        TypeMap<Value>* tm = it->second->fill_to( ty->params(), rpre );

        if ( ! tm->leaf ) {
            tm->set( ty, forward<V>(v) );
            r = true;
        }

        return { iterator{ Iter{ tm, move(rpre) } }, r };
    }

    template<typename V>
    std::pair<iterator, bool> insert( const PolyType* ty, V&& v ) {
        bool r = false;

        TypeKey k = TypeKey{ ty };
        auto it = nodes.find( k );
        if ( it == nodes.end() ) {
            it = nodes.emplace_hint( it, copy(k), make_unique<TypeMap<Value>>() );
            polys.emplace_back( *k.key_for<PolyType>(), it->second.get() );
        }
        if ( ! it->second->leaf ) {
            it->second->set( ty, forward<V>(v) );
            r = true;
        }

        return { 
            iterator{ Iter{ it->second.get(), BacktrackList{ Backtrack{ it, this } } } }, 
            r };
    }

    template<typename V>
    std::pair<iterator, bool> insert( const TupleType* ty, V&& v ) {
        BacktrackList rpre;
        bool r = false;

        TypeMap<Value>* tm = fill_to( ty->types(), rpre );

        if ( ! tm->leaf ) {
            tm->set( ty, forward<V>(v) );
            r = true;
        }

        return { iterator{ Iter{ tm, move(rpre) } }, r };
    }

    template<typename V>
    std::pair<iterator, bool> insert( const FuncType* ty, V&& v ) {
        BacktrackList rpre;
        bool r = false;

        // function header
        TypeKey k{ ty };
        auto it = nodes.find( k );
        if ( it == nodes.end() ) {
            it = nodes.emplace_hint( it, k, make_unique<TypeMap<Value>>() );
        }

        // parameter list
        rpre.emplace_back( it, this );
        TypeMap<Value>* tm = it->second->fill_to( ty->params(), rpre );

        // return types
        auto rid = typeof( ty->returns() );
        if ( rid == typeof<VoidType>() ) {
            /* do nothing */
        } else if ( rid == typeof<TupleType>() ) {
            tm = tm->fill_to( as<TupleType>(ty->returns())->types(), rpre );
        } else {
            tm = tm->fill_step( ty->returns(), rpre );
        }
        
        if ( ! tm->leaf ) {
            tm->set( ty, forward<V>(v) );
            r = true;
        }

        return { iterator{ Iter{ tm, move(rpre) } }, r };
    }

    template<typename V>
    std::pair<iterator, bool> insert( const Type* ty, V&& v ) {
        if ( ! ty ) return { end(), false };

        auto tid = typeof(ty);
        if ( tid == typeof<ConcType>() ) return insert( as<ConcType>(ty), forward<V>(v) );
        else if ( tid == typeof<NamedType>() ) return insert( as<NamedType>(ty), forward<V>(v) );
        else if ( tid == typeof<TupleType>() ) return insert( as<TupleType>(ty), forward<V>(v) );
        else if ( tid == typeof<VoidType>() ) return insert( as<VoidType>(ty), forward<V>(v) );
        else if ( tid == typeof<PolyType>() ) return insert( as<PolyType>(ty), forward<V>(v) );
        else if ( tid == typeof<FuncType>() ) return insert( as<FuncType>(ty), forward<V>(v) );
        
        unreachable("invalid type kind");
        return { end(), false };
    }
    
    std::pair<iterator, bool> insert( value_type&& v ) {
        return insert( v.first, move(v.second) );
    }
    std::pair<iterator, bool> insert( const value_type& v ) {
        return insert( v.first, v.second );
    }
    iterator insert( const const_iterator& hint, value_type&& v ) {
        return insert( v.first, move(v.second) ).first;
    }
    iterator insert( const const_iterator& hint, const value_type& v ) {
        return insert( v.first, v.second ).first;
    }

    size_type count( std::nullptr_t ) const { return 0; }

    size_type count( const VoidType* ) const {
        return leaf ? 1 : 0;
    }

    size_type count( const ConcType* ty ) const {
        auto it = nodes.find( TypeKey{ ty } );
        if ( it == nodes.end() ) return 0;
        return it->second->leaf ? 1 : 0;
    }

    size_type count( const NamedType* ty ) const {
        auto it = nodes.find( TypeKey{ ty } );
        if ( it == nodes.end() ) return 0;
        return it->second->count( ty->params() );
    }

    size_type count( const PolyType* ty ) const {
        auto it = nodes.find( TypeKey{ ty } );
        if ( it == nodes.end() ) return 0;
        return it->second->leaf ? 1 : 0;
    }

    size_type count( const TupleType* ty ) const {
        return count( ty->types() );
    }

    size_type count( const FuncType* ty ) const {
        auto it = nodes.find( TypeKey{ ty } );
        if ( it == nodes.end() ) return 0;

        // scan down trie for params
        TypeMap<Value>* tm = it->second.get();
        const auto& ps = ty->params();
        for ( unsigned i = 0; i < ps.size(); ++i ) {
            auto jt = tm->nodes.find( TypeKey{ ps[i] } );
            if ( jt == tm->nodes.end() ) return 0;

            tm = jt->second.get();
        }

        // handle return type
        return tm->count( ty->returns() );
    }

    size_type count( const Type* ty ) const {
        if ( ! ty ) return 0;

        auto tid = typeof(ty);
        if ( tid == typeof<ConcType>() ) return count( as<ConcType>(ty) );
        else if ( tid == typeof<NamedType>() ) return count( as<NamedType>(ty) );
        else if ( tid == typeof<TupleType>() ) return count( as<TupleType>(ty) );
        else if ( tid == typeof<VoidType>() ) return count( as<VoidType>(ty) );
        else if ( tid == typeof<PolyType>() ) return count( as<PolyType>(ty) );
        else if ( tid == typeof<FuncType>() ) return count( as<FuncType>(ty) );
        
        unreachable("invalid type kind");
        return 0;
    }

    size_type count( const List<Type>& tys ) const {
        // scan down trie as far as it matches
        TypeMap<Value>* tm = this;
        for ( unsigned i = 0; i < tys.size(); ++i ) {
            auto it = tm->nodes.find( TypeKey{ tys[i] } );
            if ( it == tm->nodes.end() ) return 0;

            tm = it->second.get();
        }
        return tm->leaf ? 1 : 0;
    }

private:
    const TypeMap<Value>* look_step( const VoidType*, BacktrackList& ) const { return this; }

    const TypeMap<Value>* look_step( const ConcType* ty, BacktrackList& rpre ) const {
        auto it = as_non_const(nodes).find( TypeKey{ ty } );
        if ( it == as_non_const(nodes).end() ) return nullptr;
        
        rpre.emplace_back( it, as_non_const(this) );
        return it->second.get();
    }

    const TypeMap<Value>* look_step( const NamedType* ty, BacktrackList& rpre ) const {
        auto it = as_non_const(nodes).find( TypeKey{ ty } );
        if ( it == as_non_const(nodes).end() ) return nullptr;

        rpre.emplace_back( it, as_non_const(this) );
        return it->second->look_to( ty->params(), rpre );
    }

    const TypeMap<Value>* look_step( const PolyType* ty, BacktrackList& rpre ) const {
        auto it = as_non_const(nodes).find( TypeKey{ ty } );
        if ( it == as_non_const(nodes).end() ) return nullptr;

        rpre.emplace_back( it, as_non_const(this) );
        return it->second.get();
    }

    const TypeMap<Value>* look_step( const TupleType* ty, BacktrackList& rpre ) const {
        return look_to( ty->types(), rpre );
    }

    const TypeMap<Value>* look_step( const FuncType* ty, BacktrackList& rpre ) const {
        auto it = as_non_const(nodes).find( TypeKey{ ty } );
        if ( it == as_non_const(nodes).end() ) return nullptr;

        rpre.emplace_back( it, as_non_const(this) );

        const TypeMap<Value>* tm = it->second->look_to( ty->params(), rpre );
        if ( ! tm ) return nullptr;

        return tm->look_step( ty->returns(), rpre );
    }

    const TypeMap<Value>* look_step( const Type* ty, BacktrackList& rpre ) const {
        auto tid = typeof(ty);
        if ( tid == typeof<ConcType>() ) return look_step( as<ConcType>(ty), rpre );
        else if ( tid == typeof<NamedType>() ) return look_step( as<NamedType>(ty), rpre );
        else if ( tid == typeof<TupleType>() ) return look_step( as<TupleType>(ty), rpre );
        else if ( tid == typeof<VoidType>() ) return look_step( as<VoidType>(ty), rpre );
        else if ( tid == typeof<PolyType>() ) return look_step( as<PolyType>(ty), rpre );
        else if ( tid == typeof<FuncType>() ) return look_step( as<FuncType>(ty), rpre );
        
        unreachable("invalid type kind");
        return nullptr;
    }

    /// Locates the typemap that matches the list of types, filling in rpre as it traverses.
    /// Returns nullptr if no match
    const TypeMap<Value>* look_to( const List<Type>& ts, BacktrackList& rpre ) const {
        const TypeMap<Value>* tm = this;
        for ( unsigned i = 0; i < ts.size(); ++i ) {
            tm = tm->look_step( ts[i], rpre );
            if ( ! tm ) return nullptr;
        }
        return tm;
    }

    /// Wraps look interface for iterators
    template<typename Ty>
    Iter locate( const Ty* ty ) const {
        BacktrackList rpre;
        const TypeMap<Value>* tm = look_step( ty, rpre );
        return ( tm && tm->leaf ) ? Iter{ as_non_const(tm), move(rpre) } : Iter{};
    }

public:
    iterator find( std::nullptr_t ) { return end(); }
    const_iterator find( std::nullptr_t ) const { return end(); }

    iterator find( const VoidType* ty ) { return iterator{ locate(ty) }; }
    const_iterator find( const VoidType* ty ) const { return const_iterator{ locate(ty) }; }

    iterator find( const ConcType* ty ) { return iterator{ locate(ty) }; }
    const_iterator find( const ConcType* ty ) const { return const_iterator{ locate(ty) }; }

    iterator find( const NamedType* ty ) { return iterator{ locate(ty) }; }
    const_iterator find( const NamedType* ty ) const { return const_iterator{ locate(ty) }; }

    iterator find( const PolyType* ty ) { return iterator{ locate(ty) }; }
    const_iterator find( const PolyType* ty ) const { return const_iterator{ locate(ty) }; }

    iterator find( const TupleType* ty ) { return iterator{ locate(ty) }; }
    const_iterator find( const TupleType* ty ) const { return const_iterator{ locate(ty) }; }

    iterator find( const FuncType* ty ) { return iterator{ locate(ty) }; }
    const_iterator find( const FuncType* ty ) const { return const_iterator{ locate(ty) }; }

    iterator find( const Type* ty ) {
        if ( ! ty ) return end();

        auto tid = typeof(ty);
        if ( tid == typeof<ConcType>() ) return iterator{ locate( as<ConcType>(ty) ) };
        else if ( tid == typeof<NamedType>() ) return iterator{ locate( as<NamedType>(ty) ) };
        else if ( tid == typeof<TupleType>() ) return iterator{ locate( as<TupleType>(ty) ) };
        else if ( tid == typeof<VoidType>() ) return iterator{ locate( as<VoidType>(ty) ) };
        else if ( tid == typeof<PolyType>() ) return iterator{ locate( as<PolyType>(ty) ) };
        else if ( tid == typeof<FuncType>() ) return iterator{ locate( as<FuncType>(ty) ) };
        
        unreachable("invalid type kind");
        return end();
    }
    const_iterator find( const Type* ty ) const {
        if ( ! ty ) return end();

        auto tid = typeof(ty);
        if ( tid == typeof<ConcType>() ) return const_iterator{ locate( as<ConcType>(ty) ) };
        else if ( tid == typeof<NamedType>() ) return const_iterator{ locate( as<NamedType>(ty) ) };
        else if ( tid == typeof<TupleType>() ) return const_iterator{ locate( as<TupleType>(ty) ) };
        else if ( tid == typeof<VoidType>() ) return const_iterator{ locate( as<VoidType>(ty) ) };
        else if ( tid == typeof<PolyType>() ) return const_iterator{ locate( as<PolyType>(ty) ) };
        else if ( tid == typeof<FuncType>() ) return const_iterator{ locate( as<FuncType>(ty) ) };
        
        unreachable("invalid type kind");
        return end();
    }
};
