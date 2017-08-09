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
#include "data/mem.h"

bool Env::mergeBound( ClassRef& r, const Type* cbound ) {
	if ( cbound == nullptr ) return true;

	if ( r->bound == nullptr ) {
		if ( r.env != this ) { copyClass(r); }
		classes[r.ind].bound = cbound;
		++cost.vars;
		return true;
	} else {
		return *r->bound == *cbound;
		// TODO possibly account for safe/unsafe conversions here; would need cost information in 
		// environment.
	}
}

void Env::trace(const GC& gc) const {
	for ( const TypeClass& entry : classes ) {
		gc << entry.vars << entry.bound;
	}

	for ( const auto& assn : assns ) {
		gc << assn.first << assn.second;
	}
	gc << parent;
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

void writeClasses(std::ostream& out, const Env* env, bool printed = false,
	              std::unordered_set<const PolyType*>&& seen = {}) {
	for ( const TypeClass& c : env->getClasses() ) {
		for ( const PolyType* v : c.vars ) {
			// skip classes containing vars we've seen before
			if ( ! seen.insert( v ).second ) goto nextClass;
		}
		if ( printed ) { out << ", "; } else { printed = true; }
		out << c;
	nextClass:; }

	// recurse to parent if necessary
	if ( env->parent ) writeClasses(out, env->parent, printed, move(seen));
}

void writeAssertions(std::ostream& out, const Env* env, TypedExprTypeMutator<EnvSubstitutor>& sub,
                     std::unordered_set<const FuncDecl*>&& seen = {}) {
	for ( const auto& assn : env->getAssertions() ) {
		// skip seen assertions
		if ( ! seen.insert( assn.first ).second ) continue;
		// print assertion
		out << " | " << *assn.first << " => ";
		if ( assn.second ) { out << *sub(assn.second); } else { out << "???"; }
	}
	// recurse to parent if necessary
	if ( env->parent ) writeAssertions(out, env->parent, sub, move(seen));
}

std::ostream& Env::write(std::ostream& out) const {
	out << "{";
	writeClasses(out, this);
	TypedExprTypeMutator<EnvSubstitutor> sub{ EnvSubstitutor{ this } };
	writeAssertions(out, this, sub);
	return out << "}";
}