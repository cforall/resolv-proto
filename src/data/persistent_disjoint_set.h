#pragma once

#include <functional>
#include <unordered_map>
#include <utility>

#include "cast.h"
#include "debug.h"
#include "persistent_map.h"
#include "gc.h"

/// Persistent disjoint-set data structure based on the persistent array in 
/// Conchon & Filliatre "A Persistent Union-Find Data Structure". Path 
/// compression is not performed (to lower cost of persistent rollback). 
/// Auxilliary lists are kept for efficient retrieval of class members.

template<typename Elm, typename Hash = std::hash<Elm>, typename Eq = std::equal_to<Elm>>
class persistent_disjoint_set : public GC_Object {
	template<typename T, typename H, typename E>
	friend const GC& operator<< (const GC& gc, const persistent_disjoint_set<T,H,E>&);

	/// Type of this class
	using Self = persistent_disjoint_set<Elm, Hash, Eq>;

	/// Type of node height
	using Height = unsigned char;

	/// Disjoint-set node
	struct Node {
		Elm parent;     ///< Parent node in equivalence class
		Elm next;       ///< Next node in equivalence class
		Height height;  ///< Tree height of the node

		template<typename E>
		Node(E&& e) : parent(e), next(std::forward<E>(e)), height(0) {}

		template<typename E, typename F>
		Node(E&& p, F&& n, Height h) 
			: parent(std::forward<E>(p)), next(std::forward<F>(n)), height(r) {}
	};

	/// Type of class map
	using Base = std::unordered_map<Elm, Node, Hash, Eq>;

	/// Node inserted into underlying map as new equivalence class
	struct Add {
		Self* base;  ///< Modified map
		Elm root;    ///< Element added

		template<typename E>
		Add(Self* b, E&& r) : base(b), root(std::forward<E>(r)) {}
	};

	/// Two classes merged
	struct AddTo {
		Self* base;       ///< Modified map
		Elm root;         ///< Root node
		Elm child;        ///< Child node, formerly root of own class
		bool new_height;  ///< Did the root node's height change?

		template<typename R, typename C>
		AddTo(Self* b, R&& r, C&& c, bool h) 
			: base(b), root(std::forward<R>(r)), child(std::forward<C>(c)), new_height(h) {}
	};

	/// Underlying storage
	union Data {
		char none;
		Base base;
		Add add;
		AddTo add_to;

		Data() : none('\0') {}
		~Data() {}
	} data;

	/// Type of node
	mutable enum Mode { 
		BASE,    ///< Base node
		ADD,     ///< Add operation
		REM,     ///< Reverse Add
		ADDTO,   ///< AddTo operation
		REMFROM  ///< Reverse AddTo
	} mode;

	/// get mutable reference as T
	template<typename T>
	T& as() { return reinterpret_cast<T&>(data); }

	/// get const reference as T
	template<typename T>
	const T& as() const { return reinterpret_cast<const T&>(data); }

	/// get mutable reference as T from const
	template<typename T>
	T& as_non_const() const { return const_cast<T&>(reinterpret_cast<const T&>(data)); }

	/// get rvalue reference as T
	template<typename T>
	T&& take_as() { return std::move(as<T>()); }

	/// initialize as T
	template<typename T, typename... Args>
	void init( Args&&... args ) {
		new( &as<T>() ) T { std::forward<Args>(args)... };
	}

	/// reset according to current mode
	void reset() {
		switch( mode ) {
			case BASE:                as<Base>().~Base();   break;
			case ADD: case REM:       as<Add>().~Add();     break;
			case ADDTO: case REMFROM: as<AddTo>().~AddTo(); break;
			default: unreachable("invalid mode");
		}
	}

	persistent_disjoint_set( Mode m, Base&& b ) : data(), mode(m) {
		assume(m == BASE, "invalid mode");
		init<Base>(std::move(b));
	}

	template<typename R>
	persistent_disjoint_set( Mode m, const Self* b, R&& r ) : data(), mode(m) {
		assume(m == ADD || m == REM, "invalid mode");
		init<Add>(b, std::forward<R>(r));
	}

	template<typename R, typename C>
	persistent_disjoint_set( Mode m, const Self* b, R&& r, C&& c, bool h ) : data(), mode(m) {
		assume(m == ADDTO || m == REMFROM, "invalid mode");
		init<AddTo>(b, std::forward<R>(r), std::forward<C>(c), h);
	}

	/// Adds (also removes) graph edges.
	/// * `from.parent` updated to `new_root`, 
	/// * `from.next` and `to.next` swapped (splices or un-splices class lists)
	/// * `to.height` adjusted by change
	template<typename R>
	static void addEdge( Node& from, Node& to, R&& new_root, Height change ) {
		from.parent = std::forward<R>(new_root);
		std::swap(from.next, to.next);
		to.height += change;
	}

protected:
	void trace( const GC& gc ) const {
		switch( mode ) {
			case BASE: {
				for (const auto& entry : as<Base>()) {
					gc << entry.first << entry.second;
				}
				return;
			}
			case ADD: case REM: {
				const Add& self = as<Add>();
				gc << self.base << self.root;
				return;
			}
			case ADDTO: case REMFROM: {
				const AddTo& self = as<AddTo>();
				gc << self.base << self.root << self.child;
				return;
			}
			default: unreachable("invalid mode");
		}
	}

public:
	using size_type = std::size_t;

	using iterator = typename Base::const_iterator;

	persistent_disjoint_set() : data(), mode(BASE) { init<Base>(); }

	persistent_disjoint_set( const Self& o ) = delete;

	Self& operator= ( const Self& o ) = delete;

	~persistent_disjoint_set() { reset(); }

	/// reroot persistent data structure at current node
	void reroot() const {
		if ( mode == BASE ) return;

		// reroot base
		Self* base = ( mode == ADD || mode == REM ) ? 
			as_non_const<Add>().base : 
			as_non_const<AddTo>().base;
		base->reroot();
		assume(base->mode == BASE, "reroot results in base");

		// take map out of base
		Base&& base_map = base->take_as<Base>();
		base->reset();

		// switch base to inverse of self and mutate base map
		switch ( mode ) {
			case ADD: {
				Add& self = as_non_const<Add>();

				base->init<Add>( this, self.root );
				base->mode = REM;

				base_map.emplace( self.root, Node{ std::move(self.root) } );
			} break;
			case REM: {
				Add& self = as_non_const<Add>();

				base->init<Add>( this, self.root );
				base->mode = ADD;

				base_map.erase( self.root );
			} break;
			case ADDTO: {
				AddTo& self = as_non_const<AddTo>();

				base->init<AddTo>( this, self.root, self.child, self.new_height );
				base->mode = REMFROM;

				auto child_it = base_map.find( self.child );
				auto root_it = base_map.find( self.root );
				assume(child_it != base_map.end() && root_it != base_map.end(), 
					"nodes must exist in base");
				Node& child = child_it->second;
				Node& root = root_it->second;

				addEdge( child, root, std::move(self.root), Height(self.new_height) );
			} break;
			case REMFROM: {
				AddTo& self = as_non_const<AddTo>();

				base->init<AddTo>( this, self.root, self.child, self.new_height );
				base->mode = ADDTO;

				auto child_it = base_map.find( self.child );
				auto root_it = base_map.find( self.root );
				assume(child_it != base_map.end() && root_it != base_map.end(), 
					"nodes must exist in base");
				Node& child = child_it->second;
				Node& root = root_it->second;

				addEdge( child, root, std::move(self.child), Height(-1 * self.new_height) );
			} break;
			default: unreachable("invalid mode");
		}

		// set base map into self
		reset();
		init<Base>( std::move(base_map) );
		mode = BASE;
	}

private:
	/// Gets the base after rerooting at the current node 
	const Base& rerooted() const {
		reroot();
		return as<Base>();
	}

public:
	/// true if the set of sets is empty
	bool empty() const { return rerooted().empty(); }

	/// Get number of entries in the map
	size_type size() const { return rerooted().size(); }

	/// Get begin iterator for map; may be invalidated by calls to non-iteration functions 
	/// or functions on other maps in the same chain
	iterator begin() const { return rerooted().begin(); }

	/// Get end iterator for map; may be invalidated by calls to non-iteration functions 
	/// or functions on other maps in the same chain
	iterator end() const { return rerooted().end(); }

	/// Check if value is present
	size_type count(Elm i) const { return rerooted().count( i ); }

	/// Finds root for element i, assumes i is present
	Elm find(Elm i) const {
		const Base& self = rerooted();
		
		auto it = self.find( i );
		while (true) {
			assume(it != self.end(), "find target not present");

			if ( it->first == it->second.parent ) return it->first;

			it = self.find( it->second.parent );
		}
	}

	/// Adds fresh class including only one item; returns updated map
	template<typename E>
	Self* add(E&& i) {
		reroot();
		
		// transfer map to new node
		Self* ret = new Self{ BASE, take_as<Base>() };
		reset();

		// set self to REM node
		init<Add>( ret, i );
		mode = REM;
		
		// add element in returned map
		Base& base_map = ret->as<Base>();
		bool added = base_map.emplace( i, Node{ std::forward<E>(i) } ).second;
		assume(added, "added element already present in map");

		return ret;
	}

	/// Merges two classes given by their roots; returns updated map
	Self* merge(Elm i, Elm j) {
		reroot();

		// transfer map to new node
		Self* ret = new Self{ BASE, take_as<Base>() };
		reset();

		// find set nodes
		Base& base_map = ret->as<Base>();
		auto it = base_map.find( i );
		auto jt = base_map.find( j );
		assume(it != base_map.end() && jt != base_map.end(), "nodes must exist in base");
		Node& in = it->second;
		Node& jn = jt->second;

		// update returned map and set self to appropriate REMFROM node
		if ( in.height < jn.height ) {
			addEdge( in, jn, j, 0 );
			init<AddTo>( ret, j, i, false );
		} else if ( jn.height < in.height ) {
			addEdge( jn, in, i, 0 );
			init<AddTo>( ret, i, j, false );
		} else /* if ( jn.height == in.height ) */ {
			addEdge( jn, in, i, 1 );
			init<AddTo>( ret, i, j, true );
		}
		mode = REMFROM;

		return ret;
	}

private:
	using Map = persistent_map<Elm, Node, Hash, Eq>;
	Map* base;  ///< Underlying map for union find

	/// Finds root for element i, performing path compression and returning new map 
	/// and root element
	static Map::Entry find_aux(Map& f, Elm i) {
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
	using iterator = Map::iterator;
	
	persistent_disjoint_set() : base(new Map{}) {}

	/// Begin iterator for map; invalidated under the same conditions as persistent_map::begin()
	iterator begin() const { return base->begin(); }

	/// End iterator for map; invalidated under the same conditions as persistent_map::end()
	iterator end() const { return base->end(); }

	/// Finds root for element i, performing path compression and insertion as needed
	Elm find(Elm i) const {
		Map::Entry fp = find_aux(*as_non_const(base), i);
		as_non_const(base) = fp.get_base();
		return fp.get_key();
	}

	/// Checks if two elements are in different classes
	bool disjoint(Elm i, Elm j) { return find(i) != find(j); }

	/// Inserts a previously absent node into the map
	void insert(Elm i) {
		assume(base->count( i ) == 0, "Element is not present in map");
		base->set( i, Node{ i } );
	}

	/// Inserts a previously absent node into the map in an existing equivalence class
	void insert(Elm i, Elm c) {
		assume(base->count( i ) == 0, "Element is not present in map");

		// get class parameters
		Map::Entry fp = find_aux(*base, c);
		const Node& pn = fp.get();
		Elm p = pn.parent;
		Elm n = pn.next;
		unsigned char r = pn.rank;

		// splice into equivalence class, increasing rank if necessary
		base = base->set( i, Node{ p, n, 0 } );
		base = base->set( p, Node{ p, i, r + (unsigned char)(r == 0) } );
	}

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
const GC& operator<< (const GC& gc, const persistent_disjoint_set<T,H,E>& uf) {
	gc << uf.base;
}
