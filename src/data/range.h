#pragma once

// Copyright (c) 2015 University of Waterloo
//
// The contents of this file are covered under the licence agreement in 
// the file "LICENCE" distributed with this repository.

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <stdexcept>
#include <utility>

/// Represents a collection as a pair of random-access iterators.
/// Functions should be interpreted as in std::vector
template<typename Iter>
class range {
	Iter i;  ///< Begin iterator
	Iter e;  ///< End iterator

public:
	using value_type = typename std::iterator_traits<Iter>::value_type;
	using size_type = std::size_t;
	using difference_type = typename std::iterator_traits<Iter>::difference_type;
	using reference = typename std::iterator_traits<Iter>::reference;
	using const_reference = const reference;
	using pointer = typename std::iterator_traits<Iter>::pointer;
	using const_pointer = const pointer;
	using iterator = Iter;
	using reverse_iterator = std::reverse_iterator<Iter>;

	range() : i(), e() {}
	range(const Iter& i, const Iter& e) : i(i), e(e) {}
	range(Iter&& i, Iter&& e) : i(std::move(i)), e(std::move(e)) {}
	range(const std::pair<Iter, Iter>& p) : i(p.first), e(p.second) {}
	range(std::pair<Iter, Iter>&& p) : i(std::move(p.first)), e(std::move(p.second)) {}

	bool empty() const { return i == e; }
	
	size_type size() const { return e - i; }

	reference at( size_type n ) {
		if ( !(n < size()) ) throw std::out_of_range{};
		return i[n];
	}
	const_reference at( size_type n ) const {
		if ( !(n < size()) ) throw std::out_of_range{};
		return i[n];
	}

	reference operator[] ( size_type n ) { return i[n]; }
	const_reference operator[] ( size_type n ) const { return i[n]; }

	/// Excludes first element from range
	range<Iter>& operator++ () { ++i; return *this; }
	/// Excludes first element from range, returning old range
	range<Iter> operator++ (int) { range<Iter> tmp = *this; ++i; return tmp; }
	
	/// Returns a new range with first n elements excluded
	range<Iter> operator+ ( size_type n ) const { return { i + n, e }; }

	/// Exclude first n elements from range
	range<Iter>& operator+= ( size_type n ) const { i += n; return *this; }

	reference front() { return *i; }
	const_reference front() const { return *i; }

	reference back() { auto tmp = e; return *--tmp; }
	const_reference back() const { auto tmp = e; return *--tmp; }

	iterator begin() { return i; }

	iterator end() { return e; }

	reverse_iterator rbegin() { return { e }; }

	reverse_iterator rend() { return { i }; }
};

template<typename Iter> bool operator== (const range<Iter>& a, const range<Iter>& b ) {
	return std::equal( a.begin(), a.end(), b.begin(), b.end() );
}

template<typename Iter> bool operator!= (const range<Iter>& a, const range<Iter>& b ) {
	return ! std::equal( a.begin(), a.end(), b.begin(), b.end() );
}

template<typename Iter> bool operator< (const range<Iter>& a, const range<Iter>& b ) {
	return std::lexicographical_compare( a.begin(), a.end(), b.begin(), b.end() );
}

template<typename Iter> bool operator<= (const range<Iter>& a, const range<Iter>& b ) {
	return ! std::lexicographical_compare( b.begin(), b.end(), a.begin(), a.end() );
}

template<typename Iter> bool operator> (const range<Iter>& a, const range<Iter>& b ) {
	return std::lexicographical_compare( b.begin(), b.end(), a.begin(), a.end() );
}

template<typename Iter> bool operator>= (const range<Iter>& a, const range<Iter>& b ) {
	return ! std::lexicographical_compare( a.begin(), a.end(), b.begin(), b.end() );
}

/// Makes a range from two Iters
template<typename Iter>
range<Iter> make_range( const Iter& i, const Iter& e ) { return { i, e }; }
template<typename Iter>
range<Iter> make_range( Iter&& i, Iter&& e ) { return { std::move(i), std::move(e) }; }

/// Makes a range from a std::pair<Iter, Iter>
template<typename Iter>
range<Iter> make_range( const std::pair<Iter, Iter>& p ) { return { p }; }
template<typename Iter>
range<Iter> make_range( std::pair<Iter, Iter>&& p ) { return { std::move(p) }; }

/// Makes a singleton range for a single element
template<typename T>
range<T*> singleton( T& x ) { return { &x, &x + 1 }; }
template<typename T>
range<const T*> singleton( const T& x ) { return { &x, &x + 1 }; }

/// Builds a range from a collection
template<typename C>
auto range_from( C& c ) -> range<decltype( std::begin(c) )> {
	return { std::begin(c), std::end(c) };
}

/// Builds a reversed range from a collection
template<typename C>
auto reversed( C& c ) -> range<decltype( std::rbegin(c) )> {
	return { std::rbegin(c), std::rend(c) };
}
