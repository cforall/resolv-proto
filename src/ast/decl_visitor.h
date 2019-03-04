#pragma once

// Copyright (c) 2015 University of Waterloo
//
// The contents of this file are covered under the licence agreement in 
// the file "LICENCE" distributed with this repository.

#include <cstddef>

#include "decl.h"

#include "data/cast.h"
#include "data/debug.h"

/// Visitor for declarations
template<typename Self, typename T>
class DeclVisitor {
public:
	/// Updates a possible value from the visit and returns bool indicating whether iteration 
	/// should continue.
	bool visit( const Decl* d, T& r ) {
		if ( ! d ) return as<Self>(this)->visit( nullptr, r );

		auto tid = typeof(d);
		if ( tid == typeof<FuncDecl>() )
			return as<Self>(this)->visit( as<FuncDecl>(d), r );
		if ( tid == typeof<VarDecl>() )
			return as<Self>(this)->visit( as<VarDecl>(d), r );
		
		unreachable("invalid Decl type");
		return false;
	}

	bool visitChildren( const FuncDecl* d, T& r ) {
		const Forall* f = d->forall();
		if ( ! f ) return true;
		for ( const Decl* asn : f->assertions() ) {
			if ( ! as<Self>(this)->visit( asn, r ) ) return false;
		}
		return true;
	}

	bool visit( std::nullptr_t, T& ) { return true; }

	bool visit( const FuncDecl* d, T& r ) { return visitChildren( d, r ); }

	bool visit( const VarDecl* d, T& r ) { return true; }

	/// Returns a (default-constructed) value from the visit
	T operator() ( const Decl* d ) {
		T r{};
		visit( d, r );
		return r;
	}

	/// Updates the given value by the visitor
	T& operator() ( const Decl* d, T& r ) {
		visit( d, r );
		return r;
	}

	/// Visits, starting from a default value
	T operator() ( const Decl* d, T&& r ) {
		visit( d, r );
		return r;
	}
};
