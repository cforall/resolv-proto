#pragma once

#include <unordered_map>

#include "forall.h"

#include "ast/type.h"
#include "ast/type_mutator.h"

class FuncDecl;

/// Replaces polymorphic type variables by fresh variables bound to a different forall clause.
class ForallSubstitutor : public TypeMutator<ForallSubstitutor> {
	/// Type variable substitution table
	using MemoTable = std::unordered_map<const PolyType*, const PolyType*>;
	/// Forall clause substitution table
	using ForallTable = std::unordered_map<const Forall*, const Forall*>;
	
	MemoTable memo;       ///< Ensures poly-types are only replaced once
	ForallTable foralls;  ///< Maps forall clauses to their replacements
public:
	using TypeMutator<ForallSubstitutor>::visit;
	using TypeMutator<ForallSubstitutor>::operator();

	ForallSubstitutor() = default;

	ForallSubstitutor( const Forall* from, const Forall* to ) : memo(), foralls() 
		{ foralls.emplace( from, to ); }

	bool visit( const PolyType* orig, const Type*& r ) {
		// Check memoization table for orig
		MemoTable::iterator it = memo.find( orig );
		if ( it != memo.end() ) {
			r = it->second;
			return true;
		}

		// Find substituting forall in table
		const Forall* from = orig->src();
		auto toit = foralls.find( from );
		if ( toit == foralls.end() ) {
			memo.emplace_hint( it, orig, orig );
			return true;
		}

		// Perform substitution
		const Forall* to = toit->second;
		it = memo.emplace_hint( it, orig, orig->clone_bound( to ) );
		r = it->second;
		return true;
	}

	/// Substitute a function declarations types according to the substitution map.
	FuncDecl* operator() ( const FuncDecl* d );
};
