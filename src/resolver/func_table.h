#pragma once

#include <string>
#include <vector>

#include "type_map.h"

#include "ast/decl.h"
#include "ast/type.h"
#include "data/flat_map.h"
#include "data/gc.h"

/// Map from key type to function declarations indexed by it
#if defined RP_SORTED
	template<typename Key, typename Inner, typename Extract>
	using FuncMap = SortedFlatMap<Key, Inner, Extract>; 
#else
	template<typename Key, typename Inner, typename Extract>
	using FuncMap = FlatMap<Key, Inner, Extract>;
#endif

/// Backing storage for an unindexed set of functions
using FuncList = std::vector<FuncDecl*>;

/// Backing storage for a set of function declarations with the same name indexed by return type
#if defined RP_DIR_TD
	/// Wrapper for type_map with appropriate args for FlatMap
	template<typename, typename V>
	using TypeMap2 = TypeMap<V>;

	/// Functor to extract the return type from a function declaration
	struct ExtractReturn {
		const Type* operator() (const FuncDecl* f) { return f->returns(); }
	};

	/// Backing storage for a set of function declarations with the same name indexed by 
	/// return type
	using FuncSubTable = FlatMap<const Type*, FuncList, ExtractReturn, TypeMap2>;
#elif defined RP_DIR_BU || defined RP_DIR_CO
	/// Functor to extract the number of parameters from a function declaration
	struct ExtractNParams {
		unsigned operator() (const FuncDecl* f) { return f->params().size(); }
	};

	/// Backing storage for a set of function declarations with the same name indexed by 
	/// number of parameters
	using FuncSubTable = FuncMap<unsigned, FuncList, ExtractNParams>;
#endif

/// Functor to extract the name from a function declaration
struct ExtractName {
	const std::string& operator() (const FuncDecl* f) { return f->name(); }	
};

/// Backing storage for a set of function declarations, indexed by name
using FuncTable = FuncMap<std::string, FuncSubTable, ExtractName>;

inline const GC& operator<< (const GC& gc, const FuncTable& funcs) {
	for ( const FuncDecl* obj : funcs ) {
		gc << obj;
	}
	return gc;
}
