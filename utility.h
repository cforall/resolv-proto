#pragma once

#include <functional>
#include <memory>
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

/// Make an owned pointer; T must define a T::Base type
template<typename T, typename... Args>
inline Ptr<typename T::Base> make( Args&&... args ) {
	typedef typename T::Base Base;
	return Ptr<Base>{ 
		dynamic_cast<Base*>( new T( std::forward<Args>(args)... ) ) };
}

using std::make_shared;

/// Borrow an owned pointer
template<typename T>
inline Ref<T> ref( const Ptr<T>& p ) { return p.get(); }

/// Borrow an lvalue
template<typename T>
inline Ref<T> ref( T& p ) { return &p; } 

/// Owning list of pointers
template<typename T>
using List = std::vector< Ptr<T> >;

/// List of borrowed pointers
template<typename T>
using RefList = std::vector< Ref<T> >;

/// List of shared pointers
template<typename T>
using SharedList = std::vector< Shared<T> >;

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

/// Owning set of pointers
template<typename T>
using Set = std::unordered_set< Ptr<T>, ByValueHash<T>, ByValueEquals<T> >;

/// Gets canonical Ref for an object from the set;
/// If the object does not exist in the set, is inserted
template<typename T>
inline Ref<T> get_ref(Set<T>& s, Ptr<T>&& x) {
	return ref( *s.emplace( std::move(x) ).first );
}

/// Gets canonical Ref for an object from the set;
/// If the object does not exist in the set, returns nullptr
template<typename T>
inline Ref<T> find_ref(const Set<T>& s, Ptr<T>&& x) {
	auto it = s.find(x);
	return it == s.end() ? nullptr : ref( *it );
}
