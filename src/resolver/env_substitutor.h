#pragma once

#include "env.h"

#include "ast/type.h"
#include "ast/type_mutator.h"
#include "data/mem.h"

/// Replaces polymorphic type variables by their substitution according to an environment.
class EnvSubstitutor : public TypeMutator<EnvSubstitutor> {
	const Env* env;
public:
	using TypeMutator<EnvSubstitutor>::visit;

	EnvSubstitutor( const Env* env ) : env(env) {}

	bool visit( const PolyType* orig, const Type*& r ) {
		r = replace( env, orig );
		return true;
	}
};
