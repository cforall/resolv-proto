#pragma once

#include <iterator>
#include <map>
#include <unordered_map>
#include <utility>

/// An iterator for a map with collection-type values that iterates over the 
/// collections contained in the map entries
/// @param Underlying the underlying map type
template<typename Underlying>
class FlatMapIter : public std::iterator<
		std::forward_iterator_tag,
		typename Underlying::value_type::second_type::value_type,
		typename Underlying::value_type::second_type::difference_type,
		typename Underlying::value_type::second_type::pointer,
		typename Underlying::value_type::second_type::reference > {
template<typename U> friend class ConstFlatMapIter;
public:
	typedef typename Underlying::value_type::second_type InnerColl;
	typedef typename Underlying::iterator Iter;
	typedef typename InnerColl::iterator Inner;
	typedef typename InnerColl::reference reference;
	typedef typename InnerColl::pointer pointer;

private:
	Iter i;   ///< Current iterator for outer range
	Iter e;   ///< End iterator for outer range
	Inner j;  ///< Inner iterator (not set if i == e)
	
	void init_j() {
		while ( i != e ) {
			if ( ! i->second.empty() ) {
				j = i->second.begin();
				break;
			}
			++i;
		}
	}
	
public:
	FlatMapIter() = default;
	
	/// Constructs a new end iterator
	FlatMapIter( const Iter& e ) : i(e), e(e), j() {}
	
	/// Constructs a new iterator wrapping [i, e)
	FlatMapIter( const Iter& i, const Iter& e ) : i(i), e(e) { init_j(); }
	
	/// Constructs a new iterator wrapping [i.j, e)
	FlatMapIter( const Iter& i, const Iter& e, const Inner& j ) : i(i), e(e), j(j) {}
	
	FlatMapIter( const FlatMapIter<Underlying>& ) = default;
	FlatMapIter<Underlying>& operator= ( const FlatMapIter<Underlying>& ) = default;
	 
	reference operator* () { return *j; }
	pointer operator-> () { return j.operator->(); }
	
	FlatMapIter<Underlying>& operator++ () {
		if ( ++j == i->second.end() ) {
			++i;
			init_j();
		}
		
		return *this;
	}
	FlatMapIter<Underlying> operator++(int) {
		FlatMapIter<Underlying> tmp = *this; ++(*this); return tmp;
	}
	
	bool operator== ( const FlatMapIter<Underlying>& o ) const {
		// i's need to match, j's need to match if not at end of range
		return i == o.i && ( i == e || j == o.j );
	}
	bool operator!= ( const FlatMapIter<Underlying>& o ) const {
		return !(*this == o);
	}
};

/// An const iterator for a map with collection-type values that iterates over 
/// the collections contained in the map entries
/// @param Underlying the underlying map type
template<typename Underlying>
class ConstFlatMapIter : public std::iterator<
		std::forward_iterator_tag,
		typename Underlying::value_type::second_type::value_type,
		typename Underlying::value_type::second_type::difference_type,
		typename Underlying::value_type::second_type::const_pointer,
		typename Underlying::value_type::second_type::const_reference > {
public:
	typedef typename Underlying::value_type::second_type InnerColl;
	typedef typename Underlying::const_iterator Iter;
	typedef typename InnerColl::const_iterator Inner;
	typedef typename InnerColl::const_reference reference;
	typedef typename InnerColl::const_pointer pointer;

private:
	Iter i;   ///< Current iterator for outer range
	Iter e;   ///< End iterator for outer range
	Inner j;  ///< Inner iterator (not set if i == e)
	
	void init_j() {
		while ( i != e ) {
			if ( ! i->second.empty() ) {
				j = i->second.begin();
				break;
			}
			++i;
		}
	}
	
public:
	ConstFlatMapIter() = default;
	
	/// Constructs a new end iterator
	ConstFlatMapIter( const Iter& e ) : i(e), e(e), j() {}
	
	/// Constructs a new iterator wrapping [i, e)
	ConstFlatMapIter( const Iter& i, const Iter& e ) : i(i), e(e) { init_j(); }
	
	/// Constructs a new iterator wrapping [i.j, e)
	ConstFlatMapIter( const Iter& i, const Iter& e, const Inner& j )
		: i(i), e(e), j(j) {}
	
	ConstFlatMapIter( const ConstFlatMapIter<Underlying>& ) = default;
	ConstFlatMapIter<Underlying>& operator= ( const ConstFlatMapIter<Underlying>& ) 
		 = default;
	ConstFlatMapIter( const FlatMapIter<Underlying>& o )
		: i(o.i), e(o.e), j(o.j) {}
	ConstFlatMapIter<Underlying>& operator= ( const FlatMapIter<Underlying>& o ) {
		i = o.i;
		e = o.e;
		j = o.j;
		
		return *this;
	}
	
	reference operator* () { return *j; }
	pointer operator-> () { return j.operator->(); }
	
	ConstFlatMapIter<Underlying>& operator++ () {
		if ( ++j == i->second.end() ) {
			++i;
			init_j();
		}
		
		return *this;
	}
	ConstFlatMapIter<Underlying> operator++(int) {
		ConstFlatMapIter<Underlying> tmp = *this; ++(*this); return tmp;
	}
	
	bool operator== ( const ConstFlatMapIter<Underlying>& o ) const {
		// i's need to match, j's need to match if not at end of range
		return i == o.i && ( i == e || j == o.j );
	}
	bool operator!= ( const ConstFlatMapIter<Underlying>& o ) const {
		return !(*this == o);
	}
};

/// Wrapper around std::unordered_map declaration with the proper number of template 
/// parameters for use with FlatMap
template<typename K, typename V>
using std_unordered_map = std::unordered_map<K, V>;

/// A map holding a collection that iterates and inserts directly into the 
/// contained collection.
/// @param Key      the key type
/// @param Inner    the inner collection type; should be default constructable
/// @param Extract  functor which extracts a key from the value type of the 
///                 inner collection
/// @param Map      the underlying map type [default std::unordered_map]
template<typename Key, typename Inner, typename Extract, 
         template<typename, typename> class Map = std_unordered_map>
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
	typedef FlatMapIter<Underlying> iterator;
	typedef ConstFlatMapIter<Underlying> const_iterator;

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
	iterator begin() { return iterator{ m.begin(), m.end() }; }
	const_iterator begin() const { return const_iterator{ m.begin(), m.end() }; }
	const_iterator cbegin() const { return const_iterator{ m.cbegin(), m.cend() }; }
	iterator end() { return iterator{ m.end() }; }
	const_iterator end() const { return const_iterator{ m.end() }; }
	const_iterator cend() const { return const_iterator{ m.cend() }; }
	
	bool empty() const { return n == 0; }
	size_type size() const { return n; }
	
	void clear() { m.clear(); n = 0; }
	
	iterator insert( const value_type& v ) {
		const Key& k = Extract()( v );
		auto i = find_or_create( k );
		auto j = i->second.insert( i->second.end(), v );
		++n;
		
		return iterator{ i, m.end(), j };
	}
	iterator insert( value_type&& v ) {
		const Key& k = Extract()( v );
		auto i = find_or_create( k );
		auto j = i->second.insert( i->second.end(), move(v) );
		++n;
		
		return iterator{ i, m.end(), j };
	}
	iterator insert( const const_iterator&, const value_type& v ) { 
		return insert(v);
	}
	iterator insert( const const_iterator&, value_type&& v ) {
		return insert( move(v) );
	}
	
	iterator find( const Key& k ) {
		auto i = m.find( k );
		return i == m.end() || i->second.empty() ?
			iterator{ m.end() } : iterator{ i, m.end() };
	}
	const_iterator find( const Key& k ) const {
		auto i = m.find( k );
		return i == m.end() || i->second.empty() ?
			const_iterator{ m.end() } : const_iterator{ i, m.end() };
	}
	size_type count( const Key& k ) const {
		auto i = m.find( k );
		return i == m.end() ? 0 : i->second.size();
	}
	
	Inner& operator[] ( const Key& k ) { return *find_or_create( k ); }
};

/// Wrapper around std::map declaration with the proper number of template 
/// parameters for use with FlatMap
template<typename K, typename V>
using std_map = std::map<K, V>;

/// Wrapper for flat map using a sorted map
template<typename Key, typename Inner, typename Extract>
using SortedFlatMap = FlatMap<Key, Inner, Extract, std_map>;
