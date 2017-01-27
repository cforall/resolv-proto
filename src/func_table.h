#pragma once

#include <string>
#include <vector>

#include "ast/decl.h"
#include "flat_map.h"
#include "gc.h"

/// Functor to extract the number of parameters from a function declaration
struct ExtractNParams {
	unsigned operator() (const FuncDecl* f) { return f->params().size(); }
};

/// Functor to extract the name from a function declaration
struct ExtractName {
	const std::string& operator() (const FuncDecl* f) { return f->name(); }	
};

/// Backing storage for an unindexed set of functions
typedef std::vector<FuncDecl*> FuncList;

#ifdef RP_SORTED
	template<typename Key, typename Inner, typename Extract>
	using FuncMap = SortedFlatMap<Key, Inner, Extract>; 
#else
	template<typename Key, typename Inner, typename Extract>
	using FuncMap = FlatMap<Key, Inner, Extract>;
#endif

/// Backing storage for a set of function declarations with the same name, 
/// indexed by number of parameters
typedef FuncMap<unsigned, FuncList, ExtractNParams> FuncParamMap;

/// Backing storage for a set of function declarations, indexed by name
typedef FuncMap<std::string, FuncParamMap, ExtractName> FuncTable;

inline const GC& operator<< (const GC& gc, const FuncTable& funcs) {
	for ( const FuncDecl* obj : funcs ) {
		gc << obj;
	}
	return gc;
}
