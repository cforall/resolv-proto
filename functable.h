#pragma once

#include <map>
#include <string>
#include <unordered_map>

#include "decl.h"
#include "utility.h"

/// Backing storage for an unindexed set of functions
typedef List<FuncDecl, Raw> FuncList;

/// Backing storage for a set of function declarations with the same name, 
/// indexed by number of parameters
typedef std::map<unsigned, FuncList> FuncMap

namespace std {
	/// Overload std::begin to return flat iterator for FuncMap
	Flat<FuncMap::const_iterator> begin( const FuncMap& m ) {
		auto it = m.begin();
		return it == m.end() ? flat_from_end( it ) : flat_from_valid( it );
	}
	
	/// Overload std::end to return flat iterator for FuncMap
	Flat<FuncMap::const_iterator> end( const FuncMap& m ) {
		return flat_from_end( m.end() );
	}
}

/// Backing storage for a set of function declarations
class FuncTable {
	friend const FuncMap* by_name(const FuncTable&);
	friend Flat< Flat<FuncMap::const_iterator> > begin(const FuncTable&);
	
	std::unordered_map<std::string, FuncMap> funcs;
	
	FuncList& find_or_insert( const FuncDecl& f ) {
		return funcs[ f.name() ][ f.params().size() ];
	}
public:
	/// Inserts a declaration into the table
	void insert( const FuncDecl& f ) {
		find_or_insert( f ).push_back( f );
	}
	
	/// Inserts a declaration into the table
	void insert( FuncDecl&& f ) {
		FuncList& fs = find_or_insert( f );
		fs.push_back( std::move(f) );
	}
};

namespace std {
	/// Overload std::begin to return flat iterator for FuncTable
	Flat< Flat<FuncMap::const_iterator> > begin( const FuncTable& t ) {
		auto it = begin( t.funcs );
		return it == end( t.funcs ) ? flat_from_end( it ) : flat_from_valid( it );
	}
	
	/// Overload std::end to return flat iterator for FuncTable
	Flat< Flat<FuncMap::const_iterator> > end( const FuncTable& t ) {
		return flat_from_end( end( t.funcs ) );
	}
}

/// Returns the functions from the table with the specified name
const FuncMap* by_name(const FuncTable& t, const std::string& name) {
	auto it = t.funcs.find( name );
	return it == t.funcs.end() ? nullptr : &(it->second);
}


