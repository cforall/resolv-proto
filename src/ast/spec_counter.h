#pragma once

// Copyright (c) 2015 University of Waterloo
//
// The contents of this file are covered under the licence agreement in 
// the file "LICENCE" distributed with this repository.

#include "type.h"
#include "type_visitor.h"

#include "data/list.h"
#include "data/option.h"
#include "resolver/cost.h"

class SpecCounter : public TypeVisitor<SpecCounter, option<Cost::Element>> {
	void updateCount( const List<Type>& ts, option<Cost::Element>& r ) {
		for ( const Type* tt : ts ) {
			option<Cost::Element> rr;
			visit( tt, rr );
			if ( rr && ( ! r || *rr < *r - 1 ) ) { r = *rr + 1; }
		}
	}

public:
	using Super = TypeVisitor<SpecCounter, option<Cost::Element>>;
	using Super::visit;
	using Super::operator();

	bool visit( const NamedType* t, option<Cost::Element>& r ) {
		updateCount( t->params(), r );
		return true;
	}
	
	bool visit( const PolyType*, option<Cost::Element>& r ) {
		r = 0;
		return true;
	}

	bool visit( const TupleType* t, option<Cost::Element>& r ) {
		updateCount( t->types(), r );
		return true;
	}

	bool visit( const FuncType* t, option<Cost::Element>& r ) {
		visit( t->returns(), r );
		updateCount( t->params(), r );
		return true;
	}
};
