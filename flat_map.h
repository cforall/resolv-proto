#pragma once

#include <iterator>
#include <map>
#include <unordered_map>
#include <utility>

/// An iterator for a map with collection-type values that iterates over the 
/// collections contained in the map entries
/// @param Iter the outer iterator type
template<typename Iter>
class FlatMapIter : public std::iterator<
		std::input_iterator_tag,
		typename Iter::value_type::second_type::value_type
		typename Iter::value_type::second_type::difference_type
		typename Iter::value_type::second_type::pointer
		typename Iter::value_type::second_type::reference > {
friend class ConstFlatMapIter;
public:
	typedef typename Iter::value_type::second_type InnerColl;
	typedef typename InnerColl::iterator Inner;
	typedef typename InnerColl::reference reference;
	typedef typename InnerColl::pointer pointer;

private:
	Iter i;
	Inner j;
	bool j_set;
	
	/// Gets j, initializing it to i->begin() if needed
	Inner& get_j() {
		if ( ! j_set ) {
			j = i->begin();
			j_set = true;
		}
		return j;
	}
	
	/// Clears j
	void clear_j() { j_set = false; }
	
public:
	/// Constructs a new iterator wrapping i (which may not be dereferencable)
	FlatMapIter( const Iter& i ) 
		: i(i), j_set(false) { /* j deliberately uninitialized */ }
	
	/// Constructs a new iterator wrapping (i, j)
	FlatMapIter( const Iter& i, const Inner& j )
		: i(i), j(j), j_set(true) {}
	
	FlatMapIter( const FlatMapIter<Iter>& o )
		: i(o.i), j_set(o.j_set) { if ( j_set ) j = o.j; }
	FlatMapIter<Iter>& operator= ( const FlatMapIter<Iter>& o ) {
		i = o.i;
		j_set = o.j_set;
		if ( j_set ) j = o.j;
		
		return *this;
	}
	 
	reference operator* () { return *get_j(); }
	pointer operator-> () { return get_j().operator->(); }
	
	FlatMapIter<Iter>& operator++ () {
		if ( get_j() != i->end() ) {
			++j;
		} else {
			++i;
			clear_j();
		}
		return *this;
	}
	FlatMapIter<Iter> operator++(int) {
		FlatMapIter<Iter> tmp = *this; ++(*this); return tmp;
	}
	
	bool operator== ( const FlatMapIter<Iter>& o ) const {
		// i's need to match, j's need to be either both unset, or match
		return i == o.i 
			&& ( j_set ? (o.j_set && j == o.j) : ! o.j_set );
	}
	bool operator!= ( const FlatMapIter<Iter>& o ) const { return !(*this == o); }
};

/// An const iterator for a map with collection-type values that iterates over 
/// the collections contained in the map entries
/// @param Iter the outer iterator type
template<typename Iter>
class ConstFlatMapIter : public std::iterator<
		std::input_iterator_tag,
		typename Iter::value_type::second_type::value_type
		typename Iter::value_type::second_type::difference_type
		typename Iter::value_type::second_type::const_pointer
		typename Iter::value_type::second_type::const_reference > {
public:
	typedef typename Iter::value_type::second_type InnerColl;
	typedef typename InnerColl::const_iterator Inner;
	typedef typename InnerColl::const_reference reference;
	typedef typename InnerColl::const_pointer pointer;

private:
	Iter i;
	Inner j;
	bool j_set;
	
	/// Gets j, initializing it to i->begin() if needed
	Inner& get_j() {
		if ( ! j_set ) {
			j = i->cbegin();
			j_set = true;
		}
		return j;
	}
	
	/// Clears j
	void clear_j() { j_set = false; }
	
public:
	/// Constructs a new iterator wrapping i (which may not be dereferencable)
	ConstFlatMapIter( const Iter& i ) 
		: i(i), j_set(false) { /* j deliberately uninitialized */ }
	
	/// Constructs a new iterator wrapping (i, j)
	ConstFlatMapIter( const Iter& i, const Inner& j )
		: i(i), j(j), j_set(true) {}
	
	ConstFlatMapIter( const ConstFlatMapIter<Iter>& o )
		: i(o.i), j_set(o.j_set) { if ( j_set ) j = o.j; }
	ConstFlatMapIter<Iter>& operator= ( const ConstFlatMapIter<Iter>& o ) {
		i = o.i;
		j_set = o.j_set;
		if ( j_set ) j = o.j;
		
		return *this;
	}
	ConstFlatMapIter( const FlatMapIter<Iter>& o )
		: i(o.i), j_set(o.j_set) { if ( j_set ) j = o.j; }
	ConstFlatMapIter<Iter>& operator= ( const FlatMapIter<Iter>& o ) {
		i = o.i;
		j_set = o.j_set;
		if ( j_set ) j = o.j;
		
		return *this;
	}
	
	reference operator* () { return *get_j(); }
	pointer operator-> () { return get_j().operator->(); }
	
	ConstFlatMapIter<Iter>& operator++ () {
		if ( get_j() != i->cend() ) {
			++j;
		} else {
			++i;
			clear_j();
		}
		return *this;
	}
	ConstFlatMapIter<Iter> operator++(int) {
		ConstFlatMapIter<Iter> tmp = *this; ++(*this); return tmp;
	}
	
	bool operator== ( const ConstFlatMapIter<Iter>& o ) const {
		// i's need to match, j's need to be either both unset, or match
		return i == o.i 
			&& ( j_set ? (o.j_set && j == o.j) : ! o.j_set );
	}
	bool operator!= ( const ConstFlatMapIter<Iter>& o ) const { return !(*this == o); }
};

/// A map holding a collection that iterates and inserts directly into the 
/// contained collection.
/// @param Key      the key type
/// @param Inner    the inner collection type; should be default constructable
/// @param Extract  functor which extracts a key from the value type of the 
///                 inner collection
/// @param Map      the underlying map type [default std::unordered_map]
template<typename Key, typename Inner, typename Extract, 
         template<typename, typename> typename Map = std::unordered_map>
class FlatMap {
public:
	typedef Map<Key, Inner> Underlying;
	typedef Key key_type;
	typedef typename Inner::value_type value_type;
	typedef typename Underlying::size_type size_type;
	typedef typename Underlying::difference_type difference_type;
	typedef typename Inner::reference reference;
	typedef typename Inner::const_reference const_reference;
	typedef typename Inner::pointer pointer;
	typedef typename Inner::const_pointer const_pointer;
	typedef FlatMapIter<typename Underlying::iterator> iterator;
	typedef ConstFlatMapIter<typename Underlying::const_iterator> const_iterator;

private:
	Underlying m;  ///< Underlying map
	size_type n;   ///< Element count
	
	/// An iterator pointing to the value for k in the underlying map, freshly 
	/// created if required.
	typename Underlying::iterator find_or_create( const Key& k ) {
		auto i = m.find( k );
		return i == m.end() ? 
			m.insert( i, std::make_pair(k, Inner{}) ) :
			i;
	}
	
public:
	iterator begin() { return m.begin(); }
	const_iterator begin() const { return m.begin(); }
	const_iterator cbegin() const { return m.cbegin(); }
	iterator end() { return m.end(); }
	const_iterator end() const { return m.end(); }
	const_iterator cend() const { return m.cend(); }
	
	bool empty() const { return n == 0; }
	size_type size() const { return n; }
	
	void clear() { m.clear(); n = 0; }
	
	iterator insert( const value_type& v ) {
		const Key& k = Extract()( v );
		auto i = find_or_create( k );
		auto j = i->insert( i->end(), v );
		++n;
		
		return iterator{i, j};
	}
	iterator insert( value_type&& v ) {
		const Key& k = Extract()( v );
		auto i = find_or_create( k );
		auto j = i->insert( i->end(), std::move(v) );
		++n;
		
		return iterator{i, j};
	}
	iterator insert( const const_iterator&, const value_type& v ) { 
		return insert(v);
	}
	iterator insert( const const_iterator&, value_type&& ) {
		return insert( std::move(v) );
	}
	
	iterator find( const Key& k ) { return m.find( k ); }
	const_iterator find( const Key& k ) const { return m.find( k ); }
	size_type count( const Key& k ) const {
		auto i = m.find( k );
		return i == m.end() ? 0 : i->size();
	}
	
	Inner& operator[] ( const Key& k ) { return *find_or_create( k ); }
};

/// Wrapper for flat map using a sorted map
template<typename Key, typename Inner, typename Extract>
using SortedFlatMap = FlatMap<Key, Inner, Extract, std::map>;
