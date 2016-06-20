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

/// Flat iterator type; iterates over contained collection
template<typename Iter>
class Flat : public std::iterator< 
		typename std::iterator_traits<std::iterator_traits<Iter>::value_type>::iterator_category, 
        typename std::iterator_traits<std::iterator_traits<Iter>::value_type>::value_type,
		typename std::iterator_traits<std::iterator_traits<Iter>::value_type>::difference_type,
		typename std::iterator_traits<std::iterator_traits<Iter>::value_type>::pointer,
		typename std::iterator_traits<std::iterator_traits<Iter>::value_type>::reference > {
public:
	typedef typename std::iterator_traits<Iter>::value_type Inner;
	typedef typename std::iterator_traits<Inner>::reference reference;
	typedef typename std::iterator_traits<Inner>::pointer pointer;
private:
	friend Flat<Iter> flat_from_valid( const Iter& );
	friend Flat<Iter> flat_from_end( const Iter& );
	friend bool operator== ( const Flat<Iter>&, const Iter& );
	
	Iter i;   ///< Iterator for outer collection
	Inner j;  ///< Iterator for inner collection
	
	/// Checks if the iterator is in a valid state
	bool is_valid() const { return j != i->end(); }
	
	/// Increments on invalid
	Flat<Iter>& next_valid() {
		if ( ! is_valid() ) { ++(*this); }
		return *this;
	}
public:
	Flat( const Flat<Iter>& ) = default;
	Flat<Iter>& operator= ( const Flat<Iter>& ) = default;
	
	reference operator* () { return *j; }
	pointer operator-> () { return j.operator->(); }
	
	Flat<Iter>& operator++ () {
		if ( is_valid() ) {
			++j;
		} else {
			++i;
			j = i->begin();
		}
		return next_valid();
	}
	Flat<Iter>& operator++ (int) {
		Flat<Iter>& tmp = *this; ++(*this); return tmp;
	}
	
	bool operator== ( const Flat<Iter>& o ) const { return i == o.i && j == o.j; }
	bool operator!= ( const Flat<Iter>& o ) const { return !(*this == o); }
};

/// Constructs a new flat iterator from a valid iterator
template<typename Iter>
Flat<Iter> flat_from_valid( const Iter& i ) {
	Flat<Iter> f = (Flat<Iter>){ i, i->begin() };
	return f.next_valid();
}

/// Constructs a new empty flat iterator from an end iterator
template<typename Iter>
Flat<Iter> flat_from_end( const Iter& i ) { return (Flat<Iter>){ i }; }

template<typename Iter>
bool operator== ( const Flat<Iter>& a, const Iter& b ) { return a.i == b; }
template<typename Iter>
bool operator== ( const Iter& a, const Flat<Iter>& b ) { return b == a; }
template<typename Iter>
bool operator!= ( const Flat<Iter>& a, const Iter& b ) { return !(a == b); }
template<typename Iter>
bool operator!= ( const Iter& a, const Flat<Iter>& b ) { return !(b == a); }

/// Range type; abstracts iterable container
template<typename Iter>
class Range {
	const Iter begin_;  // Range begin iterator
	const Iter end_;    // Range end iterator
public:
	typedef Iter iterator;
	typedef Iter const_iterator;
	
	Range() = default;
	Range(const Iter& begin_, const Iter& end_) : begin_(begin_), end_(end_) {}
	template<typename Container>
	Range(const Container& c) : begin_(c.begin()), end_(c.end()) {}
	
	Range(const Range&) = default;
	Range(Range&&) = default;
	Range& operator= (const Range&) = default;
	Range& operator= (Range&&) = default;
	
	Iter begin() const { return begin_; }
	Iter end() const { return end_; }
};

/// Hasher for underlying type of pointers
template<typename T, template<typename> typename P>
struct ByValueHash {
	std::size_t operator() (const P<T>& p) const { return std::hash<T>()(*p); }
};

/// Equality checker for underlying type of pointers
template<typename T, template<typename> typename P>
struct ByValueEquals {
	bool operator() (const P<T>& p, const P<T>& q) const { return *p == *q; }
};

/// Comparator for underlying type of pointers
template<typename T, template<typename> typename P>
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
template<typename T, template<typename> typename Storage = ByPtr>
using List = std::vector<typename Storage<T>::Element>;

/// Set type
template<typename T, template<typename> typename Storage = ByPtr>
using Set = std::unordered_set<typename Storage<T>::Element,
                               typename Storage<T>::Hash,
							   typename Storage<T>::Equals>;

/// Sorted set type
template<typename T, template<typename> typename Storage = ByPtr>
using SortedSet = std::set<typename Storage<T>::Element,
                           typename Storage<T>::Compare>;

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

/// Gets canonical Ref for an object from the set;
/// If the object does not exist in the set, returns nullptr
template<typename T>
inline Ref<T> find_ref(const SortedSet<T>& s, Ptr<T>&& x) {
	auto it = s.find(x);
	return it == s.end() ? nullptr : ref( *it );
}
