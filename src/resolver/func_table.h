#pragma once

#include <string>
#include <vector>

#include "type_map.h"

#include "ast/decl.h"
#include "ast/type.h"
#include "data/cast.h"
#include "data/debug.h"
#include "data/flat_map.h"
#include "data/gc.h"
#include "data/scoped_map.h"
#include "data/std_wrappers.h"

/// Map from key type to function declarations indexed by it
#if defined RP_SORTED
	template<typename Key, typename Inner, typename Extract>
	using FuncMap = FlatMap<Key, Inner, Extract, std_map>;

	template<typename Key, typename Value>
	using ScopedMap2 = ScopedMap<Key, Value, std_map>;
#else
	template<typename Key, typename Inner, typename Extract>
	using FuncMap = FlatMap<Key, Inner, Extract, std_unordered_map>;

	template<typename Key, typename Value>
	using ScopedMap2 = ScopedMap<Key, Value, std_unordered_map>;
#endif

/// Backing storage for an unindexed set of functions
using FuncList = std::vector<Decl*>;

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
	/// Functor to extract the number of parameters from a declaration
	struct ExtractNParams {
		unsigned operator() (const Decl* d) {
			auto did = typeof(d);
			if ( did == typeof<FuncDecl>() ) return as<FuncDecl>(d)->params().size();
			if ( did == typeof<VarDecl>() ) return (unsigned)-1;
			
			unreachable("invalid decl type");
			return (unsigned)-2;
		}
	};

	/// Backing storage for a set of function declarations with the same name indexed by 
	/// number of parameters
	using FuncSubTable = FuncMap<unsigned, FuncList, ExtractNParams>;
#endif

/// Backing storage for a set of function declarations, indexed by name
using FuncTable = ScopedMap2<std::string, FuncSubTable>;

inline const GC& operator<< (const GC& gc, const FuncTable& funcs) {
	for ( const auto& scope : funcs ) {
		for ( const Decl* decl : scope.second ) {
			gc << decl;
		}
	}
	return gc;
}
