#pragma once

// Copyright (c) 2015 University of Waterloo
//
// The contents of this file are covered under the licence agreement in 
// the file "LICENCE" distributed with this repository.

#include <iterator>
#include <vector>

#include "cast.h"
#include "mem.h"

/// A lazily-generated list that caches the result of a generating iterator.
///
/// @param Gen	A forward-iterator with an explicit const cast to bool 
///             (true for non-empty)
template<typename Gen>
class lazy_list {
public:
	using value_type = typename std::iterator_traits<Gen>::value_type;
	using cache_type = std::vector<value_type>;
	using size_type = cache_type::size_type;
	using difference_type = cache_type::difference_type;
	using reference = cache_type::reference;
	using const_reference = cache_type::const_reference;
	using pointer = cache_type::pointer;
	using const_pointer = cache_type::const_pointer;

private:
	cache_type cache;  ///< Cache of previously produced values
	Gen next;          ///< Generator for new values

	/// Fills the cache to i elements, if possible
	void fill_to( size_type i ) const {
		while ( i >= cache.size() && next ) {
			as_non_const(cache).push_back(*next);
			++as_non_const(next);
		}
	}

public:
	lazy_list() : cache(), next() {}
	lazy_list(const Gen& g) : cache(), next(g) {}
	lazy_list(Gen&& g) : cache(), next(move(g)) {}

	/// Have all the elements been generated?
	/// May become true between calls on a const lazy_list
	bool done() const { return !next; }

	/// Generate all elements. May change done() or size(), will create infinite loop on 
	/// infinite generator.
	void finish() const {
		while ( next ) {
			as_non_const(cache).push_back(*next);
			++as_non_const(next);
		}
	}

	/// Is the list empty and the generator is done
	bool empty() const { return cache.empty() && done(); }

	/// Size of the cache, plus one if the generator is not done.
	/// May grow between calls on a const lazy_list
	size_type size() const { return cache.size() + !done(); }

	/// Maximum possible number of elements in cache
	size_type max_size() const { return cache.max_size(); }

	/// Reserves storage
	void reserve( size_type i ) { cache.reserve(i); }

	/// Number of elements the cache has currently allocated space for
	size_type capacity() const { return cache.capacity(); }

	/// Element at i; may cache further elements from generator. 
	/// Throws std::out_of_range if i is past the end of the final list.
	reference at( size_type i ) { fill_to(i); return cache.at(i); }
	const_reference at( size_type i ) const { fill_to(i); return cache.at(i); }

	/// Element at i; unchecked, may cache further elements from generator.
	reference operator[] ( size_type i ) { fill_to(i); return cache[i]; }
	const_reference operator[] ( size_type i ) const { fill_to(i); return cache[i]; }

	/// Gets first element
	reference front() { fill_to(0); return cache.front(); }
	const_reference front() const { fill_to(0); return cache.front(); }

	class iterator : public std::iterator< std::random_access_iterator_tag,
			value_type, difference_type, pointer, reference> {
	friend class const_iterator;

		lazy_list& l;  ///< List referenced
		size_type i;   ///< Index in list
	
	public:
		iterator() : l(*(lazy_list*)0), i(std::numeric_limits<size_type>::max()) {}
		iterator(lazy_list& l) : l(l), i(std::numeric_limits<size_type>::max()) {}
		iterator(lazy_list& l, size_type i) : l(l), i(i) {}

		reference operator* () { return l[i]; }

		pointer operator-> () { return &l[i]; }

		iterator& operator++ () { ++i; return *this; }
		iterator operator++ (int) { auto tmp = *this; ++i; return tmp; }

		iterator& operator-- () { --i; return *this; }
		iterator operator-- (int) { auto tmp = *this; --i; return tmp; }

		iterator& operator+= (difference_type n) { i += n; return *this; }
		iterator operator+ (difference_type n) const { auto tmp = *this; return tmp += n; }

		iterator& operator-= (difference_type n) { i -= n; return *this; }
		iterator operator- (difference_type n) const { auto tmp = *this; return tmp -= n; }
		difference_type operator- (const iterator& o) const { return i - o.i; }

		reference operator[] (difference_type n) { return l[i + n]; }

		bool operator== (const iterator& o) const {
			return i == o.i 
				|| (i == std::numeric_limits<size_type>::max() && o.i >= o.l.size())
				|| (o.i == std::numeric_limits<size_type>::max() && i > l.size());
		}
		
		bool operator!= (const iterator& o) const { return !(*this == o); }

		bool operator< (const iterator& o) const { return i < o.i; }

		bool operator<= (const iterator& o) const { return i <= o.i; }

		bool operator> (const iterator& o) const { return i > o.i; }

		bool operator>= (const iterator& o) const { return i >= o.i; }
	};

	class const_iterator : public std::iterator< std::random_access_iterator_tag,
			value_type, difference_type, const_pointer, const_reference> {
		
		lazy_list& l;  ///< List referenced
		size_type i;   ///< Index in list
	
	public:
		const_iterator() : l(*(lazy_list*)0), i(std::numeric_limits<size_type>::max()) {}
		const_iterator(lazy_list& l) : l(l), i(std::numeric_limits<size_type>::max()) {}
		const_iterator(lazy_list& l, size_type i) : l(l), i(i) {}
		const_iterator(const iterator& o) : l(o.l), i(o.i) {}

		const_reference operator* () { return l[i]; }

		const_pointer operator-> () { return &l[i]; }

		const_iterator& operator++ () { ++i; return *this; }
		const_iterator operator++ (int) { auto tmp = *this; ++i; return tmp; }

		const_iterator& operator-- () { --i; return *this; }
		const_iterator operator-- (int) { auto tmp = *this; --i; return tmp; }

		const_iterator& operator+= (difference_type n) { i += n; return *this; }
		const_iterator operator+ (difference_type n) const { auto tmp = *this; return tmp += n; }

		const_iterator& operator-= (difference_type n) { i -= n; return *this; }
		const_iterator operator- (difference_type n) const { auto tmp = *this; return tmp -= n; }
		difference_type operator- (const const_iterator& o) const { return i - o.i; }

		const_reference operator[] (difference_type n) { return l[i + n]; }

		bool operator== (const const_iterator& o) const {
			return i == o.i 
				|| (i == std::numeric_limits<size_type>::max() && o.i >= o.l.size())
				|| (o.i == std::numeric_limits<size_type>::max() && i > l.size());
		}
		
		bool operator!= (const const_iterator& o) const { return !(*this == o); }

		bool operator< (const const_iterator& o) const { return i < o.i; }

		bool operator<= (const const_iterator& o) const { return i <= o.i; }

		bool operator> (const const_iterator& o) const { return i > o.i; }

		bool operator>= (const const_iterator& o) const { return i >= o.i; }
	};

	/// Iterator to the first element
	iterator begin() { return {*this, 0}; }
	const_iterator begin() const { return {*this, 0}; }
	const_iterator cbegin() const { return {*this, 0}; }

	/// Iterator to the last element
	iterator end() { return {*this}; }
	const_iterator end() const { return {*this}; }
	const_iterator cend() const { return {*this}; }

	/// Clears the current cache; does not reset the generator.
	void clear() { cache.clear(); }
};

/// Wrapper for an iterator that adds the boolean cast to be used as a generator for 
/// lazy_list.
template<typename Iter>
class generator_from : public std::iterator<
		std::forward_iterator_tag,
		typename std::iterator_traits<Iter>::value_type,
		typename std::iterator_traits<Iter>::difference_type,
		typename std::iterator_traits<Iter>::pointer,
		typename std::iterator_traits<Iter>::reference> {
	
	Iter i;  ///< Wrapped iterator
	Iter e;  ///< End iterator (tested against for empty)

public:
	generator_from() = default;
	generator_from(const Iter& e) : i(e), e(e) {}
	generator_from(Iter&& e) : i(copy(e)), e(move(e)) {}
	generator_from(const Iter& i, const Iter& e) : i(i), e(e) {}
	generator_from(Iter&& i, Iter&& e) : i(move(i)), e(move(e)) {}

	explicit operator bool() const { return i != e; }

	reference operator* () { return *i; }
	
	pointer operator-> () { return i.operator->(); }

	generator_from<Iter>& operator++ () { ++i; return this; }
	generator_from<Iter> operator++ (int) { auto tmp = *this; ++(*this); return tmp; }

	bool operator== ( const generator_from<Iter>& o ) const { return i == o.i; }
	
	bool operator!= ( const generator_from<Iter>& o ) const { return i != o.i; }
};
