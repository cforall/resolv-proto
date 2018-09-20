#pragma once

#include <algorithm>
#include <unordered_map>
#include <vector>

#include "forall.h"
#include "type.h"
#include "type_mutator.h"

#include "data/list.h"
#include "data/range.h"

class FuncDecl;

/// Replaces unsourced polymorphic type variables by fresh variables bound to a specified source.
class ForallSubstitutor : public TypeMutator<ForallSubstitutor> {
	std::vector<const Forall*> ctx;  ///< Forall clauses being substituted into

public:
	using TypeMutator<ForallSubstitutor>::visit;
	using TypeMutator<ForallSubstitutor>::operator();

	ForallSubstitutor( const Forall* base ) : ctx{ base } {}

	bool visit( const PolyType* orig, const Type*& r ) {
		// Break early if not unbound
		if ( orig->id() != 0 ) return true;

		// Check for existing variable with given name in destination forall(s) and substitute
		for ( const Forall* forall : reversed(ctx) ) {
			if ( const PolyType* rep = forall->get( orig->name() ) ) {
				r = rep;
				return true;
			}
		}
		return true;
	}

	/// Substitute a function declaration's types according to the substitution map; 
	/// Uses src to produce new IDs for nested forall clauses
	FuncDecl* operator() ( const FuncDecl* d, unsigned& src );

	/// Substitute a list of types
	List<Type> operator() ( const List<Type>& ts ) {
		List<Type> rs;
		rs.reserve( ts.size() );
		for ( const Type* t : ts ) { rs.push_back( (*this)( t ) ); }
		return rs;
	}
};
