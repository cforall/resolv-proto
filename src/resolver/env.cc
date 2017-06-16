#include <cassert>
#include <string>

#include "env.h"

#include "interpretation.h"
#include "env_substitutor.h"

#include "ast/decl.h"
#include "ast/type.h"
#include "ast/typed_expr_type_mutator.h"
#include "data/cast.h"
#include "data/gc.h"

bool Env::mergeBound( ClassRef& r, const Type* cbound ) {
	if ( cbound == nullptr ) return true;

	const TypeClass* rc = &*r;
	if ( rc->bound == nullptr ) {
		if ( r.env != this ) {
			r = { this, copyClass( r ) };
			rc = &classes[ r.ind ];
		}
		as_non_const(rc)->bound = cbound;
		return true;
	} else {
		return *rc->bound == *cbound;
		// TODO: possibly account for safe/unsafe conversions here; would need cost 
		// information in environment.
	}
}

void Env::trace(const GC& gc) const {
	gc << parent;
	for ( const TypeClass& entry : classes ) {
		gc << entry.vars << entry.bound;
	}

	for ( const auto& assn : assns ) {
		gc << assn.first << assn.second;
	}
}

std::ostream& operator<< (std::ostream& out, const TypeClass& c) {
	if ( c.vars.size() == 1 ) {
		out << *c.vars.front();
	} else {
		out << '[';
		auto it = c.vars.begin();
		while (true) {
			out << **it;
			if ( ++it == c.vars.end() ) break;
			out << ", ";
		}
		out << ']';
	}
	
	out << " => ";

	if ( c.bound ) { out << *c.bound; }
	else { out << "???"; }

	return out;
}

std::ostream& Env::write(std::ostream& out) const {
	out << "{";
	
	// TODO include parents?

	auto it = classes.begin();
	while (true) {
		out << *it;
		if ( ++it == classes.end() ) break;
		out << ", ";
	}

	TypedExprTypeMutator<EnvSubstitutor> sub{ EnvSubstitutor{ this } };
	for ( const auto& assn : assns ) {
		out << " | " << *assn.first << " => ";
		if ( assn.second ) { out << *sub(assn.second); } else { out << "???"; }
	}

	return out << "}";
}