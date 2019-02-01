#include "env.h"

#include "cost.h"
#include "env_substitutor.h"
#include "occurs_in.h"
#include "type_unifier.h"

#include "ast/typed_expr_type_mutator.h"
#include "data/debug.h"
#include "data/gc.h"
#include "data/mem.h"

#if defined RP_DEBUG && RP_DEBUG >= 1
void Env::verify() const {
	for ( unsigned ci = 0; ci < classes.size(); ++ci ) {
		const TypeClass& tc = classes[ci];

		for ( auto vi = tc.vars.begin(); vi != tc.vars.end(); ++vi ) {
			const PolyType* v = *vi;

			auto vj = vi;
			for ( ++vj; vj != tc.vars.end(); ++vj ) {
				assume( v != *vj, "variable is unique in type class" );
			}

			for ( unsigned cj = ci + 1; cj < classes.size(); ++cj ) {
				for ( const PolyType* u : classes[cj].vars ) {
					assume( v != u, "variable is unique in environment" );
				}
			}
		}

		if ( tc.bound ) {
			assume( ! occursIn( tc.vars, tc.bound ), "occurs check not subverted" );
		}
	}
}
#endif

bool Env::occursIn( const Set<PolyType>& vars, const Type* t ) const {
	return OccursIn<Set>{ vars, *this }( t );
}

bool Env::mergeBound( ClassRef& r, const Type* cbound ) {
	dbg_verify();
	if ( cbound == nullptr ) return true; // trivial if no type to bind to

	if ( r->bound == nullptr ) {  // bind if no type in target class
		return bindType( r, cbound );
	} else if ( r->bound == cbound ) {  // shortcut for easy case
		return true;
	} else {  // attempt to structurally bind r->bound to cbound
		Cost::Element cost = 0;
		const PolyType* rt = *r->vars.begin();  // save variable to reaquire
		TypeUnifier tu{ *this, cost };
		const Type* common = tu( r->bound, cbound );
		if ( ! common ) return false;
		r = findRef( rt );  // reset ref to restored class
		as_non_const(*r).bound = common;  // specialize bound
		dbg_verify();
		return true;
	}
}

bool Env::mergeClasses( ClassRef& r, ClassRef s ) {
	const PolyType* st = *s->vars.begin();

	// ensure bounds match
	if ( ! mergeBound( r, s->bound ) ) return false;

	// find r, s after mergeBound
	TypeClass& rc = as_non_const(*r);
	s = findRef( st );
	const TypeClass& sc = *s;

	// ensure occurs check not violated
	if ( occursIn( s->vars, rc.bound ) ) return false;

	// merging two existing local classes; guaranteed vars don't overlap
	rc.vars.insert( sc.vars.begin(), sc.vars.end() );
	
	// remove s from local class list
	std::size_t ri = &rc - &classes[0];  // index of r in class list
	std::size_t si = &sc - &classes[0];  // index of s in class list
	std::size_t vi = classes.size() - 1; // index of victim in class list
	if ( si != vi ) {
		classes[ si ] = move(classes[vi]);
	}
	classes.pop_back();
	if ( ri == vi ) {
		// r has been moved to old position of s
		r = s;
	}

	dbg_verify();
	return true;
}

bool Env::merge( const Env& o ) {
	// short-circuit easy cases
	if ( o.empty() ) return true;
	if ( empty() ) {
		*this = o;
		return true;
	}

	// merge classes
	for ( const TypeClass& c : o.classes ) {
		auto i = c.vars.begin();
		ClassRef r{}; // typeclass in this environment
		
		// look for first existing bound variable
		for ( ; i != c.vars.end(); ++i ) {
			r = findRef( *i );
			if ( r ) break;
		}

		if ( i != c.vars.end() ) { // c needs to be merged into r
			// attempt to merge bounds
			if ( ! mergeBound( r, c.bound ) ) return false;
			// make sure occurs check is not violated
			if ( occursIn( c.vars, r->bound ) ) return false;
			// merge previous variables into this class
			as_non_const(r->vars).insert( c.vars.begin(), i );
			// merge subsequent variables into this class (skip *i, already bound)
			while ( ++i != c.vars.end() ) {
				ClassRef rr = findRef( *i );
				if ( ! rr ) {
					// unbound; safe to add
					as_non_const(*r).vars.insert( *i );
				} else if ( rr == r ) {
					// bound to target already, do nothing
				} else {
					// merge new class into existing
					if ( ! mergeClasses( r, rr ) ) return false;
				}
			}
		} else {  // no variables in c bound, just copy over
			classes.push_back( c );
		}
	}

	// merge assertions
	for ( const auto& a : o.assns ) {
		auto it = assns.find( a.first );
		if ( it == assns.end() ) {
			assns.insert( a );
		} else {
			if ( it->second != a.second ) return false;
		}
	}

	return true;
}

const GC& operator<< (const GC& gc, const Env& env) {
	for ( const TypeClass& entry : env.classes ) {
		gc << entry.vars << entry.bound;
	}

	for ( const auto& assn : env.assns ) {
		gc << assn.first << assn.second;
	}

	return gc;
}

std::ostream& operator<< (std::ostream& out, const TypeClass& c) {
	if ( c.vars.size() == 1 ) {
		out << **c.vars.begin();
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

std::ostream& Env::write( std::ostream& out ) const {
	out << "{";
	// print classes
	bool printed = false;
	for ( const auto& c : classes ) {
		if ( printed ) { out << ", "; } else { printed = true; }
		out << c;
	}
	// print assertions
	TypedExprTypeMutator<EnvSubstitutor> sub{ EnvSubstitutor{ *this } };
	for ( const auto& a : assns ) {
		out << " | " << *a.first << " => ";
		if ( a.second ) { out << *sub(a.second); } else { out << "???"; }
	}
	return out << "}";
}

