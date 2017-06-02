#pragma once

#include <ostream>
#include <string>

#include "data/list.h"
#include "data/gc.h"
#include "data/mem.h"

class FuncDecl;
class PolyType;

/// Represents a forall clause; owns a set of type variables and assertions.
class Forall {
	friend const GC& operator<< (const GC&, const Forall*);
	friend class ForallSubstitutor;

	std::string n;        ///< Name for this clause; not required to be unique
	List<PolyType> vars;  ///< Type variables owned by this forall clause
	List<FuncDecl> assns; ///< Type assertions owned by this forall clause

public:
	Forall() = default;

	/// Name-only constructor
	Forall( const std::string& n ) : n(n), vars(), assns() {}

	/// Copy constructor re-binds vars to new forall instance
	Forall( const Forall& );
	/// Assignment operator re-binds vars to new forall instance
	Forall& operator= ( const Forall& );

	/// Copies the given forall clause, returning a null pointer if o is null.
	static unique_ptr<Forall> from( const Forall* o ) { return { o ? new Forall{ *o } : nullptr }; }
	
	const std::string& name() const { return n; }
	const List<PolyType>& variables() const { return vars; }
	const List<FuncDecl>& assertions() const { return assns; }

	void set_name( const std::string& x ) { n = x; }

	/// Finds the existing type variable with the given name; returns null if none such
	const PolyType* get( const std::string& p ) const;

	/// Adds a new type variable in this forall if not present; returns existing var if present
	PolyType* add( const std::string& p );

	/// Adds a new assertion to this forall
	void addAssertion( const FuncDecl* f ) { assns.push_back( f ); }

	/// true iff no variables or assertions
	bool empty() const { return vars.empty() && assns.empty(); }
};

std::ostream& operator<< (std::ostream&, const Forall&);
