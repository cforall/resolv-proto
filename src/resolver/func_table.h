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

/// Backing storage for a set of function declarations with the same name indexed by return type
#if defined RP_DIR_TD
	/// Backing storage for an unindexed set of functions
	using FuncList = std::vector<FuncDecl*>;
	/// Backing storage for an unindexed set of variables
	using VarList = std::vector<Decl*>;

	/// Wrapper for type_map with appropriate args for FlatMap
	template<typename, typename V>
	using TypeMap2 = TypeMap<V>;

	/// Functor to extract the return type from a function declaration
	struct ExtractReturn {
		const Type* operator() (const FuncDecl* d) { return d->returns(); }
	};

	/// Functor to extract the type from a declaration
	struct ExtractType {
		const Type* operator() (const Decl* d) { return d->type(); }
	};

	/// Backing storage for a set of function declarations with the same name indexed by 
	/// return type
	using FuncSubTable = FlatMap<const Type*, FuncList, ExtractReturn, TypeMap2>;

	/// Backing storage for a set of declarations with the same name indexed by type
	using VarSubTable = FlatMap<const Type*, VarList, ExtractType, TypeMap2>;

	/// Backing storage for a set of declarations, indexed by name and segregated by variable vs. function
	struct FuncTable {
		using Funcs = ScopedMap2<std::string, FuncSubTable>;
		using Vars = ScopedMap2<std::string, VarSubTable>;
		Funcs funcs;
		Vars vars;

		void beginScope() {
			funcs.beginScope();
			vars.beginScope();
		}

		void endScope() {
			funcs.endScope();
			vars.endScope();
		}
	};

	inline void addDecl(FuncTable& table, Decl* decl) {
		// add to function table if function
		if ( is<FuncDecl>(decl) ) {
			auto it = table.funcs.findAt( table.funcs.currentScope(), decl->name() );
			if ( it == table.funcs.end() ) {
				it = table.funcs.insert( decl->name(), FuncTable::Funcs::mapped_type{} ).first;
			}
			it->second.insert( as<FuncDecl>(decl) );
		}
		// always add to variable table
		auto it = table.vars.findAt( table.vars.currentScope(), decl->name() );
		if ( it == table.vars.end() ) {
			it = table.vars.insert( decl->name(), FuncTable::Vars::mapped_type{} ).first;
		}
		it->second.insert( decl );
	}

	inline const GC& operator<< (const GC& gc, const FuncTable& funcs) {
		for ( const auto& scope : funcs.vars ) {
			for ( const Decl* decl : scope.second ) {
				gc << decl;
			}
		}
		return gc;
	}
#elif defined RP_DIR_BU || defined RP_DIR_CO
	/// Backing storage for an unindexed set of functions
	using FuncList = std::vector<Decl*>;

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

	/// Backing storage for a set of function declarations, indexed by name
	using FuncTable = ScopedMap2<std::string, FuncSubTable>;

	inline void addDecl( FuncTable& funcs, Decl* decl ) {
		// insert function at current scope, creating function list for name if necessary
		auto it = funcs.findAt( funcs.currentScope(), decl->name() );
		if ( it == funcs.end() ) {
			it = funcs.insert( decl->name(), FuncTable::mapped_type{} ).first;
		}
		it->second.insert( decl );
	}

	inline const GC& operator<< (const GC& gc, const FuncTable& funcs) {
		for ( const auto& scope : funcs ) {
			for ( const Decl* decl : scope.second ) {
				gc << decl;
			}
		}
		return gc;
	}
#endif
