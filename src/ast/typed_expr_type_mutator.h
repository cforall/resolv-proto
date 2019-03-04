#pragma once

// Copyright (c) 2015 University of Waterloo
//
// The contents of this file are covered under the licence agreement in 
// the file "LICENCE" distributed with this repository.

#include "expr.h"
#include "mutator.h"
#include "type.h"
#include "type_mutator.h"
#include "typed_expr_mutator.h"

#include "data/cast.h"
#include "data/mem.h"

/// Produces a mutated copy of a typed expression, where all the types in the typed 
/// expression are mutated according to the wrapped type mutator
template<typename SubMutator>
class TypedExprTypeMutator : public TypedExprMutator<TypedExprTypeMutator<SubMutator>> {
	SubMutator m;

public:
	using Super = TypedExprMutator<TypedExprTypeMutator<SubMutator>>;
	using Super::visit;

	TypedExprTypeMutator( SubMutator&& m = SubMutator{} ) : m(move(m)) {}

	bool visit( const ValExpr* e, const TypedExpr*& r ) {
		const Type* ty = m( e->type() );
		if ( ty != e->type() ) {
			r = new ValExpr{ ty };
		}
		return true;
	}

	bool visit( const TruncateExpr* e, const TypedExpr*& r ) {
		const TypedExpr* arg = e->arg();
        if ( ! visit( e->arg(), arg ) ) return false;
		if ( ! arg ) {
			r = nullptr;
			return true;
		}
		const Type* ty = m( e->type() );
		if ( arg != e->arg() || ty != e->type() ) {
			r = new TruncateExpr{ arg, ty };
		}
		return true;
	}
};
