#pragma once

#include <ostream>

#include "data/list.h"
#include "data/mem.h"

#if defined RP_DEBUG && RP_DEBUG >= 1
	#define dbg_verify() verify()
#else
	#define dbg_verify()
#endif

class PolyType;
class Type;

/// Class of equivalent type variables, along with optional concrete bound type
struct TypeClass {
	List<PolyType> vars;  ///< Equivalent polymorphic types
	const Type* bound;    ///< Type which the class binds to

	TypeClass( List<PolyType>&& vars = List<PolyType>{}, const Type* bound = nullptr )
		: vars( move(vars) ), bound(bound) {}
};

std::ostream& operator<< (std::ostream&, const TypeClass&);

#if defined RP_ENV_PER
#include "env-per.h"
#elif defined RP_ENV_BAS
#include "env-bas.h"
#else
#include "env-iti.h"
#endif
