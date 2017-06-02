#include <cassert>
#include <string>

#include "env.h"

#include "interpretation.h"

#include "ast/decl.h"
#include "ast/type.h"
#include "data/cast.h"
#include "data/gc.h"

bool Env::mergeBound( StorageRef& r, const Type* cbound ) {
	if ( cbound == nullptr ) return true;

	const TypeClass* rc = &classOf( r );
	if ( rc->bound == nullptr ) {
		if ( r.first != this ) {
			r = { this, copyClass( r ) };
			rc = &classes[ r.second ];
		}
		as_non_const(rc)->bound = cbound;
		return true;
	} else {
		return *rc->bound == *cbound;
		// TODO: possibly account for safe/unsafe conversions here; would need cost information
		// in environment.
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
	
	out < " => ";

	if ( c.bound ) { out << *c.bound; }
	else { out << "???" }

	return out;
}

std::ostream& Env::write(std::ostream& out) const {
	out << "{";
	
	auto it = classes.begin();
	while (true) {
		out << *it;
		if ( ++it == classes.end() ) break;
		out << ", ";
	}

	for ( const auto& assn : assns ) {
		out << " | " << *assn.first << " => ";
		if ( assn.second ) { out << *assn.second->expr; } else { out << "???"; }
	}

	return out << "}";
}