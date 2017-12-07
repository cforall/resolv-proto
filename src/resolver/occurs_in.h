#pragma once

#include "env.h"

#include "ast/type.h"
#include "ast/type_visitor.h"
#include "data/list.h"

class OccursIn : public TypeVisitor<OccursIn, bool> {
	const Env* env;
	const List<PolyType>& vars;

public:
	using Parent = TypeVisitor<OccursIn, bool>;
	using Parent::visit;
	using Parent::operator();

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
