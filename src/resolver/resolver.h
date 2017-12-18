#pragma once

//#include <iostream>
//
#include <cstddef>
#include <functional>
#include <unordered_map>
#include <utility>

#include "conversion.h"
#include "cost.h"
#include "env.h"
#include "func_table.h"
#include "interpretation.h"
#include "type_map.h"

#include "ast/expr.h"
#include "ast/type.h"
#include "data/cast.h"
#include "data/mem.h"

/// Effect to run on invalid interpretation; argument is the expression which 
/// could not be resolved.
using InvalidEffect = std::function<void(const Expr*)>;

/// Effect to run on ambiguous interpretation; arguments are the expression which 
/// could not be resolved and an iterator pair representing the first ambiguous 
/// candidate and the iterator after the last (range is guaranteed to be non-empty).
using AmbiguousEffect = std::function<void(const Expr*, 
                                           List<Interpretation>::const_iterator, 
	                                       List<Interpretation>::const_iterator)>;

/// Effect to be run on unbound type variables; arguments are the expression which 
/// has unbound type variables and the unbound typeclasses.
using UnboundEffect = std::function<void(const Expr*, const List<TypeClass>&)>;

#ifdef RP_MODE_TD
/// argument cache; prevents re-calculation of shared subexpressions under same environment
class ArgCache {
	using KeyedMap = TypeMap< InterpretationList >;
	std::unordered_map<const Expr*, KeyedMap> tcache; // typed expression cache
	std::unordered_map<const Expr*, InterpretationList> ucache; // untyped expression cache

public:
	/// Looks up the interpretations with a given type in the cache, generating them if needed.
	/// The generation function takes ty and e as arguments and returns an InterpretationList
	template<typename F>
	InterpretationList& operator() ( const Type* ty, const Expr* e, F gen ) {
		// search for expression
		auto eit = tcache.find( e );
		if ( eit == tcache.end() ) {
			eit = tcache.emplace_hint( eit, e, KeyedMap{} );
		}

		// find appropriately typed return value
		KeyedMap& ecache = eit->second;
		auto it = ecache.find( ty );
		if ( it == ecache.end() ) {
			// build if not found
			it = ecache.insert( ty, gen( ty, e ) ).first;
//			std::cerr << 'm' << std::endl;
//		} else {
//			std::cerr << 'h' << std::endl;
		}

		return it.get();
	}

	/// Looks up the untyped interpretations in the cache, generating them if needed.
	/// The generation function takes e as an argument, and returns an InterpretationList
	template<typename F>
	InterpretationList& operator() ( const Expr* e, F gen ) {
		// search for expression
		auto it = ucache.find( e );
		if ( it == ucache.end() ) {
			it = ucache.emplace_hint( it, e, gen( e ) );
		}
		return it->second;
	}

	/// Clears the argument cache
	void clear() {
		tcache.clear();
		ucache.clear();
	}
};
#endif

/// State-tracking class for resolving expressions
class Resolver {
public:
	ConversionGraph& conversions;  ///< Conversions between known types
	FuncTable& funcs;              ///< Known function declarations
	unsigned id_src;               ///< Source of type variable IDs

#ifdef RP_MODE_TD
	ArgCache cached;               ///< Cached expression resolutions
#endif
	
	InvalidEffect on_invalid;      ///< Effect to run on invalid interpretation
	AmbiguousEffect on_ambiguous;  ///< Effect to run on ambiguous interpretation
	UnboundEffect on_unbound;      ///< Effect to run on unbound type variables

	Resolver( ConversionGraph& conversions, FuncTable& funcs,
	          InvalidEffect on_invalid, AmbiguousEffect on_ambiguous, UnboundEffect on_unbound )
		: conversions( conversions ), funcs( funcs ), id_src(0),
#ifdef RP_MODE_TD
		  cached(),
#endif
		  on_invalid( on_invalid ), on_ambiguous( on_ambiguous ), on_unbound( on_unbound ) {}

	/// Flags for interpretation mode
	class Mode {
		constexpr Mode(bool ec, bool av, bool ca)
			: expand_conversions(ec), allow_void(av), check_assertions(ca) {}
	public:
		/// Should interpretations be expanded by their conversions? [true]
		const bool expand_conversions;
		/// Should interpretations with void type be allowed? [false]
		const bool allow_void;
		/// Should assertions be checked? [false]
		const bool check_assertions;

		/// Default flags
		constexpr Mode() : expand_conversions(true), allow_void(false), check_assertions(false) {}

		/// Flags for top-level resolution
		static constexpr Mode top_level() { return { false, true, true }; }

		/// Turn off expand_conversions
		Mode& without_conversions() { as_non_const(expand_conversions) = false; return *this; }
		/// Turn on allow_void
		Mode& with_void() { as_non_const(allow_void) = true; return *this; }
		/// Conditionally turn on allow_void if t is void
		Mode& with_void_as( const Type* t ) {
			as_non_const(allow_void) = is<VoidType>(t); return *this;
		}
		/// Turn on check_assertions
		Mode& with_assertions() { as_non_const(check_assertions) = true; return *this; }
	};

	/// Recursively resolve interpretations subject to `env`, expanding conversions if 
	/// not at the top level. May return ambiguous interpretations, but otherwise will 
	/// not return invalid interpretations.
	InterpretationList resolve( const Expr* expr, const Env* env, Mode resolve_mode = {} );
	
	/// Resolves `expr` as `targetType`, subject to `env`. 
	/// Returns all interpretations (possibly ambiguous).
	InterpretationList resolveWithType( const Expr* expr, const Type* targetType, const Env* env );

	/// Resolve best interpretation of input expression
	/// Will return invalid interpretation and run appropriate effect if 
	/// resolution fails
	const Interpretation* operator() ( const Expr* expr );
};
