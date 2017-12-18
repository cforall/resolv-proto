#pragma once

#include "cost.h"
#include "env.h"

#include "ast/type.h"
#include "ast/type_mutator.h"
#include "data/mem.h"

/// Replaces polymorphic type variables by their substitution in an environment.
class EnvSubstitutor : public TypeMutator<EnvSubstitutor> {
	const Env* env;
public:
	using Super = TypeMutator<EnvSubstitutor>;
	using Super::visit;

	EnvSubstitutor( const Env* env ) : env(env) {}

	bool visit( const PolyType* orig, const Type*& r ) {
		r = replace( env, orig );
		return true;
	}
};

/// Replaces polymorphic type variables by their subsitution in an environment, tracking 
/// substitution cost and polymorphic nature of returned type
class CostedSubstitutor : public TypeMutator<CostedSubstitutor> {
	const Env* env;
	Cost::Element& cost;
	bool poly;
public:
	using Super = TypeMutator<CostedSubstitutor>;
	using Super::visit;

	CostedSubstitutor( const Env* env, Cost::Element& cost ) : env(env), cost(cost), poly(false) {}

	bool visit( const PolyType* orig, const Type*& r ) {
		if ( ! env ) {
			poly = true;
			return false;
		}

		// check for substitution, note polymorphic nature if none
		ClassRef c = env->findRef( orig );
		if ( !( c && c->bound ) ) {
			poly = true;
			return true;
		}

		// count cost of substitution and recurse until substitution stops
		++cost;
		r = c->bound;
		while (true) {
			const Type* sub = r;
			visit( sub, r );
			if ( sub == r ) return true;
		}
	}

	bool is_poly() const { return poly; }
};
