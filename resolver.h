#pragma once

#include <functional>

#include "conversion.h"
#include "expr.h"
#include "func_table.h"
#include "interpretation.h"

/// Effect to run on invalid interpretation; argument is the expression which 
/// could not be resolved
typedef std::function<void(const Expr*)> InvalidEffect;

/// Effect to run on ambiguous interpretation; arguments are the expression which 
/// could not be resolved and an iterator pair representing the first ambiguous 
/// candidate and the iterator after the last (range is guaranteed to be non-empty)
typedef std::function<void(const Expr*,
                           InterpretationList::iterator, 
	                       InterpretationList::iterator)> AmbiguousEffect;  

/// State-tracking class for resolving expressions
class Resolver {
	ConversionGraph& conversions;  ///< Conversions between known types
	FuncTable& funcs;              ///< Known function declarations
	
	/// Effect to run on invalid interpretation
	InvalidEffect on_invalid;
	/// Effect to run on ambiguous interpretation
	AmbiguousEffect on_ambiguous;
	
	/// Recursively resolve interpretations, expanding conversions if not at the 
	/// top level.
	/// May return ambiguous interpretations, but otherwise will not return 
	/// invalid interpretations.
	InterpretationList resolve( const Expr* expr, bool topLevel = false );
	
public:
	Resolver( ConversionGraph& conversions, FuncTable& funcs,
	          InvalidEffect on_invalid, AmbiguousEffect on_ambiguous )
		: conversions( conversions ), funcs( funcs ),
		  on_invalid( on_invalid ), on_ambiguous( on_ambiguous ) {}

	/// Resolve best interpretation of input expression
	/// Will return invalid interpretation and run appropriate effect if 
	/// resolution fails
	const Interpretation* operator() ( const Expr* expr );
};
