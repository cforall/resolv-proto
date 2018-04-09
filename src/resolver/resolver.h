#pragma once

#include <cstddef>
#include <functional>
#include <unordered_map>
#include <utility>
#include <vector>

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

// check on match_funcs for necessity of doing resolution
#if defined RP_RES_DEF
#define RP_ASSN_CHECK(expr) false
#else
#define RP_ASSN_CHECK(expr) resolve_mode.check_assertions && (expr)
#endif

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
using UnboundEffect = std::function<void(const Expr*, const std::vector<TypeClass>&)>;

/// Flags for interpretation mode
class ResolverMode {
#if defined RP_DIR_TD
	constexpr ResolverMode(bool ec, bool av, bool ca, bool t)
		: expand_conversions(ec), allow_void(av), check_assertions(ca), truncate(t) {}
#else
	constexpr ResolverMode(bool ec, bool av, bool ca)
		: expand_conversions(ec), allow_void(av), check_assertions(ca) {}
#endif
public:
	/// Should interpretations be expanded by their conversions? [true]
	const bool expand_conversions;
	/// Should interpretations with void type be allowed? [false]
	const bool allow_void;
	/// Should assertions be checked? [false]
	const bool check_assertions;
#if defined RP_DIR_TD
	// Should tuple types be truncated? [true]
	const bool truncate;
#endif

	/// Default flags
	constexpr ResolverMode() : expand_conversions(true), allow_void(false)
#if defined RP_RES_IMM
		, check_assertions(true) 
#else
		, check_assertions(false)
#endif
#if defined RP_DIR_TD
		, truncate(true)
#endif
		{}

	/// Flags for top-level resolution
	static constexpr ResolverMode top_level() {
#if defined RP_DIR_TD
		return { false, true, true, true };
#else
		return { false, true, true };
#endif
	}

	/// Get a integer code for this mode
	constexpr unsigned id() const {
		return 
			expand_conversions
			| (allow_void << 1)
			| (check_assertions << 2)
#if defined RP_DIR_TD
			| (truncate << 3)
#endif
			;
	}

	/// Turn off expand_conversions
	ResolverMode&& without_conversions() && {
		as_non_const(expand_conversions) = false; 
		return move(*this);
	}
	/// Conditionally turn on allow_void if t is void
	ResolverMode&& with_void_as( const Type* t ) && {
		as_non_const(allow_void) = is<VoidType>(t);
		return move(*this);
	}

#if defined RP_RES_IMM
	// Turn off check assertions
	ResolverMode&& without_assertions() && {
		as_non_const(check_assertions) = false;
		return move(*this);
	}

	// Conditionally turn off check assertions
	ResolverMode&& with_assertions_if( bool b ) && {
		as_non_const(check_assertions) = b;
		return move(*this);
	}
#endif

#if defined RP_DIR_TD
	// Turn off truncate
	ResolverMode&& without_truncation() && {
		as_non_const(truncate) = false;
		return move(*this);
	}
#endif
};

inline bool operator== ( const ResolverMode& a, const ResolverMode& b ) {
	return a.id() == b.id();
}

namespace std {
	template<> struct hash<ResolverMode> {
		typedef ResolverMode argument_type;
		typedef std::size_t result_type;
		result_type operator() ( const argument_type& t ) const { return t.id(); }
	};

	template<> struct hash< std::pair<const Expr*,ResolverMode> > {
		typedef std::pair<const Expr*,ResolverMode> argument_type;
		typedef std::size_t result_type;
		result_type operator() ( const argument_type& t ) const {
			return std::hash<const Expr*>{}( t.first ) ^ t.second.id();
		}
	};
}

#if defined RP_DIR_TD
/// argument cache; prevents re-calculation of shared subexpressions under same environment
class ArgCache {
	using ExprKey = std::pair<const Expr*, ResolverMode>;
	using KeyedMap = TypeMap< InterpretationList >;
	std::unordered_map<ExprKey, KeyedMap> tcache; // typed expression cache
	std::unordered_map<ExprKey, InterpretationList> ucache; // untyped expression cache

public:
	/// Looks up the interpretations with a given type in the cache, generating them if needed.
	/// The generation function takes ty and e as arguments and returns an InterpretationList
	template<typename F>
	InterpretationList& operator() ( const Type* ty, const Expr* e, ResolverMode mode, F gen ) {
		// search for expression
		ExprKey key{ e, mode };
		auto eit = tcache.find( key );
		if ( eit == tcache.end() ) {
			eit = tcache.emplace_hint( eit, key, KeyedMap{} );
		}

		// find appropriately typed return value
		KeyedMap& ecache = eit->second;
		auto it = ecache.find( ty );
		if ( it == ecache.end() ) {
			// build if not found
			it = ecache.insert( ty, gen( ty, e, mode ) ).first;
		}

		return it.get();
	}

	/// Looks up the untyped interpretations in the cache, generating them if needed.
	/// The generation function takes e as an argument, and returns an InterpretationList
	template<typename F>
	InterpretationList& operator() ( const Expr* e, ResolverMode mode, F gen ) {
		// search for expression
		ExprKey key{ e, mode };
		auto it = ucache.find( key );
		if ( it == ucache.end() ) {
			it = ucache.emplace_hint( it, key, gen( e, mode ) );
		}
		return it->second;
	}

	/// Clears the argument cache
	void clear() {
		tcache.clear();
		ucache.clear();
	}
};
#endif // RP_DIR_TD

/// State-tracking class for resolving expressions
class Resolver {
public:
	ConversionGraph& conversions;       ///< Conversions between known types
	FuncTable& funcs;                   ///< Known function declarations
	unsigned id_src;                    ///< Source of type variable IDs
	unsigned max_recursive_assertions;  ///< Maximum recursive assertion depth

#if defined RP_DIR_TD
	ArgCache cached;               ///< Cached expression resolutions
#endif
	
	InvalidEffect on_invalid;      ///< Effect to run on invalid interpretation
	AmbiguousEffect on_ambiguous;  ///< Effect to run on ambiguous interpretation
	UnboundEffect on_unbound;      ///< Effect to run on unbound type variables

	Resolver( ConversionGraph& conversions, FuncTable& funcs,
	          InvalidEffect on_invalid, AmbiguousEffect on_ambiguous, UnboundEffect on_unbound,
			  unsigned max_recursive_assertions = 5 )
		: conversions( conversions ), funcs( funcs ), id_src( 0 ), 
		  max_recursive_assertions( max_recursive_assertions ), 
#if defined RP_DIR_TD
		  cached(),
#endif
		  on_invalid( on_invalid ), on_ambiguous( on_ambiguous ), on_unbound( on_unbound ) {}

	/// Recursively resolve interpretations subject to `env`, expanding conversions if 
	/// not at the top level. May return ambiguous interpretations, but otherwise will 
	/// not return invalid interpretations.
	InterpretationList resolve( const Expr* expr, const Env& env, ResolverMode resolve_mode = {} );
	
	/// Resolves `expr` as `targetType`, subject to `env`. 
	/// Returns all interpretations (possibly ambiguous).
	InterpretationList resolveWithType( const Expr* expr, const Type* targetType, const Env& env );

	/// Resolve best interpretation of input expression
	/// Will return invalid interpretation and run appropriate effect if 
	/// resolution fails
	const Interpretation* operator() ( const Expr* expr );
};
