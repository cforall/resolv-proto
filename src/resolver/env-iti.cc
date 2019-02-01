#include "env.h"

#include <ostream>

#include "cost.h"
#include "interpretation.h"
#include "env_substitutor.h"
#include "occurs_in.h"
#include "type_unifier.h"

#include "data/debug.h"
#include "ast/decl.h"
#include "ast/type.h"
#include "ast/typed_expr_type_mutator.h"
#include "data/cast.h"
#include "data/gc.h"
#include "data/mem.h"

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

#if defined RP_DEBUG && RP_DEBUG >= 1
void EnvGen::verify() const {
	for ( unsigned ci = 0; ci < classes.size(); ++ci ) {
		const TypeClass& tc = classes[ci];

		for ( unsigned vi = 0; vi < tc.vars.size(); ++vi ) {
			const PolyType* v = tc.vars[vi];

			const ClassRef& vr = bindings.at( v );
			assume( vr.env == this && vr.ind == ci, "local variable is cached locally" );

			for ( unsigned vj = vi + 1; vj < tc.vars.size(); ++vj ) {
				assume( v != tc.vars[vj], "variable is unique in type class" );
			}

			for ( unsigned cj = ci + 1; cj < classes.size(); ++cj ) {
				for ( const PolyType* u : classes[cj].vars ) {
					assume( v != u, "variable is unique in environment" );
				}
			}
		}

		if ( tc.bound ) {
			Env env{ as_non_const(this) };
			assume( ! env.occursIn( tc.vars, tc.bound ), "occurs check not subverted" );
		}
	}

	for ( const auto& b : bindings ) {
		assume( b.second.ind < b.second.env->classes.size(), "valid cache index" );
		const TypeClass& bc = *b.second;
		assume( std::count( bc.vars.begin(), bc.vars.end(), b.first ), "cached binding valid" );
	}

	if ( parent ) parent->verify();
}
#endif

bool EnvGen::mergeBound( Env& env, ClassRef& r, const Type* cbound ) {
	dbg_verify();
	if ( cbound == nullptr ) return true; // trivial if no type to bind to

	if ( r->bound == nullptr ) { // bind if no type in target class
		return env.bindType( r, cbound );
	} else if ( r->bound == cbound ) { // shortcut for easy case
		return true;
	} else { // attempt to structurally bind r->bound to cbound
		Cost::Element cost = 0;
		const PolyType* rt = r->vars[0];  // save variable in this class
		TypeUnifier tu{ env, cost };
		const Type* common = tu( r->bound, cbound );
		if ( ! common ) return false;
		Env rEnv{ as_non_const(r.env) };
		r = rEnv.findRef( rt );  // reset ref to restored class
		if ( r.env != env.self ) { env.self->copyClass( r ); }
		env.self->classes[ r.ind ].bound = common;  // specialize r's bound to common type
#if defined RP_DEBUG && RP_DEBUG >= 1
		env.self->verify();
#endif
		return true;
	}
}

bool EnvGen::mergeClasses( Env& env, ClassRef& r, ClassRef s ) {
	const PolyType* st = s->vars[0]; // to reaquire s when complete

	// ensure bounds match
	if ( ! mergeBound( env, r, s->bound ) ) return false;

	// find r, s after mergeBound
	TypeClass& rc = env.self->classes[ r.ind ];
	Env sEnv{ as_non_const(s.env) };
	s = sEnv.findRef( st );
	const TypeClass& sc = *s;

	// ensure occurs check not violated
	if ( env.occursIn( s->vars, rc.bound ) ) return false;

	if ( s.env == env.self ) {
		// merging two existing local classes; guaranteed that vars don't overlap
		rc.vars.insert( rc.vars.end(), sc.vars.begin(), sc.vars.end() );
		for ( const PolyType* v : sc.vars ) { env.self->bindings[v] = r; }
		// remove s from local class list
		std::size_t victim = env.self->classes.size() - 1;
		if ( s.ind != victim ) {
			env.self->classes[ s.ind ] = move(env.self->classes[victim]);
			for ( const PolyType* v : env.self->classes[ s.ind ].vars ) {
				env.self->bindings[v] = s;
			}
		}
		env.self->classes.pop_back();
		if ( r.ind == victim ) {
			// r has been moved to old position of s
			r.ind = s.ind;
		}
#if defined RP_DEBUG && RP_DEBUG >= 1
		env.self->verify();
#endif
	} else {
		// merge in variables
		rc.vars.reserve( rc.vars.size() + sc.vars.size() );
		for ( const PolyType* v : sc.vars ) {
			ClassRef rr = env.findRef( v );
			if ( ! rr || rr == s ) {
				// not bound or bound in target class in parent; can safely add to r if no cycle
				env.self->addToClass( r.ind, v );
			} else if ( rr == r ) {
				// already in class, do nothing
			} else {
				// new victim class; needs to merge successfully as well
				if ( ! env.self->mergeClasses( env, r, rr ) ) return false;
			}
		}
#if defined RP_DEBUG && RP_DEBUG >= 1
		env.self->verify();
#endif
	}

	return true;
}

/// marks a type as seen; true iff the type has already been marked
bool markedSeen( EnvGen::SeenTypes& seen, const PolyType* v ) {
	return seen.insert( v ).second == false;
}

/// true iff any of the types in vs have been seen already; adds types in vs to seen
bool seenAny( EnvGen::SeenTypes& seen, const List<PolyType>& vs ) {
	bool ret = false;
	for ( const PolyType* v : vs ) if ( markedSeen( seen, v ) ) ret = true;
	return ret;
}

bool EnvGen::merge( Env& env, const EnvGen& o, EnvGen::SeenTypes& seen ) {
	// merge classes
	for ( std::size_t ci = 0; ci < o.classes.size(); ++ci ) {
		const TypeClass& c = o.classes[ci];
		std::size_t n = c.vars.size();
		std::size_t i;
		ClassRef r{};  // typeclass in this environment
		bool nonempty = false;  // cover corner case where all vars in class previously seen

		for ( i = 0; i < n; ++i ) {
			if ( markedSeen( seen, c.vars[i] ) ) continue;
			nonempty = true;
			// look for first existing bound variable
			r = env.findRef( c.vars[i] );
			if ( r ) break;
		}

		if ( i < n ) {
			// attempt to merge bound into r
			if ( ! mergeBound( env, r, c.bound ) ) return false;
			// make sure that occurs check is not violated
			if ( env.occursIn( c.vars, r->bound ) ) return false;
			// merge previous variables into this class
			if ( r.env != env.self ) { env.self->copyClass( r ); }
			for ( std::size_t j = 0; j < i; ++j ) { env.self->addToClass( r.ind, c.vars[j] ); }
			// merge subsequent variables into this class
			while ( ++i < n ) {
				if ( markedSeen( seen, c.vars[i] ) ) continue;
				// copy unbound variables into this class, skipping ones already bound to it
				ClassRef rr = env.findRef( c.vars[i] );
				if ( ! rr ) {
					// unbound; safe to add
					env.self->addToClass( r.ind, c.vars[i] );
				} else if ( rr == r ) {
					// bound to target class already; do nothing
				} else {
					// merge new class into existing
					if ( ! env.self->mergeClasses( env, r, rr ) ) return false;
				}
			}
		} else if ( nonempty ) {
			// no variables in this typeclass bound, just copy up
			env.self->copyClass( ClassRef{ &o, ci } );
		}
	}

	// merge assertions
	for ( const auto& a : o.assns ) {
		auto it = env.self->assns.find( a.first );
		if ( it == env.self->assns.end() ) {
			env.self->assns.insert( a );
			++env.self->localAssns;
		} else {
			if ( it->second != a.second ) return false;
		}
	}

	// merge parents
	if ( o.parent ) {
		if ( o.parent->empty() || inheritsFrom( o.parent ) ) return true;
		return merge( env, *o.parent, seen );
	}
	
	return true;
}

void EnvGen::getUnbound( SeenTypes& seen, std::vector<TypeClass>& out ) const {
	for ( const TypeClass& c : classes ) {
		// skip classes containing vars we've seen before
		if ( seenAny( seen, c.vars ) ) continue;
		// report unbound classes
		if ( ! c.bound ) { out.push_back( c ); }
	}

	if ( parent ) { parent->getUnbound( seen, out ); }
}

void EnvGen::trace( const GC& gc ) const {
	for ( const TypeClass& entry : classes ) {
		gc << entry.vars << entry.bound;
	}

	for ( const auto& assn : assns ) {
		gc << assn.first << assn.second;
	}

	gc << parent;
}

void writeClasses(std::ostream& out, const EnvGen* env, bool printed = false, 
		EnvGen::SeenTypes&& seen = {}) {
	for ( const TypeClass& c : env->getClasses() ) {
		if ( seenAny( seen, c.vars ) ) continue;
		if ( printed ) { out << ", "; } else { printed = true; }
		out << c;
	}

	if ( env->parent ) writeClasses(out, env->parent, printed, move(seen));
}

void writeAssertions(std::ostream& out, const EnvGen* env, 
		TypedExprTypeMutator<EnvSubstitutor>& sub, 
		std::unordered_set<const Decl*>&& seen = {}) {
	for ( const auto& assn : env->getAssertions() ) {
		// skip seen assertions
		if ( ! seen.insert( assn.first ).second ) continue;
		// print assertion
		out << " | " << *assn.first << " => ";
		if ( assn.second ) { out << *sub(assn.second); } else { out << "???"; }
	}
	
	if ( env->parent ) writeAssertions(out, env->parent, sub, move(seen));
}

std::ostream& Env::write(std::ostream& out) const {
	out << "{";
	// print classes
	writeClasses(out, self);
	TypedExprTypeMutator<EnvSubstitutor> sub{ EnvSubstitutor{ *this } };
	writeAssertions(out, self, sub);
	return out << "}";
}

bool Env::occursIn( const List<PolyType>& vars, const Type* t ) const {
	return OccursIn<List>{ vars, *this }( t );
}