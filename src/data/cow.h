#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <type_traits>
#include <utility>

/// Copy-on-write pointer to a T; T must be default and copy-constructable and have 
/// alignment >= 2. Maintains a flag for whether or not the underlying pointer is owned  
/// (and thus can be written without a copy). Owned pointers will be deleted on destruction.
/// Copy-constructing makes an unowned copy, move-constructing makes an owned copy if the 
/// original is owned, assignment will not drop ownership when assigning an equal pointer, 
/// but otherwise behaves the same as construction with respect to ownership. 
template<typename T>
class cow_ptr {
    static_assert( alignof(T) >= 2, "cow_ptr requires alignment >= 2" );

    /// Gets a copy of this pointer with the owned flag set false
    static constexpr T* as_unowned( T* p ) {
        return reinterpret_cast<T*>( reinterpret_cast<uintptr_t>(p) | UINTMAX_C(1) );
    }

    T* p;  ///< Underlying pointer; low-order bit 0 if owned, 1 otherwise

    /// get pointer
    T* ptr() const {
        return reinterpret_cast<T*>( reinterpret_cast<uintptr_t>(p) & ~UINTMAX_C(1) );
    }
public:
    typedef typename std::add_lvalue_reference<T>::type reference;
    typedef typename std::add_lvalue_reference<const T>::type const_reference;

    /// get owned state
    bool owned() const { return !( reinterpret_cast<uintptr_t>(p) & UINTMAX_C(1) ); }

    /// Resets the stored pointer to an owned copy
    void reset( T* q ) {
        if ( owned() ) {
            if ( p == q ) return;
            else delete p;
        }
        p = q;
    }

    /// Resets the stored pointer to an unowned copy
    void reset( const T* q ) {
        if ( owned() ) {
            if ( p == q ) return;
            else delete p;
        }
        p = as_unowned(q);
    }

    void reset( std::nullptr_t ) {
        if ( owned() ) delete p;
        p = nullptr;
    }

    void reset() { reset(nullptr); }

    cow_ptr( T* p = nullptr ) : p(p) {}
    cow_ptr( const T* p ) : p( as_unowned(p) ) {}
    cow_ptr( std::nullptr_t ) : p(nullptr) {}

    cow_ptr( const cow_ptr<T>& o ) : p( as_unowned(o.p) ) {}
    cow_ptr( cow_ptr<T>&& o ) : p( o.p ) { o.p = nullptr; }
    cow_ptr<T>& operator= ( const cow_ptr<T>& o ) {
        if ( owned() ) {
            if ( p == o.ptr() ) return *this;  // no ownership leak
            else delete p;
        }
        p = as_unowned( o.p );
        return *this;
    }
    cow_ptr<T>& operator= ( cow_ptr<T>&& o ) {
        if ( owned() ) {
            if ( p == o.ptr() ) return *this;  // no ownership leak; assume only one owner
            else delete p; 
        }
        p = o.p;
        o.p = nullptr;  // necessary in case of ownership transfer
        return *this;
    }
    ~cow_ptr() { if ( owned() ) delete p; }

    void swap( cow_ptr<T>& o ) {
        uintptr_t tmp = reinterpret_cast<uintptr_t>(p);
        reinterpret_cast<uintptr_t&>(p) = reinterpret_cast<uintptr_t>(o.p);
        reinterpret_cast<uintptr_t&>(o.p) = tmp;
    }

    /// true iff the wrapped pointer is not null
    operator bool () const { return ptr() != nullptr; }

    /// get wrapped pointer
    const T* get() const { return ptr(); }
    const_reference operator*() const { return *ptr(); }
    const T* operator->() const { return ptr(); }

    /// get writable copy of wrapped object; will clone underlying object if not owned
    reference mut() {
        if ( ! owned() ) { p = new T( *ptr() ); }
        return *p;
    }
};

template<typename T, typename... Args>
cow_ptr<T> make_cow( Args&&... args ) {
    return cow_ptr<T>( new T( std::forward<Args>(args)... ) );
}

template<typename T>
bool operator== (const cow_ptr<T>& a, const cow_ptr<T>& b) { return a.get() == b.get(); }
template<typename T>
bool operator== (const cow_ptr<T>& a, const T* b) { return a.get() == b; }
template<typename T>
bool operator== (const T* a, const cow_ptr<T>& b) { return a == b.get(); }

template<typename T>
bool operator!= (const cow_ptr<T>& a, const cow_ptr<T>& b) { return !(a == b); }
template<typename T>
bool operator!= (const cow_ptr<T>& a, const T* b) { return !(a == b); }
template<typename T>
bool operator!= (const T* a, const cow_ptr<T>& b) { return !(a == b); }

template<typename T>
bool operator< (const cow_ptr<T>& a, const cow_ptr<T>& b) {
    return std::less<const T*>{}(a.get(), b.get());
}
template<typename T>
bool operator< (const cow_ptr<T>& a, const T* b) {
    return std::less<const T*>{}(a.get(), b);
}
template<typename T>
bool operator< (const T* a, const cow_ptr<T>& b) {
    return std::less<const T*>{}(a, b.get());
}

template<typename T>
bool operator> (const cow_ptr<T>& a, const cow_ptr<T>& b) { return b < a; }
template<typename T>
bool operator> (const cow_ptr<T>& a, const T* b) { return b < a; }
template<typename T>
bool operator> (const T* a, const cow_ptr<T>& b) { return b < a; }

template<typename T>
bool operator<= (const cow_ptr<T>& a, const cow_ptr<T>& b) { return !(a > b); }
template<typename T>
bool operator<= (const cow_ptr<T>& a, const T* b) { return !(a > b); }
template<typename T>
bool operator<= (const T* a, const cow_ptr<T>& b) { return !(a > b); }

template<typename T>
bool operator>= (const cow_ptr<T>& a, const cow_ptr<T>& b) { return !(a < b); }
template<typename T>
bool operator>= (const cow_ptr<T>& a, const T* b) { return !(a < b); }
template<typename T>
bool operator>= (const T* a, const cow_ptr<T>& b) { return !(a < b); }

namespace std {
    template<typename T>
    void swap( cow_ptr<T>& a, cow_ptr<T>& b ) { a.swap(b); }

    template<typename T>
    struct hash< cow_ptr<T> > {
        size_t operator() ( const cow_ptr<T>& p ) {
            return hash<const T*>{}( p.get() );
        } 
    };
}
