#pragma once

#include "type.h"
#include "type_visitor.h"

/// Checks if a type contains any polymorphic sub-types
class IsPoly : public TypeVisitor<IsPoly, bool> {
public:
	using Super = TypeVisitor<IsPoly, bool>;
	using Super::visit;
	using Super::operator();

	bool visit( const PolyType*, bool& r ) {
		r = true;
		return false;
	}
};

static inline bool is_poly( const Type* t ) { return IsPoly{}( t ); }
