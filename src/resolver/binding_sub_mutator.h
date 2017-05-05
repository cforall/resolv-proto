#pragma once

#include "binding.h"
#include "environment.h"

#include "ast/type.h"
#include "ast/type_mutator.h"
#include "data/cow.h"

/// Replaces polymorphic type variables in a return type by either their substitution 
/// or a branded polymorphic type variable.
class BindingSubMutator : public TypeMutator<BindingSubMutator> {
	const TypeBinding& binding;
public:
	using TypeMutator<BindingSubMutator>::visit;

	BindingSubMutator( const TypeBinding& binding ) : binding(binding) {}

	bool visit( const PolyType* orig, const Type*& r ) {
		const Type* sub = binding[ orig->name() ];
		if ( sub ) { r = sub; }
		else if ( orig->src() != &binding ) { r = orig->clone_bound( &binding ); }
		return true;
	}
};

/// Replaces polymorphic type variables in a return type by either their substitution 
/// in binding or env, or a branded polymorphic type variable.
class BindingEnvSubMutator : public TypeMutator<BindingEnvSubMutator> {
	const TypeBinding& binding;
	const cow_ptr<Environment>& env;
public:
	using TypeMutator<BindingEnvSubMutator>::visit;

	BindingEnvSubMutator( const TypeBinding& binding, const cow_ptr<Environment>& env )
		: binding(binding), env(env) {}

	bool visit( const PolyType* orig, const Type*& r ) {
		const Type* sub = binding[ orig->name() ];
		if ( sub ) { r = sub; return true; }
		sub = replace( env, orig );
		if ( sub != orig ) { r = sub; }
		else if ( orig->src() != &binding ) { r = orig->clone_bound( &binding ); }
		return true;
	}
};