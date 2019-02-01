#pragma once

#include "env.h"

#include "ast/type.h"
#include "ast/type_visitor.h"

/// Checks if any of a list of forbidden variables occur in the expansion of a type in an 
/// environment.
template<template<typename> class Coll>
class OccursIn : public TypeVisitor<OccursIn<Coll>, bool> {
	const Coll<PolyType>& vars;
	const Env& env;

public:
	using Super = TypeVisitor<OccursIn<Coll>, bool>;
	using Super::visit;
	using Super::operator();

	OccursIn( const Coll<PolyType>& vars, const Env& env ) : vars(vars), env(env) {}

	bool visit( const PolyType* t, bool& r ) {
		// check forbidden variable list for occurance of t
		for ( const PolyType* v : vars ) {
			if ( *v == *t ) {
				r = true;
				return false;
			}
		}

		// recursively check substitution of t from environment
		ClassRef tr = env.findRef( t );
		if ( tr ) return visit( tr.get_bound(), r );

		// didn't find substitution
		return true;
	}
};
