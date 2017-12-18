#pragma once

#include "env.h"

#include "ast/type.h"
#include "ast/type_visitor.h"
#include "data/list.h"

/// Checks if any of a list of forbidden variables occur in the expansion of a type in an 
/// environment.
class OccursIn : public TypeVisitor<OccursIn, bool> {
	const Env* env;
	const List<PolyType>& vars;

public:
	using Super = TypeVisitor<OccursIn, bool>;
	using Super::visit;
	using Super::operator();

	OccursIn( const List<PolyType>& vars ) : env(nullptr), vars(vars) {}
	OccursIn( const Env* env, const List<PolyType>& vars ) : env(env), vars(vars) {}

	bool visit( const PolyType* t, bool& r ) {
		// check forbidden variable list for occurance of t
		for ( const PolyType* v : vars ) {
			if ( *v == *t ) {
				r = true;
				return false;
			}
		}
		// recursively check substitution of t from environment
		ClassRef tr = findClass( env, t );
		if ( tr ) return visit( tr->bound, r );
		// didn't find substitution
		return true;
	}
};
