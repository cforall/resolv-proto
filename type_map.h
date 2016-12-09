#pragma once

#include <cassert>
#include <cstddef>
#include <functional>
#include <iterator>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "data.h"
#include "type.h"

/// A map from types to some value; lookup is done by structural 
/// decomposition on types.
template<typename Value>
class TypeMap {
public:
    typedef const Type* key_type;
    typedef Value mapped_type;
    typedef std::pair<const Type* const, Value&> value_type;
    typedef std::pair<const Type* const, const Value&> const_value_type;
    typedef std::size_t size_type;
    typedef std::ptrdiff_t difference_type;
    typedef ByValueCompare<Type> key_compare;
    typedef value_type reference;
    typedef const_value_type const_reference;
    typedef std::unique_ptr< value_type > pointer;
    typedef std::unique_ptr< const_value_type > const_pointer;

private:
    /// Variant key type for map
    class Key {
        /// Key type for ConcType
        struct ConcKey {
            int id;

            ConcKey( int id ) : id(id) {}
        };

        /// Key type for PolyType
        struct PolyKey {
            std::string name;
            const TypeBinding* src;

            PolyKey( const std::string& name, const TypeBinding* src ) : name(name), src(src) {}
        };

        enum { Conc, Poly } key_type;  ///< Variant discriminator
        union {
            ConcKey conc;  ///< ConcType variant
            PolyKey poly;  ///< PolyType variant
        };

        /// Initializes key from ConcType
        void init( const ConcType* ct ) {
            key_type = Conc;
            new(&conc) ConcKey{ ct->id() };
        }

        /// Initializes key from PolyType
        void init( const PolyType* pt ) {
            key_type = Poly;
            new(&poly) PolyKey{ pt->name(), pt->src() };
        }

        /// Initializes variant fields (but not key type) by copy
        void init( const Key& o ) {
            switch ( o.key_type ) {
            case Conc:
                new(&conc) ConcKey{ o.conc };
                break;
            case Poly:
                new(&poly) PolyKey{ o.poly };
                break;
            }
        }

        /// Assigns variant fields (but not key type) by move
        void init( Key&& o ) {
            switch ( o.key_type ) {
            case Conc:
                new(&conc) ConcKey{ move(o.conc) };
                break;
            case Poly:
                new(&poly) PolyKey{ move(o.poly) };
                break;
            }
        }

        /// Clears variant fields
        void reset() {
            switch ( key_type ) {
            case Conc:
                conc.~ConcKey();
                break;
            case Poly:
                poly.~PolyKey();
                break;
            }
        }

    public:
        /// Constructs key from type; type should be either ConcType or PolyType
        Key( const Type* t ) {
            auto tid = typeof(t);
            if ( tid == typeof<ConcType>() ) {
                init( as<ConcType>(t) );
            } else if ( tid == typeof<PolyType>() ) {
                init( as<PolyType>(t) );
            } else assert(false && "Invalid key type");
        }

        Key( const ConcType* t ) { init( t ); }

        Key( const PolyType* t ) { init( t ); }

        // deliberately no nullptr_t implmentation; no meaningful value

        Key( const Key& o ) : key_type( o.key_type ) { init( o ); }

        Key( Key&& o ) : key_type( o.key_type ) { init( move(o) ); }

        Key& operator= ( const Key& o ) {
            if ( this == &o ) return *this;
            if ( key_type == o.key_type ) {
                switch( key_type ) {
                case Conc:
                    conc = o.conc;
                    break;
                case Poly:
                    poly = o.poly;
                    break;
                }
            } else {
                reset();
                key_type = o.key_type;
                init( o );
            }

            return *this;
        }

        Key& operator= ( Key&& o ) {
            if ( this == &o ) return *this;
            if ( key_type == o.key_type ) {
                switch( key_type ) {
                case Conc:
                    conc = move(o.conc);
                    break;
                case Poly:
                    poly = move(o.poly);
                    break;
                }
            } else {
                reset();
                key_type = o.key_type;
                init( move(o) );
            }

            return *this;
        }

        ~Key() { reset(); }

        /// Retrieves the underlying key
        const Type* get() const {
            switch ( key_type ) {
            case Conc:
                return new ConcType{ conc.id };
            case Poly:
                return new PolyType{ poly.name, poly.src };
            default: assert(false && "Invalid key type"); return nullptr;
            }
        } 

        bool operator== ( const Key& o ) const {
            if ( key_type != o.key_type ) return false;
            switch ( key_type ) {
            case Conc:
                return conc.id == o.conc.id;
            case Poly:
                return poly.src == o.poly.src && poly.name == o.poly.name;
            default: assert(false && "Invalid key type"); return false;
            }
        }

        bool operator< ( const Key& o ) const {
            if ( key_type != o.key_type ) return (int)key_type < (int)o.key_type;
            switch ( key_type ) {
            case Conc:
                return conc.id < o.conc.id;
            case Poly:
                return poly.name < o.poly.name 
                    || poly.name == o.poly.name 
                       && std::less<TypeBinding*>{}( poly.src, o.poly.src );
            default: assert(false && "Invalid key type"); return false;
            }
        }

        std::size_t hash() const {
            std::size_t h;
            switch ( key_type ) {
            case Conc:
                h = std::hash<int>{}( conc.id );
                break;
            case Poly:
                h = (std::hash<std::string>{}( poly.name ) << 1) 
                    ^ std::hash<const TypeBinding*>()( poly.src );
                break;
            default: assert(false && "Invalid key type");
            }
            return (h << 1) | (std::size_t)key_type;
        }
    };

    struct KeyHash {
        std::size_t operator() ( const Key& k ) const { return k.hash(); }
    };

    /// Underlying map type
    template<typename V>
    using KeyMap =
    #ifdef RP_SORTED
         std::map<Key, V>;
    #else
        std::unordered_map<Key, V, KeyHash>;
    #endif

    /// Optional leaf value for map
    typedef std::unique_ptr<Value> Leaf;
    /// Map type for concrete types
    typedef KeyMap< std::unique_ptr<TypeMap<Value>> > ConcMap;

    // TODO build union representation
    Leaf leaf;      ///< Storage for leaf values
    ConcMap nodes;  ///< Storage for concrete types (and tuples starting with concrete types)

    struct Iter {
        typedef TypeMap<Value> Base;                  ///< Underlying map
        typedef typename ConcMap::iterator ConcIter;  ///< Concrete map iterator

        struct Backtrack {
            ConcIter it;  ///< Iterator for concrete map
            Base* base;   ///< TypeMap containing concrete map

            Backtrack() = default;
            Backtrack( ConcIter& i, Base* b ) : it( i ), base( b ) {}

            bool operator== ( const Backtrack& o ) const { return it == o.it && base == o.base; }
            bool operator!= ( const Backtrack& that ) const { return !( *this == that ); }
        };
        typedef std::vector<Backtrack> BacktrackList;

        Base* base;            ///< Current concrete map; null if none such
        BacktrackList prefix;  ///< Prefix of tuple types

        inline void push_prefix() {
            auto next = base->nodes.begin();
            prefix.emplace_back( next, base );
            base = next->second.get();
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

        /// Get the type represented by the key of this iterator [iterator should not be end]
        const Type* key() const {
            switch ( prefix.size() ) {
            case 0:
                return new VoidType();
            case 1:
                return prefix.front().it->first.get();
            default: {
                List<Type> types( prefix.size() );
                for (unsigned i = 0; i < prefix.size(); ++i) {
                    types[i] = prefix[i].it->first.get();
                }
                return new TupleType( move(types) );
            }
            }
        }

        /// Get the value stored in the map at this iterator
        Value& get() { return *base->leaf; }
        const Value& get() const { return *base->leaf; }

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

    };
    typedef typename Iter::Backtrack Backtrack;
    typedef typename Iter::BacktrackList BacktrackList;

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
        iterator( nullptr_t p ) : i( p ) {}
        iterator( TypeMap<Value>* p ) : i( p ) {}
    public:
        iterator() = default;

        const Type* key() { return i.key(); }
        Value& get() { return i.get(); }

        reference operator* () { return { i.key(), i.get() }; }
        pointer operator-> () { return std::make_unique< value_type >( i.key(), i.get() ); }

        iterator& operator++ () { ++i; return *this; }
        iterator& operator++ (int) { iterator tmp = *this; ++i; return tmp; }

        bool operator== ( const iterator& o ) const { return i == o.i; }
        bool operator!= ( const iterator& o ) const { return i != o.i; }
    };

    class const_iterator : public std::iterator<
            std::input_iterator_tag,
            const_value_type,
            long,
            const_pointer,
            const_reference> {
    friend TypeMap;
        Iter i;
    
        const_iterator( Iter&& i ) : i( move(i) ) {}
        const_iterator( nullptr_t ) : i( nullptr ) {}
        const_iterator( const TypeMap<Value>* p ) : i( as_non_const(p) ) {}
    public:
        const_iterator() = default;
        const_iterator( const iterator& o ) : i(o.i) {}
        const_iterator( iterator&& o ) : i( move(o.i) ) {}

        const Type* key() { return i.key(); }
        const Value& get() { return i.get(); }

        const_reference operator* () { return { i.key(), i.get() }; }
        const_pointer operator-> () { return std::make_unique< const_value_type >( i.key(), i.get() ); }

        const_iterator& operator++ () { ++i; return *this; }
        const_iterator& operator++ (int) { const_iterator tmp = *this; ++i; return tmp; }

        bool operator== ( const const_iterator& o ) const { return i == o.i; }
        bool operator!= ( const const_iterator& o ) const { return !(*this == o); }
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
    }

    /// Gets the leaf value, or nullptr for none such. 
    Value* get() { return leaf.get(); }
    const Value* get() const { return leaf.get(); }

    /// Gets the TypeMap for types starting with this type, or nullptr for none such.
    TypeMap<Value>* get( nullptr_t ) { return nullptr; }
    const TypeMap<Value>* get( nullptr_t ) const { return nullptr; }

    TypeMap<Value>* get( const VoidType* ) { return this; }
    const TypeMap<Value>* get( VoidType* ) const { return this; }

    TypeMap<Value>* get( const ConcType* ty ) {
        auto it = nodes.find( Key{ ty } );
        return it == nodes.end() ? nullptr : it->second.get();
    }
    const TypeMap<Value>* get( const ConcType* ty ) const {
        return as_non_const(this)->get( ty );
    }

    TypeMap<Value>* get( const PolyType* ty ) {
        auto it = nodes.find( Key{ ty } );
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
    const TypeMap<Value>* get( TupleType* ty ) const {
        return as_non_const(this)->get( ty );
    }

    TypeMap<Value>* get( const Type* ty ) {
        if ( ! ty ) return nullptr;

        auto tid = typeof(ty);
        if ( tid == typeof<ConcType>() ) return get( as<ConcType>(ty) );
        else if ( tid == typeof<TupleType>() ) return get( as<TupleType>(ty) );
        else if ( tid == typeof<VoidType>() ) return get( as<VoidType>(ty) );
        else if ( tid == typeof<PolyType>() ) return get( as<PolyType>(ty) );
        
        assert(false);
        return nullptr;
    }
    const TypeMap<Value>* get( Type* ty ) const {
        return as_non_const(this)->get( ty );
    }

    /// Stores v in the leaf pointer
    template<typename V>
    void set( V&& v ) { leaf = std::make_unique<Value>( forward<V>(v) ); }

    template<typename V>
    std::pair<iterator, bool> insert( nullptr_t, V&& ) { return { end(), false }; }

    template<typename V>
    std::pair<iterator, bool> insert( const VoidType* ty, V&& v ) {
        bool r = false;
        
        if ( ! leaf ) {
            set( forward<V>(v) );
            r = true;
        }

        return { iterator{ Iter{ this, BacktrackList{} } }, r };
    }

    template<typename V>
    std::pair<iterator, bool> insert( const ConcType* ty, V&& v ) {
        bool r = false;

        Key k = Key{ ty };
        auto it = nodes.find( k );
        if ( it == nodes.end() ) {
            it = nodes.emplace_hint( it, k, std::make_unique<TypeMap<Value>>() );
        }
        if ( ! it->second->leaf ) {
            it->second->set( forward<V>(v) );
            r = true;
        }

        return { 
            iterator{ Iter{ it->second.get(), BacktrackList{ Backtrack{ it, this } } } }, 
            r };
    }

    template<typename V>
    std::pair<iterator, bool> insert( const PolyType* ty, V&& v ) {
        bool r = false;

        Key k = Key{ ty };
        auto it = nodes.find( k );
        if ( it == nodes.end() ) {
            it = nodes.emplace_hint( it, move(k), std::make_unique<TypeMap<Value>>() );
        }
        if ( ! it->second->leaf ) {
            it->second->set( forward<V>(v) );
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
            Key k{ ty->types()[i] };
            
            auto it = tm->nodes.find( k );
            if ( it == tm->nodes.end() ) {
                // add new nodes as needed
                it = tm->nodes.emplace_hint( it, move(k), std::make_unique<TypeMap<Value>>() );
            }

            rpre.emplace_back( it, tm );
            tm = it->second.get();
        }

        if ( ! tm->leaf ) {
            tm->set( forward<V>(v) );
            r = true;
        }

        return { iterator{ Iter{ tm, move(rpre) } }, r };
    }

    template<typename V>
    std::pair<iterator, bool> insert( const Type* ty, V&& v ) {
        if ( ! ty ) return { end(), false };

        auto tid = typeof(ty);
        if ( tid == typeof<ConcType>() ) return insert( as<ConcType>(ty), forward<V>(v) );
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

    size_type count( nullptr_t ) const { return 0; }

    size_type count( const VoidType* ) const {
        return leaf ? 1 : 0;
    }

    size_type count( const ConcType* ty ) const {
        auto it = nodes.find( Key{ ty } );
        if ( it == nodes.end() ) return 0;
        return it.second->leaf ? 1 : 0;
    }

    size_type count( const PolyType* ty ) const {
        auto it = nodes.find( Key{ ty } );
        if ( it == nodes.end() ) return 0;
        return it.second->leaf ? 1 : 0;
    }

    size_type count( const TupleType* ty ) const {
        // scan down trie as far as it matches
        TypeMap<Value>* tm = this;
        for ( unsigned i = 0; i < ty->size(); ++i ) {
            auto it = tm->nodes.find( Key{ ty->types()[i] } );
            if ( it == tm->nodes.end() ) return 0;

            tm = it->second.get();
        }
        return tm->leaf ? 1 : 0;
    }

    size_type count( const Type* ty ) const {
        if ( ! ty ) return 0;

        auto tid = typeof(ty);
        if ( tid == typeof<ConcType>() ) return count( as<ConcType>(ty) );
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
        auto it = as_non_const(nodes).find( Key{ ty } );
        if ( it == as_non_const(nodes).end() ) return Iter{};

        return it->second->leaf ?
            Iter{ it->second.get(), BacktrackList{ Backtrack{ it, as_non_const(this) } } } :
            Iter{};
    }

    Iter locate( const PolyType* ty ) const {
        auto it = as_non_const(nodes).find( Key{ ty } );
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
            auto it = tm->nodes.find( Key{ ty->types()[i] } );
            if ( it == tm->nodes.end() ) return Iter{};

            rpre.emplace_back( it, tm );
            tm = it->second.get();
        }

        return tm->leaf ? Iter{ tm, move(rpre) } : Iter{};
    }

public:
    iterator find( nullptr_t ) { return end(); }
    const_iterator find( nullptr_t ) const { return end(); }

    iterator find( const VoidType* ty ) { return iterator{ locate(ty) }; }
    const_iterator find( const VoidType* ty ) const { return const_iterator{ locate(ty) }; }

    iterator find( const ConcType* ty ) { return iterator{ locate(ty) }; }
    const_iterator find( const ConcType* ty ) const { return const_iterator{ locate(ty) }; }

    iterator find( const PolyType* ty ) { return iterator{ locate(ty) }; }
    const_iterator find( const PolyType* ty ) const { return const_iterator{ locate(ty) }; }

    iterator find( const TupleType* ty ) { return iterator{ locate(ty) }; }
    const_iterator find( const TupleType* ty ) const { return const_iterator{ locate(ty) }; }

    iterator find( const Type* ty ) {
        if ( ! ty ) return end();

        auto tid = typeof(ty);
        if ( tid == typeof<ConcType>() ) return iterator{ locate( as<ConcType>(ty) ) };
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
        else if ( tid == typeof<TupleType>() ) return const_iterator{ locate( as<TupleType>(ty) ) };
        else if ( tid == typeof<VoidType>() ) return const_iterator{ locate( as<VoidType>(ty) ) };
        else if ( tid == typeof<PolyType>() ) return const_iterator{ locate( as<PolyType>(ty) ) };
        
        assert(false);
        return end();
    }
};
