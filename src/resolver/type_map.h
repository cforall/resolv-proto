#pragma once

#include <cassert>
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
#include "data/cast.h"
#include "data/collections.h"
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
    
    NamedKey( const std::string& name ) : name(name) {}
    NamedKey( const NamedType* nt ) : name( nt->name() ) {}

    const Type* value() const { return new NamedType{ name }; }

    bool operator== (const NamedKey& o) const { return name == o.name; }
    bool operator< (const NamedKey& o) const { return name < o.name; }

    std::size_t hash() const { return std::hash<std::string>{}(name); }
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

/// Variants of a key
enum KeyMode { Conc, Named, Poly };

/// Associates key type and enum with node type
template<typename Ty>
struct key_info {};

template<>
struct key_info<ConcType> {
    typedef ConcKey type;
    static constexpr KeyMode key = Conc;
};

template<>
struct key_info<NamedType> {
    typedef NamedKey type;
    static constexpr KeyMode key = Named;
};

template<>
struct key_info<PolyType> {
    typedef PolyKey type;
    static constexpr KeyMode key = Poly;
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
        case Conc:
            init<ConcType>( o.get<ConcKey>() );
            break;
        case Named:
            init<NamedType>( o.get<NamedKey>() );
            break;
        case Poly:
            init<PolyType>( o.get<PolyKey>() );
            break;
        }
    }

    void init_from( TypeKey&& o ) {
        switch ( o.key_type ) {
        case Conc:
            init<ConcType>( o.take<ConcKey>() );
            break;
        case Named:
            init<NamedType>( o.take<NamedKey>() );
            break;
        case Poly:
            init<PolyType>( o.take<PolyKey>() );
            break;
        }
    }

    /// Clears variant fields
    void reset() {
        switch ( key_type ) {
        case Conc:
            get<ConcKey>().~ConcKey();
            break;
        case Named:
            get<NamedKey>().~NamedKey();
            break;
        case Poly:
            get<PolyKey>().~PolyKey();
            break;
        }
    }

    /// Default constructor only for internal use
    TypeKey() {}

public:
    /// Constructs key from type; type should be ConcType, NamedType or PolyType
    TypeKey( const Type* t ) {
        auto tid = typeof(t);
        if ( tid == typeof<ConcType>() ) {
            init<ConcType>( as<ConcType>(t) );
        } else if ( tid == typeof<NamedType>() ) {
            init<NamedType>( as<NamedType>(t) );
        } else if ( tid == typeof<PolyType>() ) {
            init<PolyType>( as<PolyType>(t) );
        } else assert(false && "Invalid key type");
    }

    TypeKey( const ConcType* t ) { init<ConcType>( t ); }

    TypeKey( const NamedType* t ) { init<NamedType>( t ); }

    TypeKey( const PolyType* t ) { init<PolyType>( t ); }

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
            case Conc:
                get<ConcKey>() = o.get<ConcKey>();
                break;
            case Named:
                get<NamedKey>() = o.get<NamedKey>();
                break;
            case Poly:
                get<PolyKey>() = o.get<PolyKey>();
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
            case Conc:
                get<ConcKey>() = o.take<ConcKey>();
                break;
            case Named:
                get<NamedKey>() = o.take<NamedKey>();
                break;
            case Poly:
                get<PolyKey>() = o.take<PolyKey>();
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
        case Conc:
            return get<ConcKey>().value();
        case Named:
            return get<NamedKey>().value();
        case Poly:
            return get<PolyKey>().value();
        default: assert(!"Invalid key type"); return nullptr;
        }
    } 

    bool operator== ( const TypeKey& o ) const {
        if ( key_type != o.key_type ) return false;
        switch ( key_type ) {
        case Conc:
            return get<ConcKey>() == o.get<ConcKey>();
        case Named:
            return get<NamedKey>() == o.get<NamedKey>();
        case Poly:
            return get<PolyKey>() == o.get<PolyKey>();
        default: assert(!"Invalid key type"); return false;
        }
    }

    bool operator< ( const TypeKey& o ) const {
        if ( key_type != o.key_type ) return (int)key_type < (int)o.key_type;
        switch ( key_type ) {
        case Conc:
            return get<ConcKey>() < o.get<ConcKey>();
        case Named:
            return get<NamedKey>() < o.get<NamedKey>();
        case Poly:
            return get<PolyKey>() < o.get<PolyKey>();
        default: assert(!"Invalid key type"); return false;
        }
    }

    std::size_t hash() const {
        std::size_t h;
        switch ( key_type ) {
        case Conc:
            h = get<ConcKey>().hash();
            break;
        case Named:
            h = get<NamedKey>().hash();
            break;
        case Poly:
            h = get<PolyKey>().hash();
            break;
        default: assert(!"Invalid key type");
        }
        return (h << 2) | (std::size_t)key_type;
    }

    /// Gets the key mode
    KeyMode mode() const { return key_type; }

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
        return it == nodes.end() ? nullptr : it->second.get();
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
        // scan down trie as far as it matches
        TypeMap<Value>* tm = this;
        for ( unsigned i = 0; i < ty->size(); ++i ) {
            tm = tm->get( ty->types()[i] );
            if ( ! tm ) return nullptr;
        }
        return tm;
    }
    const TypeMap<Value>* get( const TupleType* ty ) const {
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
        
        assert(false);
        return nullptr;
    }
    const TypeMap<Value>* get( const Type* ty ) const {
        return as_non_const(this)->get( ty );
    }

    /// Gets the value for the key for type T constructed from the given arguments
    template<typename T, typename... Args>
    TypeMap<Value>* get_as( Args&&... args ) {
        auto it = nodes.find( TypeKey::make<T>( forward<Args>(args)... ) );
        return it == nodes.end() ? nullptr : it->second.get();
    }
    template<typename T, typename... Args>
    const TypeMap<Value>* get_as( Args&&... args ) const {
        return as_non_const(this)->get( forward<Args>(args)... );
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
        const Type* ty;        ///< Type to iterate

        /// Gets the type atom at index j; j should be less than ty->size()
        inline const Type* type_at(unsigned j) const {
            switch ( ty->size() ) {
            // return atom for void or concrete type
            case 0: case 1: return ty;
            // return tuple element otherwise
            default: return as<TupleType>(ty)->types()[j];
            }
        }

        /// Steps further into the type; ty should not be fully consumed
        inline bool push_prefix() {
            ListIter it = m->polys.begin();
            if ( const Base* nm = m->get( type_at( prefix.size() ) ) ) {
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
            unsigned n = ty->size();
            while ( prefix.size() < n ) {
                if ( ! push_prefix() ) {
                    // backtrack again on dead-end prefix
                    prefix.pop_back();
                    next_valid();
                    return;
                }
            }
        }

    public:
        PolyIter() : m(nullptr), prefix(), ty(nullptr) {}

        PolyIter(const Base* b, const Type* t) : m(b), prefix(), ty(t) {
            // scan initial prefix until it matches the target type
            unsigned n = ty->size();
            while ( prefix.size() < n ) {
                if ( ! push_prefix() ) {
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

    template<typename V>
    std::pair<iterator, bool> insert( const NamedType* ty, V&& v ) {
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

        // scan down trie as far as it matches
        TypeMap<Value>* tm = this;
        for ( unsigned i = 0; i < ty->size(); ++i ) {
            TypeKey k{ ty->types()[i] };
            
            auto it = tm->nodes.find( k );
            if ( it == tm->nodes.end() ) {
                // add new nodes as needed
                if ( k.mode() == Poly ) {
                    it = tm->nodes.emplace_hint( it, copy(k), make_unique<TypeMap<Value>>() );
                    tm->polys.emplace_back( 
                        *k.key_for<PolyType>(), it->second.get() );
                } else {
                    it = tm->nodes.emplace_hint( it, move(k), make_unique<TypeMap<Value>>() );
                }
            }

            rpre.emplace_back( it, tm );
            tm = it->second.get();
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
        
        assert(false);
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
        return it.second->leaf ? 1 : 0;
    }

    size_type count( const NamedType* ty ) const {
        auto it = nodes.find( TypeKey{ ty } );
        if ( it == nodes.end() ) return 0;
        return it.second->leaf ? 1 : 0;
    }

    size_type count( const PolyType* ty ) const {
        auto it = nodes.find( TypeKey{ ty } );
        if ( it == nodes.end() ) return 0;
        return it.second->leaf ? 1 : 0;
    }

    size_type count( const TupleType* ty ) const {
        // scan down trie as far as it matches
        TypeMap<Value>* tm = this;
        for ( unsigned i = 0; i < ty->size(); ++i ) {
            auto it = tm->nodes.find( TypeKey{ ty->types()[i] } );
            if ( it == tm->nodes.end() ) return 0;

            tm = it->second.get();
        }
        return tm->leaf ? 1 : 0;
    }

    size_type count( const Type* ty ) const {
        if ( ! ty ) return 0;

        auto tid = typeof(ty);
        if ( tid == typeof<ConcType>() ) return count( as<ConcType>(ty) );
        else if ( tid == typeof<NamedType>() ) return count( as<NamedType>(ty) );
        else if ( tid == typeof<TupleType>() ) return count( as<TupleType>(ty) );
        else if ( tid == typeof<VoidType>() ) return count( as<VoidType>(ty) );
        else if ( tid == typeof<PolyType>() ) return count( as<PolyType>(ty) );
        
        assert(false);
        return 0;
    }

private:
    Iter locate( const VoidType* ) const {
        return leaf ? Iter{ as_non_const(this), BacktrackList{} } : Iter{};
    }

    Iter locate( const ConcType* ty ) const {
        auto it = as_non_const(nodes).find( TypeKey{ ty } );
        if ( it == as_non_const(nodes).end() ) return Iter{};

        return it->second->leaf ?
            Iter{ it->second.get(), BacktrackList{ Backtrack{ it, as_non_const(this) } } } :
            Iter{};
    }

    Iter locate( const NamedType* ty ) const {
        auto it = as_non_const(nodes).find( TypeKey{ ty } );
        if ( it == as_non_const(nodes).end() ) return Iter{};

        return it->second->leaf ?
            Iter{ it->second.get(), BacktrackList{ Backtrack{ it, as_non_const(this) } } } :
            Iter{};
    }

    Iter locate( const PolyType* ty ) const {
        auto it = as_non_const(nodes).find( TypeKey{ ty } );
        if ( it == as_non_const(nodes).end() ) return Iter{};

        return it->second->leaf ?
            Iter{ it->second.get(), BacktrackList{ Backtrack{ it, as_non_const(this) } } } :
            Iter{};
    }

    Iter locate( const TupleType* ty ) const {
        BacktrackList rpre;

        // scan down trie as far as it matches
        TypeMap<Value>* tm = as_non_const(this);
        for ( unsigned i = 0; i < ty->size(); ++i ) {
            auto it = tm->nodes.find( TypeKey{ ty->types()[i] } );
            if ( it == tm->nodes.end() ) return Iter{};

            rpre.emplace_back( it, tm );
            tm = it->second.get();
        }

        return tm->leaf ? Iter{ tm, move(rpre) } : Iter{};
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

    iterator find( const Type* ty ) {
        if ( ! ty ) return end();

        auto tid = typeof(ty);
        if ( tid == typeof<ConcType>() ) return iterator{ locate( as<ConcType>(ty) ) };
        else if ( tid == typeof<NamedType>() ) return iterator{ locate( as<NamedType>(ty) ) };
        else if ( tid == typeof<TupleType>() ) return iterator{ locate( as<TupleType>(ty) ) };
        else if ( tid == typeof<VoidType>() ) return iterator{ locate( as<VoidType>(ty) ) };
        else if ( tid == typeof<PolyType>() ) return iterator{ locate( as<PolyType>(ty) ) };
        
        assert(false);
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
        
        assert(false);
        return end();
    }
};
