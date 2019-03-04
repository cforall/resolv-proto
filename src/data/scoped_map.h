//
// Cforall Version 1.0.0 Copyright (C) 2015 University of Waterloo
//
// The contents of this file are covered under the licence agreement in the
// file "LICENCE" distributed with this repository.
//
// ScopedMap.h --
//
// Author           : Aaron B. Moss
// Created On       : Wed Dec 02 11:37:00 2015
// Last Modified By : Aaron B. Moss
// Last Modified On : Thu Sep 06 14:19:40 2018
// Update Count     : 4
//

#pragma once

#include <cassert>
#include <iterator>
#include <utility>
#include <vector>

#include "std_wrappers.h"

/// A map where the items are placed into nested scopes;
/// inserted items are placed into the innermost scope, lookup looks from the innermost scope 
/// outward.
/// @param Key     The key type
/// @param Value   The mapped type
/// @param Map     Internal map type for each scope [default std::unordered_map]
template<typename Key, typename Value, 
		 template<typename, typename> class Map = std_unordered_map>
class ScopedMap {
	typedef Map< Key, Value > Scope;
	typedef std::vector< Scope > ScopeList;

	ScopeList scopes; ///< scoped list of maps
public:
	typedef typename Scope::key_type key_type;
	typedef typename Scope::mapped_type mapped_type;
	typedef typename Scope::value_type value_type;
	typedef typename ScopeList::size_type size_type;
	typedef typename ScopeList::difference_type difference_type;
	typedef typename Scope::reference reference;
	typedef typename Scope::const_reference const_reference;
	typedef typename Scope::pointer pointer;
	typedef typename Scope::const_pointer const_pointer;
	typedef typename Scope::iterator inner_iterator;
	typedef typename Scope::const_iterator inner_const_iterator;

	class iterator : public std::iterator< std::bidirectional_iterator_tag,
	                                       value_type > {
	friend class ScopedMap;
	friend class const_iterator;
		typedef typename ScopedMap::inner_iterator wrapped_iterator;
		typedef typename ScopedMap::ScopeList scope_list;
		typedef typename scope_list::size_type size_type;

		/// Checks if this iterator points to a valid item
		bool is_valid() const {
			return it != (*scopes)[level].end();
		}

		/// Increments on invalid
		iterator& next_valid() {
			if ( ! is_valid() ) { ++(*this); }
			return *this;
		}

		/// Decrements on invalid
		iterator& prev_valid() {
			if ( ! is_valid() ) { --(*this); }
			return *this;
		}

		iterator(scope_list &_scopes, const wrapped_iterator &_it, size_type inLevel)
			: scopes(&_scopes), it(_it), level(inLevel) {}
	public:
		iterator(const iterator &that) : scopes(that.scopes), it(that.it), level(that.level) {}
		iterator& operator= (const iterator &that) {
			scopes = that.scopes; level = that.level; it = that.it;
			return *this;
		}

		reference operator* () { return *it; }
		const_reference operator* () const { return *it; }
		pointer operator-> () { return it.operator->(); }
		const_pointer operator-> () const { return it.operator->(); }

		iterator& operator++ () {
			if ( it == (*scopes)[level].end() ) {
				if ( level == 0 ) return *this;
				--level;
				it = (*scopes)[level].begin();
			} else {
				++it;
			}
			return next_valid();
		}
		iterator operator++ (int) { iterator tmp = *this; ++(*this); return tmp; }

		iterator& operator-- () {
			// may fail if this is the begin iterator; allowed by STL spec
			if ( it == (*scopes)[level].begin() ) {
				++level;
				it = (*scopes)[level].end();
			}
			--it;
			return prev_valid();
		}
		iterator operator-- (int) { iterator tmp = *this; --(*this); return tmp; }

		bool operator== (const iterator &that) const {
			return scopes == that.scopes && level == that.level && it == that.it;
		}
		bool operator!= (const iterator &that) const { return !( *this == that ); }

		size_type get_level() const { return level; }

	private:
		scope_list *scopes;
		wrapped_iterator it;
		size_type level;
	};

	class const_iterator : public std::iterator< std::bidirectional_iterator_tag,
	                                             value_type > {
	friend class ScopedMap;
		typedef typename ScopedMap::inner_iterator wrapped_iterator;
		typedef typename ScopedMap::inner_const_iterator wrapped_const_iterator;
		typedef typename ScopedMap::ScopeList scope_list;
		typedef typename scope_list::size_type size_type;

		/// Checks if this iterator points to a valid item
		bool is_valid() const {
			return it != (*scopes)[level].end();
		}

		/// Increments on invalid
		const_iterator& next_valid() {
			if ( ! is_valid() ) { ++(*this); }
			return *this;
		}

		/// Decrements on invalid
		const_iterator& prev_valid() {
			if ( ! is_valid() ) { --(*this); }
			return *this;
		}

		const_iterator(scope_list const &_scopes, const wrapped_const_iterator &_it, size_type inLevel)
			: scopes(&_scopes), it(_it), level(inLevel) {}
	public:
		const_iterator(const iterator &that) : scopes(that.scopes), it(that.it), level(that.level) {}
		const_iterator(const const_iterator &that) : scopes(that.scopes), it(that.it), level(that.level) {}
		const_iterator& operator= (const iterator &that) {
			scopes = that.scopes; level = that.level; it = that.it;
			return *this;
		}
		const_iterator& operator= (const const_iterator &that) {
			scopes = that.scopes; level = that.level; it = that.it;
			return *this;
		}

		const_reference operator* () { return *it; }
		const_pointer operator-> () { return it.operator->(); }

		const_iterator& operator++ () {
			if ( it == (*scopes)[level].end() ) {
				if ( level == 0 ) return *this;
				--level;
				it = (*scopes)[level].begin();
			} else {
				++it;
			}
			return next_valid();
		}
		const_iterator operator++ (int) { const_iterator tmp = *this; ++(*this); return tmp; }

		const_iterator& operator-- () {
			// may fail if this is the begin iterator; allowed by STL spec
			if ( it == (*scopes)[level].begin() ) {
				++level;
				it = (*scopes)[level].end();
			}
			--it;
			return prev_valid();
		}
		const_iterator operator-- (int) { const_iterator tmp = *this; --(*this); return tmp; }

		bool operator== (const const_iterator &that) const {
			return scopes == that.scopes && level == that.level && it == that.it;
		}
		bool operator!= (const const_iterator &that) const { return !( *this == that ); }

		size_type get_level() const { return level; }

	private:
		scope_list const *scopes;
		wrapped_const_iterator it;
		size_type level;
	};

	/// Starts a new scope
	void beginScope() {
		scopes.emplace_back();
	}

	/// Ends a scope; invalidates any iterators pointing to elements of that scope
	void endScope() {
		scopes.pop_back();
		assert( ! scopes.empty() );
	}

	/// Default constructor initializes with one scope
	ScopedMap() : scopes() { beginScope(); }

	/// Clears all elements from all scopes
	void clear() { scopes.clear(); }

	iterator begin() { return iterator(scopes, scopes.back().begin(), currentScope()).next_valid(); }
	const_iterator begin() const { return const_iterator(scopes, scopes.back().begin(), currentScope()).next_valid(); }
	const_iterator cbegin() const { return const_iterator(scopes, scopes.back().begin(), currentScope()).next_valid(); }
	iterator end() { return iterator(scopes, scopes[0].end(), 0); }
	const_iterator end() const { return const_iterator(scopes, scopes[0].end(), 0); }
	const_iterator cend() const { return const_iterator(scopes, scopes[0].end(), 0); }

	/// Gets the index of the current scope (counted from 0)
	size_type currentScope() const { return scopes.size() - 1; }

	/// Finds the given key in the outermost scope it occurs; returns end() for none such
	iterator find( const Key &key ) {
		for ( size_type i = scopes.size() - 1; ; --i ) {
			inner_iterator val = scopes[i].find( key );
			if ( val != scopes[i].end() ) return iterator( scopes, val, i );
			if ( i == 0 ) break;
		}
		return end();
	}
	const_iterator find( const Key &key ) const {
			return const_iterator( const_cast< ScopedMap< Key, Value >* >(this)->find( key ) );
	}

	/// Finds the given key in the provided scope; returns end() for none such
	iterator findAt( size_type scope, const Key& key ) {
		inner_iterator val = scopes[scope].find( key );
		if ( val != scopes[scope].end() ) return iterator( scopes, val, scope );
		return end();
	}
	const_iterator findAt( size_type scope, const Key& key ) const {
		return const_iterator( const_cast< ScopedMap< Key, Value >* >(this)->findAt( scope, key ) );
	}

	/// Finds the given key in the outermost scope inside the given scope where it occurs
	iterator findNext( const_iterator &it, const Key &key ) {
		if ( it.level == 0 ) return end();
		for ( size_type i = it.level - 1; ; --i ) {
			inner_iterator val = scopes[i].find( key );
			if ( val != scopes[i].end() ) return iterator( scopes, val, i );
			if ( i == 0 ) break;
		}
		return end();
	}
	const_iterator findNext( const_iterator &it, const Key &key ) const {
			return const_iterator( const_cast< ScopedMap< Key, Value >* >(this)->findNext( it, key ) );
	}

	/// Finds the given key in all scopes where it appears; returns vector of all references from 
	/// outermost scope inward, empty vector if none such
	std::vector<inner_iterator> findAll( const Key& key ) {
		std::vector<inner_iterator> found;
		for ( size_type i = scopes.size() - 1; ; --i ) {
			inner_iterator val = scopes[i].find( key );
			if ( val != scopes[i].end() ) { found.emplace_back( val ); }
			if ( i == 0 ) break;
		}
		return found;
	}
	std::vector<inner_const_iterator> findAll( const Key& key ) const {
		std::vector<inner_const_iterator> found;
		for ( size_type i = scopes.size() - 1; ; --i ) {
			inner_const_iterator val = scopes[i].find( key );
			if ( val != scopes[i].end() ) { found.emplace_back( val ); }
			if ( i == 0 ) break;
		}
		return found;
	}

	/// Inserts the given key-value pair into the outermost scope
	template< typename value_type_t >
	std::pair< iterator, bool > insert( value_type_t&& value ) {
		std::pair< inner_iterator, bool > res = scopes.back().insert( std::forward<value_type_t>( value ) );
		return std::make_pair( iterator(scopes, std::move( res.first ), scopes.size()-1), std::move( res.second ) );
	}

	template< typename value_type_t >
	iterator insert( iterator at, value_type_t&& value ) {
		Scope& scope = (*at.scopes) [ at.level ];
		return iterator{ scopes, 
			scope.insert( std::forward<value_type_t>( value ) ).first, at.level };
	}

	template< typename value_t >
	std::pair< iterator, bool > insert( const Key &key, value_t&& value ) { return insert( std::make_pair( key, std::forward<value_t>( value ) ) ); }

	template< typename value_type_t >
	std::pair< iterator, bool > insertAt( size_type scope, value_type_t&& value ) {
		std::pair< inner_iterator, bool > res = scopes.at(scope).insert( std::forward<value_type_t>( value ) );
		return std::make_pair( iterator(scopes, std::move( res.first ), scope), std::move( res.second ) );
	}

	template< typename value_t >
	std::pair< iterator, bool > insertAt( size_type scope, const Key& key, value_t&& value ) {
		return insertAt( scope, std::make_pair( key, std::forward<value_t>( value ) ) );
	}

	Value& operator[] ( const Key &key ) {
		iterator slot = find( key );
		if ( slot != end() ) return slot->second;
		return insert( key, Value() ).first->second;
	}

	iterator erase( iterator pos ) {
		Scope& scope = (*pos.scopes) [ pos.level ];
		const typename iterator::wrapped_iterator& new_it = scope.erase( pos.it );
		iterator it( *pos.scopes, new_it, pos.level );
		return it.next_valid();
	}

	size_type count( const Key &key ) const {
		size_type c = 0;
		auto it = find( key );
		auto end = cend();

		while(it != end) {
			c++;
			it = findNext(it, key);
		}

		return c;
	}

};

// Local Variables: //
// tab-width: 4 //
// mode: c++ //
// compile-command: "make install" //
// End: //
