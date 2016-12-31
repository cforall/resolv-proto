#pragma once

#include "binding.h"
#include "type.h"
#include "type_mutator.h"
#include "visitor.h"

/// Replaces polymorphic type variables in a return type by either their substitution 
/// or a branded polymorphic type variable.
class BindingSubMutator : public TypeMutator {
	const TypeBinding& binding;
public:
	BindingSubMutator( const TypeBinding& binding ) : binding(binding) {}

	Visit visit( const PolyType* orig, const Type*& r ) override {
		const Type* sub = binding[ orig->name() ];
		if ( sub ) { r = sub; }
		else if ( orig->src() != &binding ) { r = orig->clone_bound( &binding ); }
		return Visit::CONT;
	}
};