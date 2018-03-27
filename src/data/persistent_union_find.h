#pragma once

#include <functional>
#include <utility>

#include "cast.h"
#include "persistent_map.h"
#include "gc.h"

/// Union-find based on a persistent map, as in Conchon & Filliatre "A 
/// Persistent Union-Find Data Structure". Nodes are augmented with an 
/// extra "next" field to allow quicker access to full classes

template<typename Elm, typename Hash = std::hash<Elm>, typename Eq = std::equal_to<Elm>>
class persistent_union_find {

	template<typename T, typename H, typename E>
	friend const GC& operator<< (const GC& gc, const persistent_union_find<T,H,E>&);

	struct Node {
		Elm parent;          ///< Parent node in equivalence class
		Elm next;            ///< Next node in equivalence class
		unsigned char rank;  ///< Rank of node

		template<typename E>
		Node(E&& e) : parent(e), next(std::forward<E>(e)), rank(0) {}

		template<typename E, typename F>
		Node(E&& p, F&& n, unsigned char r) 
			: parent(std::forward<E>(p)), next(std::forward<F>(n)), rank(r) {}
	};

	using Map = persistent_map<Elm, Node, Hash, Eq>;
	Map* base;  ///< Underlying map for union find

	/// Finds root for element i, performing path compression and returning new map 
	/// and root element
	Map::Entry find_aux(Map& f, Elm i) {
		// get entry, inserting if necessary
		Map::Entry fi = f[i];
		if ( ! fi.exists() ) { return fi = Node{ i }; }
		// return if root
		const Node& fin = fi.get();
		Elm p = fin.parent;
		if ( i == p ) return fi;
		// otherwise recurse, compressing path
		Elm n = fin.next;
		unsigned char r = fin.rank;

		Map::Entry fp = find_aux(f, p);
		p = fp.get_key();
		Map* f2 = fp.get_base()->set( i, Node{ p, n, r } );
		return { f2, p };
	}

public:
	persistent_union_find() : base(new Map{}) {}

	/// Finds root for element i, performing path compression and insertion as needed
	Elm find(Elm i) {
		Map::Entry fp = find_aux(*base, i);
		base = fp.get_base();
		return fp.get_key();
	}

	/// Checks if two elements are in different classes
	bool disjoint(Elm i, Elm j) { return find(i) != find(j); }

	/// Takes union of two element classes
	void merge(Elm i, Elm j) {
		Map::Entry fx = find_aux(*base, i);
		const Node& fxn = fx.get();
		Elm x = fxn.parent;
		Elm xx = fxn.next;
		unsigned char xr = fxn.rank;
		base = fx.get_base();

		Map::Entry fy = find_aux(*base, j);
		
		// return early if already merged
		if ( x == fy.get_key() ) return;

		const Node& fyn = fy.get();
		Elm y = fyn.root;
		Elm yy = fyn.next;
		unsigned char yr = fyn.rank;

		// unify classes
		if ( xr < yr ) {
			// place x under y, splicing lists together
			base = base->set( x, Node{ y, yy, xr } );
			base = base->set( y, Node{ y, xx, yr } );
		} else /* if ( xr >= yr ) */ {
			// place y under x, splicing lists together
			base = base->set( y, Node{ x, xx, yr } );
			// increase x's rank if ranks are equal
			base = base->set( x, Node{ x, yy, xr + (unsigned char)(xr == yr) } );
		}
	}

	bool contains(Elm i) const { return base->count( i ); }

	/// Reports all members of a class to `out`; none if i does not exist in map
	template<typename OutputIterator>
	void find_class(Elm i, OutputIterator& out) const {
		// skip empty classes
		if ( ! base->count( i ) ) return;

		Elm crnt = i;
		do {
			*out++ = crnt;
			crnt = base->get( crnt ).next;
		} while ( crnt != i );
	}
};

template<typename T, typename H, typename E>
const GC& operator<< (const GC& gc, const persistent_union_find<T,H,E>& uf) {
	gc << uf.base;
}
