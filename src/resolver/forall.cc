#include "forall.h"

#include "forall_substitutor.h"

#include "ast/decl.h"
#include "ast/type.h"
#include "data/gc.h"

Forall::Forall( const Forall& o ) : vars(), assns() {
	ForallSubstitutor m{ &o, this };
	vars.reserve( o.vars.size() );
	for ( const PolyType* v : o.vars ) { vars.push_back( as<PolyType>(m(v)) ); }
	assns.reserve( o.assns.size() );
	for ( const FuncDecl* a : o.assns ) { assns.push_back( m(a) ); }
}

Forall& Forall::operator= ( const Forall& o ) {
	if ( &o == this ) return *this;

	vars.clear();
	assns.clear();

	ForallSubstitutor m{ &o, this };
	vars.reserve( o.vars.size() );
	for ( const PolyType* v : o.vars ) { vars.push_back( as<PolyType>(m(v)) ); }
	assns.reserve( o.assns.size() );
	for ( const FuncDecl* a : o.assns ) { assns.push_back( m(a) ); }

	return *this;
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
