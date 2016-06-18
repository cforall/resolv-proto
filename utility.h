#pragma once

#include <functional>
#include <memory>
#include <set>
#include <unordered_set>
#include <utility>
#include <vector>

/// Owning pointer to T; must be constructable from T* 
template<typename T>
using Ptr = std::unique_ptr<T>;

/// Borrowed pointer to T
template<typename T>
using Ref = const T*;

/// Ref-counted pointer to T; must be constructable from T*
template<typename T>
using Shared = std::shared_ptr<T>;

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

using std::make_shared;

/// Borrow an owned pointer
template<typename T>
inline Ref<T> ref( const Ptr<T>& p ) { return p.get(); }

/// Borrow a shared pointer
template<typename T>
inline Ref<T> ref( const Shared<T>& p ) { return p.get(); }

/// Borrow an lvalue
template<typename T>
inline Ref<T> ref( T& p ) { return &p; }

/// Underlying list type
template<typename T>
using RawList = std::vector<T>;

/// Owning list of pointers
template<typename T>
using List = RawList< Ptr<T> >;

/// List of borrowed pointers
template<typename T>
using RefList = RawList< Ref<T> >;

/// List of shared pointers
template<typename T>
using SharedList = RawList< Shared<T> >;

/// Hasher for underlying type of pointers
template<typename T>
struct ByValueHash {
	std::size_t operator() (const Ptr<T>& p) const { return std::hash<T>()(*p); }
};

/// Equality checker for underlying type of pointers
template<typename T>
struct ByValueEquals {
	bool operator() (const Ptr<T>& p, const Ptr<T>& q) const { return *p == *q; }
};

/// Comparator for underlying type of pointers
template<typename T>
struct ByValueCompare {
	bool operator() (const Ptr<T>& p, const Ptr<T>& q) const { return *p < *q; }
};

/// Underlying set type
template<typename T, typename Hash = std::hash<T>, typename KeyEqual = std::equal_to<T> >
using RawSet = std::unordered_set< T, Hash, KeyEqual >;

/// Underlying sorted set type
template<typename T, typename KeyCompare = std::less<T> >
using RawSortedSet = std::set< T, KeyCompare >;

/// Common base for by-pointer sets
template<typename T, template<typename> typename P>
using ByPtrSet = RawSet< P<T>, ByValueHash<T>, ByValueEquals<T> >;

/// Common base for by-pointer sorted sets
template<typename T, template<typename> typename P>
using ByPtrSortedSet = RawSortedSet< P<T>, ByValueCompare<T> >;

/// Owning set of pointers
template<typename T>
using Set = ByPtrSet< T, Ptr >;

/// Owning sorted set of pointers
template<typename T>
using SortedSet = ByPtrSortedSet< T, Ptr >;

/// Gets canonical Ref for an object from the set;
/// If the object does not exist in the set, is inserted
template<typename T>
inline Ref<T> get_ref(Set<T>& s, Ptr<T>&& x) {
	return ref( *s.insert( std::move(x) ).first );
}

/// Gets canonical Ref for an object from the set;
/// If the object does not exist in the set, is inserted
template<typename T>
inline Ref<T> get_ref(SortedSet<T>& s, Ptr<T>&& x) {
	return ref( *s.insert( std::move(x) ).first );
}

/// Gets canonical Ref for an object from the set;
/// If the object does not exist in the set, returns nullptr
template<typename T>
inline Ref<T> find_ref(const Set<T>& s, Ptr<T>&& x) {
	auto it = s.find(x);
	return it == s.end() ? nullptr : ref( *it );
}

template<typename T>
inline Ref<T> find_ref(const SortedSet<T>& s, Ptr<T>&& x) {
	auto it = s.find(x);
	return it == s.end() ? nullptr : ref( *it );
}
