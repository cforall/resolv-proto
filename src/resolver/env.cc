#include <cassert>
#include <string>

#include "env.h"

#include "cost.h"
#include "interpretation.h"
#include "env_substitutor.h"
#include "occurs_in.h"
#include "type_unifier.h"

#include "ast/decl.h"
#include "ast/type.h"
#include "ast/typed_expr_type_mutator.h"
#include "data/cast.h"
#include "data/gc.h"
#include "data/mem.h"

bool Env::mergeBound( ClassRef& r, const Type* cbound ) {
	if ( cbound == nullptr ) return true;

	if ( r->bound == nullptr ) {
		return bindType( r, cbound );
	} else {
		Cost::Element cost = 0;
		const PolyType* rt = r->vars[0];  // save variable in this class
		const Type* common = TypeUnifier{ this, cost }( r->bound, cbound );
		if ( ! common ) return false;
		r = findRef( rt );  // reset ref to restored class
		if ( r.env != this ) { copyClass( r ); }
		as_non_const(*r).bound = common;  // specialize r's bound to common type
		return true;
	}
}

Env* Env::make( ClassRef& r, const Type* sub ) {
	if ( OccursIn{ r->vars }( sub ) ) return nullptr;
	Env* env = new Env{ r };
	env->classes.front().bound = sub;
	return env;
}

bool Env::bindType( ClassRef& r, const Type* sub ) {
	if ( OccursIn{ this, r->vars }( sub ) ) return false;
	if ( r.env != this ) { copyClass( r ); }
	classes[ r.ind ].bound = sub;
	return true;
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