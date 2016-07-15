#pragma once

#include <functional>
#include <iterator>
#include <memory>
#include <set>
#include <type_traits>
#include <unordered_set>
#include <utility>
#include <vector>

/// Owning pointer to T; must be constructable from T* 
template<typename T>
using Ptr = std::unique_ptr<T>;

/// Ref-counted pointer to T; must be constructable from T*
template<typename T>
using Shared = std::shared_ptr<const T>;

/// Borrowed pointer to T
template<typename T>
using Brw = const T*;

/// Make an owned pointer to T of type T::Base
template<typename T, typename... Args>
inline Ptr<typename T::Base> make( Args&&... args ) {
	typedef typename T::Base Base;
	return Ptr<Base>{ 
		dynamic_cast<Base*>( new T( std::forward<Args>(args)... ) ) };
}

/// Make an owned pointer to T
template<typename T, typename... Args>
inline Ptr<T> make_ptr( Args&&... args ) {
	return Ptr<T>{ new T( std::forward<Args>(args)... ) };
}

/// Make a shared pointer to T
using std::make_shared;

/// Checks if T is Ptr<U>, Shared<U>, or Brw<U> for some U,
/// for template metaprogramming
template<typename T>
struct is_pointer_type : std::false_type {};

template<typename U>
struct is_pointer_type< Ptr<U> > : std::true_type {};
template<typename U>
struct is_pointer_type< Shared<U> > : std::true_type {};
template<typename U>
struct is_pointer_type< Brw<U> > : std::true_type {};

/// Borrow an owned pointer
template<typename T>
inline Brw<T> brw( const Ptr<T>& p ) { return p.get(); }

/// Borrow a shared pointer
template<typename T>
inline Brw<T> brw( const Shared<T>& p ) { return p.get(); }

/// Borrow an lvalue
template<typename T,
         typename = typename std::enable_if<!is_pointer_type<T>::value, T>::type>
inline Brw<T> brw( T& p ) {
	return &p;
}

/// Borrow a pointer or value as a different type, returning nullptr if not 
/// runtime compatible
template<typename T, typename F>
inline Brw<T> brw_as( Brw<F> p ) {
	return dynamic_cast< Brw<T> >( p );
}

template<typename T, typename F>
inline Brw<T> brw_as( const Ptr<F>& p ) {
	return dynamic_cast< Brw<T> >( brw(p) );
}

template<typename T, typename F>
inline Brw<T> brw_as( const Shared<F>& p ) {
	return dynamic_cast< Brw<T> >( brw(p) );
}

/// Share a pointer as a different type, returning nullptr if not runtime compatible
template <typename T, typename U>
inline Shared<T> shared_as( const Shared<U>& p ) {
	return std::dynamic_pointer_cast<const T>( p );
}

/// Take ownership of a value
using std::move;

/// Shallow copy of an object
template<typename T>
inline T copy( const T& x ) { return x; }

/// Clone an object into a fresh pointer having the same runtime type
template<typename T>
inline Ptr<T> clone( const Ptr<T>& p ) { return p->clone(); }

template<typename T>
inline Ptr<T> clone( const Shared<T>& p ) { return p->clone(); }

template<typename T>
inline Ptr<T> clone( Brw<T> p ) { return p->clone(); }

/// Clone all the elements in a collection of Ptr<T>
template<typename C>
inline C cloneAll( const C& c ) {
	C d;
	for ( const auto& e : c ) { d.insert( d.end(), clone(e) ); }
	return d;
}

/// Get a mutable copy of the value pointed to by a shared pointer
template<typename T>
inline Ptr<T> mut( Shared<T>&& p ) {
	if ( p.unique() ) {
		// deliberately leak ptr by swapping with shared pointer that won't release
		auto leaker = Shared<T>{ nullptr, [](const T*){} }; 
		p.swap( leaker );
		return Ptr<T>{ const_cast<T*>( leaker.get() ) }; 
	} else {
		// replace this pointer to p with a fresh clone 
		Ptr<T> q = clone( p );
		p.reset();
		return q;
	}
}

/// Hasher for underlying type of pointers
template<typename T, template<typename> class P>
struct ByValueHash {
	std::size_t operator() (const P<T>& p) const { return std::hash<T>()(*p); }
};

/// Equality checker for underlying type of pointers
template<typename T, template<typename> class P>
struct ByValueEquals {
	bool operator() (const P<T>& p, const P<T>& q) const { return *p == *q; }
};

/// Comparator for underlying type of pointers
template<typename T, template<typename> class P>
struct ByValueCompare {
	bool operator() (const P<T>& p, const P<T>& q) const { return *p < *q; }
};

/// Traits class for owned pointer
template<typename T>
struct ByPtr {
	typedef Ptr<T> Element;
	typedef ByValueHash<T, Ptr> Hash;
	typedef ByValueEquals<T, Ptr> Equals;
	typedef ByValueCompare<T, Ptr> Compare;
};

/// Traits class for borrowed pointer
template<typename T>
struct ByBrw {
	typedef Brw<T> Element;
	typedef ByValueHash<T, Brw> Hash;
	typedef ByValueEquals<T, Brw> Equals;
	typedef ByValueCompare<T, Brw> Compare;
};

/// Traits class for owned pointer
template<typename T>
struct ByShared {
	typedef Shared<T> Element;
	typedef ByValueHash<T, Shared> Hash;
	typedef ByValueEquals<T, Shared> Equals;
	typedef ByValueCompare<T, Shared> Compare;
};

/// Traits class for raw value
template<typename T>
struct Raw {
	typedef T Element;
	typedef std::hash<T> Hash;
	typedef std::equal_to<T> Equals;
	typedef std::less<T> Compare;
};

/// List type
template<typename T, template<typename> class Storage = ByPtr>
using List = std::vector<typename Storage<T>::Element>;

/// Set type
template<typename T, template<typename> class Storage = ByPtr>
using Set = std::unordered_set<typename Storage<T>::Element,
                               typename Storage<T>::Hash,
							   typename Storage<T>::Equals>;

/// Sorted set type
template<typename T, template<typename> class Storage = ByPtr>
using SortedSet = std::set<typename Storage<T>::Element,
                           typename Storage<T>::Compare>;

/// Gets canonical reference for an object from the set;
/// If the object does not exist in the set, is inserted
template<typename T>
inline Brw<T> get_canon(Set<T>& s, Ptr<T>&& x) {
	return brw( *s.insert( move(x) ).first );
}

/// Gets canonical reference for an object from the set;
/// If the object does not exist in the set, is inserted
template<typename T>
inline Brw<T> get_canon(SortedSet<T>& s, Ptr<T>&& x) {
	return brw( *s.insert( move(x) ).first );
}

/// Gets canonical reference for an object from the set;
/// If the object does not exist in the set, returns nullptr
template<typename T>
inline Brw<T> find_canon(const Set<T>& s, Ptr<T>&& x) {
	auto it = s.find(x);
	return it == s.end() ? nullptr : brw( *it );
}

/// Gets canonical reference for an object from the set;
/// If the object does not exist in the set, returns nullptr
template<typename T>
inline Brw<T> find_canon(const SortedSet<T>& s, Ptr<T>&& x) {
	auto it = s.find(x);
	return it == s.end() ? nullptr : brw( *it );
}
