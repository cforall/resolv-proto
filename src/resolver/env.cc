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
#include "data/persistent_map.h"

bool Env::occursIn( const Env* env, const List<PolyType>& vars, const Type* t ) {
	return OccursIn{ env, vars }( t );
}

bool Env::occursIn( const List<PolyType>& vars, const Type* t ) {
	return OccursIn{ vars }( t );
}

bool Env::mergeBound( ClassRef& r, const Type* cbound ) {
	dbg_verify();
	if ( cbound == nullptr ) return true;  // trivial if no type to bind to

	if ( r->bound == nullptr ) {  // bind if no type in target class
		return bindType( r, cbound );
	} else if ( r->bound == cbound ) {  // shortcut for easy case
		return true;
	} else {  // attempt to structurally bind r->bound to cbound
		Cost::Element cost = 0;
		const PolyType* rt = r->vars[0];  // save variable in this class
		TypeUnifier tu{ this, cost };
		const Type* common = tu( r->bound, cbound );
		if ( ! common ) return false;
		r = r.env->findRef( rt );  // reset ref to restored class
		if ( r.env != this ) { copyClass( r ); }
		classes[ r.ind ].bound = common;  // specialize r's bound to common type
		dbg_verify();
		return true;
	}
}

#if defined RP_DEBUG && RP_DEBUG >= 1
#include <cassert>

void Env::verify() const {
	for ( unsigned ci = 0; ci < classes.size(); ++ci ) {
		const TypeClass& tc = classes[ci];

		// check variable uniqueness
		for ( unsigned vi = 0; vi < tc.vars.size(); ++vi ) {
			const PolyType* v = tc.vars[vi];

			// check variable is properly bound in cache
			const ClassRef& vr = bindings.at( v );
			assert( vr.env == this && vr.ind == ci );

			// check variables are unique in a class
			for ( unsigned vj = vi + 1; vj < tc.vars.size(); ++vj ) {
				assert(v != tc.vars[vj]);
			}

			// check variables are unique in an environment
			for ( unsigned cj = ci + 1; cj < classes.size(); ++cj ) {
				for ( const PolyType* u : classes[cj].vars ) {
					assert(v != u);
				}
			}
		}

		// check occurs check not subverted
		if ( tc.bound ) {
			assert( ! occursIn( this, tc.vars, tc.bound ) );
		}
	}

	// check cached bindings are still valid
	for ( const auto& b : bindings ) {
		assert( b.second.ind < b.second.env->classes.size() );
		const TypeClass& bc = *b.second;
		assert( std::count( bc.vars.begin(), bc.vars.end(), b.first ) );
	}

	// same for parent
	if ( parent ) parent->verify();
}
#endif

void Env::trace(const GC& gc) const {
	for ( const TypeClass& entry : classes ) {
		gc << entry.vars << entry.bound;
	}

	gc << assns << parent;
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