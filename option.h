#pragma once

#include <cassert>
#include <functional>
#include <type_traits>
#include <utility>

using std::move;

/// A class that may or may not hold a value of type T
template<typename T>
class option {
    typename std::aligned_storage<sizeof(T), alignof(T)>::type storage;
    bool filled;

    /// gets the underlying value (filled must be true)
    T& get() { return reinterpret_cast<T&>(storage); }
    const T& get() const { return reinterpret_cast<const T&>(storage); }
    
    /// copy-initializes from a value
    void init( const T& x ) {
        new(&storage) T(x);
        filled = true;
    }

    /// move-initializes from a value
    void init( T&& x ) {
        new(&storage) T( move(x) );
        filled = true;
    }

    /// destroys the underlying value (must be filled), sets filled to false
    void destroy() {
        get().~T();
        filled = false;
    }

public:

    option() : filled(false) {}
    option( const T& x ) { init( x ); }
    option( T&& x ) { init( move(x) ); }
    option( const option<T>& o ) : filled(o.filled) { if ( filled ) init( o.get() ); }
    option( option<T>&& o ) : filled(o.filled) { if ( filled ) init( move(o.get()) ); }
    option& operator= ( const T& x ) {
        if ( filled ) { get() = x; } else { init( x ); }
        return *this;
    }
    option& operator= ( T&& x ) {
        if ( filled ) { get() = move(x); } else { init( move(x) ); }
        return *this;
    }
    option& operator= ( const option<T>& o ) {
        if ( filled ) {
            if ( o.filled ) {
                get() = o.get();
            } else {
                destroy();
            }
        } else if ( o.filled ) {
            init( o.get() );
        }
        return *this;
    }
    option& operator= ( option<T>&& o ) {
        if ( filled ) {
            if ( o.filled ) {
                get() = move(o.get());
            } else {
                destroy();
            }
        } else if ( o.filled ) {
            init( move(o.get()) );
        }
        return *this;
    }
    ~option() { if ( filled ) get().~T(); }

    void swap( option<T>& o ) {
        if ( filled ) {
            if ( o.filled ) {
                std::swap( get(), o.get() );
            } else {
                o.init( move(get()) );
                destroy();
            }
        } else if ( o.filled ) {
            init( move(o.get()) );
            o.destroy();
        }
    }

    /// Get contained value (unchecked)
    T* operator->() { return &get(); }
    const T* operator->() const { return &get(); }
    T& operator*() & { return get(); }
    const T& operator*() const& { return get(); }
    T&& operator*() && { return move(get()); }
    const T&& operator*() const&& { return move(get()); }

    /// Check if option is filled
    constexpr explicit operator bool() const { return filled; }
    constexpr bool has_value() const { return filled; }

    /// Get contained value (checked)
    T& value() & { assert(filled); return get(); }
    const T& value() const& { assert(filled); return get(); }
    T&& value() && { assert(filled); return move(get()); }
    const T&& value() const&& { assert(filled); return move(get()); }

    /// Get contained or default value
    template<typename U>
    T value_or( U&& y ) const& { return filled ? get() : static_cast<T>(std::forward<U>(y)); }
    template<typename U>
    T value_or( U&& y ) && { return filled ? move(get()) : static_cast<T>(std::forward<U>(y)); }

    /// Unset value if set
    void reset() { if ( filled ) destroy(); }

    /// Construct value in-place (destroys existing value if needed)
    template<class... Args>
    void emplace( Args&&... args ) {
        if ( filled ) { destroy(); }
        new(&storage) T{ std::forward<Args>(args)... };
        filled = true;
    }
};

template<typename T>
bool operator== ( const option<T>& a, const option<T>& b ) {
    return a ? (b && *a == *b) : !b;
}

template<typename T>
bool operator!= ( const option<T>& a, const option<T>& b ) { return !(a == b); }

template<typename T>
bool operator< ( const option<T>& a, const option<T>& b ) {
    return b && (!a || *a < *b); 
}

template<typename T>
bool operator<= ( const option<T>& a, const option<T>& b ) {
    return !a || (b && *a <= *b);
}

template<typename T>
bool operator> ( const option<T>& a, const option<T>& b ) { return b < a; }

template<typename T>
bool operator>= ( const option<T>& a, const option<T>& b ) { return b <= a; }

template<typename T>
bool operator== ( const option<T>& a, const T& x ) {
    return a && *a == x;
}

template<typename T>
bool operator!= ( const option<T>& a, const T& x ) { return !(a == x); }

template<typename T>
bool operator< ( const option<T>& a, const T& x ) {
    return !a || *a < x;
}

template<typename T>
bool operator<= ( const option<T>& a, const T& x ) {
    return !a || *a <= x;
}

template<typename T>
bool operator> ( const option<T>& a, const T& x ) {
    return a && *a > x;
}

template<typename T>
bool operator>= ( const option<T>& a, const T& x ) {
    return a && *a >= x;
}

template<typename T>
bool operator== ( const T& x, const option<T>& b ) { return b == x; }

template<typename T>
bool operator!= ( const T& x, const option<T>& b ) { return !(b == x); }

template<typename T>
bool operator< ( const T& x, const option<T>& b ) { return b > x; }

template<typename T>
bool operator<= ( const T& x, const option<T>& b ) { return b >= x; }

template<typename T>
bool operator> ( const T& x, const option<T>& b ) { return b < x; }

template<typename T>
bool operator>= ( const T& x, const option<T>& b ) { return b <= x; }

template<typename T, typename... Args>
option<T> make_option( Args&&... args ) {
    option<T> o;
    o.emplace( std::forward<Args>(args)... );
    return o;
}

namespace std {
    template<typename T>
    void swap( option<T>& a, option<T>& b ) { a.swap(b); }

    template<typename T>
    struct hash< option<T> > {
        size_t operator() ( const option<T>& a ) {
            return a ? hash<T>{}( *a ) : size_t(1);
        }
    };
}
