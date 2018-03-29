#pragma once

#include <cstddef>
#include <functional>
#include <stdexcept>
#include <unordered_map>
#include <utility>

#include "debug.h"
#include "gc.h"

/// Wraps a hash table in a persistent data structure, using a technique based 
/// on the persistent array in Conchon & Filliatre "A Persistent Union-Find 
/// Data Structure"

template<typename Key, typename Val, 
         typename Hash = std::hash<Key>, typename Eq = std::equal_to<Key>>
class persistent_map : public GC_Object {
public:
	/// Type of this class
	using Self = persistent_map<Key, Val, Hash, Eq>;

	/// Types of version nodes
	enum Mode { 
		BASE,  ///< Root node of version tree
		REM,   ///< Key removal node
		INS,   ///< Key update node
		UPD    ///< Key update node
	};

private:
	/// Type of underlying hash table
	using Base = std::unordered_map<Key, Val, Hash, Eq>;
	
	/// Node inserted into underlying map
	struct Ins {
		Self* base;  ///< Modified map
		Key key;     ///< Key inserted
		Val val;     ///< Value stored

		template<typename K, typename V>
		Ins(Self* b, K&& k, V&& v) : base(b), key(std::forward<K>(k)), val(std::forward<V>(v)) {}
	};

	/// Node removed from underlying map
	struct Rem {
		Self* base;  ///< Modified map
		Key key;     ///< Key removed

		template<typename K>
		Rem(Self* b, K&& k) : base(b), key(std::forward<K>(k)) {}
	};

	/// Underlying storage
	union Data {
		char def;
		Base base;
		Ins ins;
		Rem rem;

		Data() : def('\0') {}
		~Data() {}
	} data;

	// Mode of node
	mutable Mode mode;

	/// get mutable reference as T
	template<typename T>
	T& as() { return reinterpret_cast<T&>(data); }

	/// get const reference as T
	template<typename T>
	const T& as() const { return reinterpret_cast<const T&>(data); }

	/// get rvalue reference as T
	template<typename T>
	T&& take_as() { return std::move(as<T>()); }

	/// initialize as T
	template<typename T, typename... Args>
	void init( Args&&... args ) {
		new( &as<T>() ) T { std::forward<Args>(args)... };
	}

	/// reset as current mode
	void reset() {
		switch( mode ) {
			case BASE:          as<Base>().~Base(); break;
			case REM:           as<Rem>().~Rem();   break;
			case INS: case UPD: as<Ins>().~Ins();   break;
			default: unreachable("invalid mode");
		}
	}

	persistent_map( Mode m, Base&& b ) : data(), mode(m) {
		assume(m == BASE, "invalid mode");
		init<Base>(std::move(b));
	}

	template<typename K, typename V>
	persistent_map( Mode m, const Self* o, K&& k, V&& v ) : data(), mode(m) {
		assume(m == INS || m == UPD, "invalid mode");
		init<Ins>(o, std::forward<K>(k), std::forward<V>(v));
	}

	template<typename K>
	persistent_map( Mode m, const Self* o, K&& k ) : data(), mode(m) {
		assume(m == REM, "invalid mode");
		init<Rem>(o, std::forward<K>(k));
	}

protected:
	void trace(const GC& gc) const {
		switch( mode ) {
			case BASE: {
				for (const auto& entry : as<Base>()) {
					gc << entry.first << entry.second;
				}
				return;
			}
			case REM: {
				const Rem& self = as<Rem>();
				gc << self.base << self.key;
				return;
			}
			case INS: case UPD: {
				const Ins& self = as<Ins>();
				gc << self.base << self.key << self.val;
				return;
			}
			default: unreachable("invalid mode");
		}
	}

public:
	using size_type = std::size_t;

	using iterator = typename Base::const_iterator;

	persistent_map() : data(), mode(BASE) { init<Base>(); }

	persistent_map( const Self& o ) = delete;

	Self& operator= ( const Self& o ) = delete;

	~persistent_map() { reset(); }

	/// reroot persistent map at current node
	void reroot() const {
		// recursive base case
		if ( mode == BASE ) return;
		
		// reroot base
		Self* mut_this = const_cast<Self*>(this);
		Self* base = ( mode == REM ) ? mut_this->as<Rem>().base : mut_this->as<Ins>().base;
		base->reroot();
		assume(base->mode == BASE, "reroot results in base");

		// take map out of base
		Base&& base_map = base->take_as<Base>();
		base->reset();

		// switch base to inverse of self and mutate base map
		switch ( mode ) {
			case REM: {
				Rem& self = mut_this->as<Rem>();
				auto it = base_map.find( self.key );
				assume( it != base_map.end(), "removed node must exist in base");

				base->init<Ins>( mut_this, std::move(self.key), std::move(it->second) );
				base->mode = INS;

				base_map.erase( it );
				break;
			}
			case INS: {
				Ins& self = mut_this->as<Ins>();

				base->init<Rem>( mut_this, self.key );
				base->mode = REM;

				base_map.emplace( std::move(self.key), std::move(self.val) );
				break;
			}
			case UPD: {
				Ins& self = mut_this->as<Ins>();
				auto it = base_map.find( self.key );
				assume( it != base_map.end(), "updated node must exist in base");

				base->init<Ins>( mut_this, std::move(self.key), std::move(it->second) );
				base->mode = UPD;

				it->second = std::move(self.val);
				break;
			}
			default: unreachable("invalid mode");
		}

		// set base map into self
		mut_this->reset();
		mut_this->init<Base>( std::move(base_map) );
		mode = BASE;
	}

private:
	/// Gets the base after rerooting at the current node
	const Base& rerooted() const {
		reroot();
		return as<Base>();
	}

public:
	/// true iff the map is empty
	bool empty() const { return rerooted().empty(); }

	/// Get number of entries in map
	size_type size() const { return rerooted().size(); }

	/// Get begin iterator for map; may be invalidated by calls to non-iteration functions 
	/// or functions on other maps in the same chain
	iterator begin() const { return rerooted().begin(); }

	/// Get end iterator for map; may be invalidated by calls to non-iteration functions 
	/// or functions on other maps in the same chain
	iterator end() const { return rerooted().end(); }

	/// Get underlying map iterator for value
	iterator find(const Key& k) const { return rerooted().find( k ); }

	/// Check if value is present
	size_type count(const Key& k) const { return rerooted().count( k ); }

	/// Get value; undefined behavior if not present
	const Val& get(const Key& k) const {
		const Base& self = rerooted();
		auto it = self.find( k );
		assume(it != self.end(), "get target not present");
		return it->second;
	}

	/// Get value; returns default if not present
	template<typename V>
	Val get_or_default(const Key& k, V&& d) const {
		const Base& self = rerooted();
		auto it = self.find( k );
		if ( it == self.end() ) return d;
		else return it->second;
	}

	/// Set value, storing new map in output variable
	template<typename K, typename V>
	Self* set(K&& k, V&& v) {
		reroot();
		assume(mode == BASE, "reroot results in base");

		// transfer map to new node
		Self* ret = new Self{ BASE, take_as<Base>() };
		reset();
		Base& base_map = ret->as<Base>();

		// check if this is update or insert
		auto it = base_map.find( k );
		if ( it == base_map.end() ) {
			// set self to REM node and insert into base
			init<Rem>( ret, k );
			mode = REM;

			base_map.emplace_hint( it, std::forward<K>(k), std::forward<V>(v) );
		} else {
			// set self to UPD node and modify base
			init<Ins>( ret, std::forward<K>(k), std::move(it->second) );
			mode = UPD;

			it->second = std::forward<V>(v);
		}

		return ret;
	}

	/// smart reference for indexing interface
	class Entry {
	friend persistent_map;
		Self* base;
		const Key& key;

	public:
		Entry(Self* b, const Key& k) : base(b), key(k) {}

		/// Gets the underlying map instance
		Self* get_base() const { return base; }

		/// Gets the key
		const Key& get_key() const { return key; }

		/// Checks if the key exists in the map
		bool exists() const { return base->count(key); }

		/// Gets the value for the key (if it exists)
		const Val& get() const { return base->get(key); }

		/// Cast version of get
		operator const Val& () const { return base->get(key); }

		/// Sets the value into the key; returns entry pointing to new set
		template<typename V>
		Entry& set(V&& v) {
			base = base->set(key, std::forward<V>(v));
			return *this;
		}

		/// Assignment version of set
		template<typename V>
		Entry& operator= (V&& v) {
			base = base->set(key, std::forward<V>(v));
			return *this;
		}

		/// Gets value or initializes to new value from args
		template<typename... Args>
		Entry& get_or(Args&&... args) {
			base = base->get_or(key, std::forward<Args>(args)...);
			return *this;
		}
	};

	/// Gets smart reference to slot with given key
	Entry operator[] (const Key& k) { return { this, k }; }

	/// Gets Entry for key, initializing from args if not present
	template<typename... Args>
	Entry get_or_insert(const Key& k, Args&&... args) {
		Base& base_map = rerooted();

		// check already present
		if ( base_map.count(k) ) return { this, k };

		// otherwise insert based on parameters
		base_map.emplace( k, Val{ std::forward<Args>(args)... } );
		Self* ret = new Self{ BASE, std::move(base_map) };

		// update self to point to new base
		reset();
		init<Rem>( ret, k );
		mode = REM;

		// return entry for new base
		return { ret, k };
	}

	/// Get node type
	Mode get_mode() const { return mode; }

	/// Get next version up the revision tree (self if base node)
	const Self* get_base() const {
		switch ( mode ) {
			case BASE:          return this;
			case REM:           return as<Rem>().base;
			case INS: case UPD: return as<Ins>().base;
			default: unreachable("invalid mode");
		}
	}

	/// Get key of revision node (undefined if called on base)
	const Key& get_key() const {
		switch ( mode ) {
			case REM:           return as<Rem>().key;
			case INS: case UPD: return as<Ins>().key;
			default: unreachable("invalid mode for get_key()");
		}
	}

	/// Get value of insert/update revision node (undefined otherwise)
	const Val& get_val() const {
		switch ( mode ) {
			case INS: case UPD: return as<Ins>().val;
			default: unreachable("invalid mode for get_val()");
		}
	}
};
