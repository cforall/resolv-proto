#pragma once

#include <functional>
#include <iterator>
#include <memory>
#include <set>
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
using Ref = const T*;

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

/// Clone an object into a fresh pointer having the same runtime type
template<typename T>
Ptr<T> clone( const Ptr<T>& p ) { return p->clone(); }

template<typename T>
Ptr<T> clone( const Shared<T>& p ) { return p->clone(); }

template<typename T>
Ptr<T> clone( Ref<T> p ) { return p->clone(); }

/// Clone all the elements in a collection of Ptr<T>
template<typename C>
C cloneAll( const C& c ) {
	C d;
	for ( const auto& e : c ) { d.insert( d.end(), clone(e) ); }
	return d;
}

/// Get a mutable copy of the value pointed to by a shared pointer
template<typename T>
Ptr<T> mut( Shared<T>&& p ) {
	if ( p.unique() ) {
		// deliberately leak ptr by swapping with shared pointer that won't release
		auto leaker = Shared<T>{ nullptr, [](const T*){} }; 
		p.swap( leaker );
		return Ptr<T>{ const_cast<T*>( leaker.get() ) }; 
	} else {
		Ptr<T> q = clone( p );
		p.reset();
		return q;
	}
}

/// Borrow an owned pointer
template<typename T>
inline Ref<T> ref( const Ptr<T>& p ) { return p.get(); }

/// Borrow a shared pointer
template<typename T>
inline Ref<T> ref( const Shared<T>& p ) { return p.get(); }

/// Borrow an lvalue
template<typename T>
inline Ref<T> ref( T& p ) { return &p; }

/// Borrow a pointer or value as a different type, returning nullptr if not 
/// runtime compatible
template<typename T, typename F>
inline Ref<T> ref_as( Ref<F> p ) {
	return dynamic_cast< Ref<T> >( p );
} 

template<typename T, typename P>
inline Ref<T> ref_as( const P& p ) { return ref_as( ref(p) ); }

/// Take ownership of a value
using std::move;

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
struct ByRef {
	typedef Ref<T> Element;
	typedef ByValueHash<T, Ref> Hash;
	typedef ByValueEquals<T, Ref> Equals;
	typedef ByValueCompare<T, Ref> Compare;
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

/// Gets canonical Ref for an object from the set;
/// If the object does not exist in the set, is inserted
template<typename T>
inline Ref<T> get_ref(Set<T>& s, Ptr<T>&& x) {
	return ref( *s.insert( move(x) ).first );
}

/// Gets canonical Ref for an object from the set;
/// If the object does not exist in the set, is inserted
template<typename T>
inline Ref<T> get_ref(SortedSet<T>& s, Ptr<T>&& x) {
	return ref( *s.insert( move(x) ).first );
}

/// Gets canonical Ref for an object from the set;
/// If the object does not exist in the set, returns nullptr
template<typename T>
inline Ref<T> find_ref(const Set<T>& s, Ptr<T>&& x) {
	auto it = s.find(x);
	return it == s.end() ? nullptr : ref( *it );
}

/// Gets canonical Ref for an object from the set;
/// If the object does not exist in the set, returns nullptr
template<typename T>
inline Ref<T> find_ref(const SortedSet<T>& s, Ptr<T>&& x) {
	auto it = s.find(x);
	return it == s.end() ? nullptr : ref( *it );
}
