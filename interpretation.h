#pragma once

#include "cost.h"
#include "decl.h"
#include "utility.h"

/// Typed interpretation of an expression
struct Interpretation {
	Ref<Decl> decl;                       /// Base declaration for interpretation
	Cost cost;                            /// Cost of interpretation
	List<Interpretation, ByShared> subs;  /// Subexpression interpretations
};
