#pragma once

#include <functional>

#include "conversion.h"
#include "cost.h"
#include "env.h"
#include "func_table.h"
#include "interpretation.h"

#include "ast/expr.h"
#include "ast/type.h"
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

/// State-tracking class for resolving expressions
class Resolver {
	ConversionGraph& conversions;  ///< Conversions between known types
	FuncTable& funcs;              ///< Known function declarations
	
	/// Effect to run on invalid interpretation
	InvalidEffect on_invalid;
	/// Effect to run on ambiguous interpretation
	AmbiguousEffect on_ambiguous;
	/// Effect to run on unbound type variables
	UnboundEffect on_unbound;
	
public:
	unsigned id_src;               ///< Source of type variable IDs

	Resolver( ConversionGraph& conversions, FuncTable& funcs,
	          InvalidEffect on_invalid, AmbiguousEffect on_ambiguous, UnboundEffect on_unbound )
		: conversions( conversions ), funcs( funcs ),
		  on_invalid( on_invalid ), on_ambiguous( on_ambiguous ), on_unbound( on_unbound ), 
		  id_src(0) {}
	
	/// Mode for type conversions
	enum Mode {
		NO_CONVERSIONS,  ///< Do not expand interpretations or allow void interpretations
		TOP_LEVEL,       ///< Do not expand interpretations; allow void interpretations
		ALL_NON_VOID     ///< Expand interpretations to all non-void types
	};

	/// Recursively resolve interpretations subject to `env`, expanding conversions if 
	/// not at the top level. May return ambiguous interpretations, but otherwise will 
	/// not return invalid interpretations.
	InterpretationList resolve( const Expr* expr, const Env* env, 
	                            Mode resolve_mode = ALL_NON_VOID );
	
	/// Resolves `expr` as `targetType`, subject to `env`. 
	/// Returns all interpretations (possibly ambiguous).
	InterpretationList resolveWithType( const Expr* expr, const Type* targetType, 
	                                    const Env* env );

	/// Resolve best interpretation of input expression
	/// Will return invalid interpretation and run appropriate effect if 
	/// resolution fails
	const Interpretation* operator() ( const Expr* expr );
};
