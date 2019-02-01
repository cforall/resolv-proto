#include <string>

#include "env.h"

#include "cost.h"
#include "interpretation.h"
#include "env_substitutor.h"
#include "type_unifier.h"

#include "ast/decl.h"
#include "ast/type.h"
#include "ast/typed_expr_type_mutator.h"
#include "data/cast.h"
#include "data/debug.h"
#include "data/gc.h"
#include "data/mem.h"
#include "data/persistent_map.h"

#include "env-common.cc"

bool Env::mergeBound( ClassRef& r, const Type* cbound ) {
	assume(r.env == this, "invalid environment reference");
	dbg_verify();

	if ( cbound == nullptr ) return true; // trivial if no type to bind to

	const Type* rbound = r.get_bound();
	if ( rbound == nullptr ) {  // bind if no type in target class
		return bindType( r, cbound );
	} else if ( rbound == cbound ) {  // shortcut easy case
		return true;
	} else {  // attempt to structurally bind rbound to cbound
		// find common bound, failing if none
		Cost::Element cost = 0;
		TypeUnifier tu{ *this, cost };
		const Type* common = tu( rbound, cbound );
		if ( ! common ) return false;

		// bind class to common bound
		r.update_root();  // may have been merges in TypeUnifier
		bindings = bindings->set( r.get_root(), common );
		
		dbg_verify();
		return true;
	}
}

#if defined RP_DEBUG && RP_DEBUG >= 1
#include <cassert>

void Env::verify() const {
	// check all type variables have a root which is bound
	for ( const auto& v : *classes ) {
		assert( bindings->count( classes->find( v.first ) ) );
	}

	for ( const auto& b : *bindings ) {
		// check all bindings correspond to type variables
		assert( classes->count( b.first ) );

		List<PolyType> vars;
		classes->find_class( b.first, std::back_inserter( vars ) );

		// check that said variable is the root of its entire class
		for ( const PolyType* v : vars ) {
			assert( b.first == classes->find( v ) );
		}

		// check occurs check not subverted
		assert( ! occursIn( vars, b.second ) );
	}
}
#endif

const GC& operator<< (const GC& gc, const Env& env) {
	return gc << env.classes << env.bindings << env.assns;
}

std::ostream& Env::write(std::ostream& out) const {
	out << "{";
	// print classes
	bool printed = false;
	for ( const auto& b : *bindings ) {
		if ( printed ) { out << ", "; } else { printed = true; }
		out << *findRef( b.first );
	}
	// print assertions
	TypedExprTypeMutator<EnvSubstitutor> sub{ EnvSubstitutor{ *this } };
	for ( const auto& a : *assns ) {
		out << " | " << *a.first << " => ";
		if ( a.second ) { out << *sub(a.second); } else { out << "???"; }
	}
	return out << "}";
}