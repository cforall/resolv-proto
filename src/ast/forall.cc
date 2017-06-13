#include "forall.h"

#include <algorithm>

#include "decl.h"
#include "forall_substitutor.h"
#include "type.h"

#include "data/gc.h"

Forall::Forall( const Forall& o ) : n(o.n), vars(), assns() {
	ForallSubstitutor m{ &o, this };
	vars.reserve( o.vars.size() );
	for ( const PolyType* v : o.vars ) { m(v); } // substitutor adds vars when seen
	assns.reserve( o.assns.size() );
	for ( const FuncDecl* a : o.assns ) { assns.push_back( m(a) ); }
}

Forall& Forall::operator= ( const Forall& o ) {
	if ( &o == this ) return *this;

	vars.clear();
	assns.clear();

	n = o.n;
	ForallSubstitutor m{ &o, this };
	vars.reserve( o.vars.size() );
	for ( const PolyType* v : o.vars ) { m(v); } // substitutor adds vars when seen
	assns.reserve( o.assns.size() );
	for ( const FuncDecl* a : o.assns ) { assns.push_back( m(a) ); }

	return *this;
}

const PolyType* Forall::get( const std::string& p ) const {
	auto rit = std::find_if( vars.begin(), vars.end(), 
		[&p]( const PolyType* v ) { return v->name() == p; } );
	return rit == vars.end() ? nullptr : *rit;
}

const PolyType* Forall::add( const std::string& p ) {
	const PolyType* rep = get( p );
	if ( ! rep ) {
		rep = new PolyType{ p, this };
		vars.push_back( rep );
	}
	return rep;
}

std::ostream& operator<< (std::ostream& out, const Forall& f) {
	if ( f.empty() ) return out << "{}";

	out << '{';
	auto& vars = f.variables();
	if ( ! vars.empty() ) {
		auto it = vars.begin();
		while (true) {
			out << (*it)->name();
			if ( ++it == vars.end() ) break;
			out << ", ";
		}
	}
	for ( auto assn : f.assertions() ) {
		out << " | ";
		assn->write( out, ASTNode::Print::InputStyle );
	}
	return out << '}';
}

const GC& operator<< (const GC& gc, const Forall* f) { return gc << f->vars << f->assns; }
