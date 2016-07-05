#pragma once

#include "cost.h"
#include "decl.h"
#include "type.h"
#include "utility.h"

/// Typed interpretation of an expression
struct Interpretation {
	Ref<Decl> decl;                       /// Base declaration for interpretation (nullptr for no valid interpretation)
	Ref<Conversion> cast;                 /// Conversion applied to decl (nullptr for no conversion)
	Ref<Type> ty;                         /// Type of interpretation (nullptr for no valid interpretation)
	Cost cost;                            /// Cost of interpretation
	List<Interpretation, ByShared> subs;  /// Subexpression interpretations
	
	Interpretation() = default;
	Interpretation(const Interpretation&) = default;
	Interpretation(Interpretation&&) = default;
	Interpretation& operator= (const Interpretation&) = default;
	Interpretation& operator= (Interpretation&&) = default;
	
	
	
	/// true iff the interpretation is ambiguous
	bool is_ambiguous() { return decl == nullptr; }
	
	/// true iff the interpretation is unambiguous and has a valid base
	bool is_valid() { return type != nullptr && decl != nullptr; }
	
	/// Returns a fresh invalid interpretation
	static Interpretation make_invalid() { return Interpretation{}; }
};
