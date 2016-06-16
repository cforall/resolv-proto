#pragma once

#include "cost.h"
#include "expr.h"
#include "utility.h"

/// Typed interpretation of an expression
struct Interpretation {
	Ref<Decl> expr;                   /// Base declaration for interpretation
	Cost cost;                        /// Cost of interpretation
	SharedList<Interpretation> subs;  /// Subexpression interpretations
};
