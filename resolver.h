#pragma once

#include "conversion.h"
#include "expr.h"
#include "interpretation.h"
#include "utility.h"

/// State-tracking class for resolving expressions
class Resolver {
	ConversionGraph conversions;  ///< Conversions between known types
	List<VarDecl, Raw>& vars;     ///< Dummy "variable" declarations
	
public:
	Resolver( ConversionGraph&& conversions, List<VarDecl, Raw>& vars )
		: conversions( std::move(conversions) ), vars( vars ) {}

	/// Resolve best interpretation of input expression
	Interpretation operator() ( Ref<Expr> expr );
};
