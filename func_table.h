#pragma once

#include <string>

#include "decl.h"
#include "flat_map.h"
#include "utility.h"

/// Functor to extract the number of parameters from a function declaration
struct ExtractNParams {
	unsigned operator() (const FuncDecl& f) { return f.params().size(); }
};

/// Functor to extract the name from a function declaration
struct ExtractName {
	const std::string& operator() (const FuncDecl& f) { return f.name(); }	
};

/// Backing storage for an unindexed set of functions
typedef List<FuncDecl, Raw> FuncList;

/// Backing storage for a set of function declarations with the same name, 
/// indexed by number of parameters
typedef FlatMap<unsigned, FuncList, ExtractNParams> FuncParamMap;

/// Backing storage for a set of function declarations, indexed by name
typedef SortedFlatMap<std::string, FuncParamMap, ExtractName> FuncTable;
