// Copyright (c) 2015 University of Waterloo
//
// The contents of this file are covered under the licence agreement in 
// the file "LICENCE" distributed with this repository.

#include "forall.h"

#include <algorithm>

#include "decl.h"
#include "forall_substitutor.h"
#include "type.h"

#include "data/gc.h"

Forall::Forall(const Forall& o, unsigned& src) : vars(), assns() {
	vars.reserve(o.vars.size());
	for ( const PolyType* v : o.vars ) { vars.push_back( new PolyType{ v->name(), ++src } ); }
	assns.reserve(o.assns.size());
	ForallSubstitutor m{ this };
	for ( const Decl* a : o.assns ) { assns.push_back( m(a, src) ); }
}

const PolyType* Forall::get( const std::string& p ) const {
	auto rit = std::find_if( vars.begin(), vars.end(), 
		[&p]( const PolyType* v ) { return v->name() == p; } );
	return rit == vars.end() ? nullptr : *rit;
}

const PolyType* Forall::add( const std::string& p ) {
	const PolyType* rep = get( p );
	if ( ! rep ) {
		rep = new PolyType{ p };
		vars.push_back( rep );
	}
	return rep;
}

const PolyType* Forall::add( const std::string& p, unsigned& src ) {
	const PolyType* rep = get( p );
	if ( ! rep ) {
		rep = new PolyType{ p, ++src };
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
			out << **it;
			if ( ++it == vars.end() ) break;
			out << ", ";
		}
	}
	for ( auto assn : f.assertions() ) {
		out << " | ";
		out << *assn;
	}
	return out << '}';
}

const GC& operator<< (const GC& gc, const Forall* f) {
	if ( ! f ) return gc;
	return gc << f->vars << f->assns;
}
