#pragma once

#include "binding.h"
#include "type.h"
#include "type_mutator.h"

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