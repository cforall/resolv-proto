#pragma once

#include <algorithm>
#include <unordered_map>

#include "forall.h"
#include "type.h"
#include "type_mutator.h"

class FuncDecl;

/// Replaces polymorphic type variables by fresh variables bound to a different forall clause.
class ForallSubstitutor : public TypeMutator<ForallSubstitutor> {
	/// Type variable substitution table
	using MemoTable = std::unordered_map<const PolyType*, const PolyType*>;
	/// Forall clause substitution table
	using ForallTable = std::unordered_map<const Forall*, Forall*>;
	
	MemoTable memo;       ///< Ensures poly-types are only replaced once
	ForallTable foralls;  ///< Maps forall clauses to their replacements
public:
	using TypeMutator<ForallSubstitutor>::visit;
	using TypeMutator<ForallSubstitutor>::operator();

	ForallSubstitutor() = default;

	ForallSubstitutor( const Forall* from, Forall* to ) : memo(), foralls() 
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

		// Check for existing variable with given name in destination forall and substitute
		const PolyType* rep = toit->second->add( orig->name() );
		it = memo.emplace_hint( it, orig, rep );
		r = it->second;
		return true;
	}

	/// Substitute a function declarations types according to the substitution map.
	FuncDecl* operator() ( const FuncDecl* d );
};
