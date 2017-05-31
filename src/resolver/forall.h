#pragma once

#include <ostream>
#include <string>

#include "data/list.h"
#include "data/gc.h"

class FuncDecl;
class PolyType;

/// Represents a forall clause; owns a set of type variables and assertions.
class Forall {
	friend const GC& operator<< (const GC&, const Forall*);
	friend class ForallSubstitutor;

	const std::string n;  ///< Name for this clause; not required to be unique
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
	
	const std::string& name() const { return n; }
	const List<PolyType>& variables() const { return vars; }
	const List<FuncDecl>& assertions() const { return assns; }

	/// true iff no variables or assertions
	bool empty() const { return vars.empty() && assns.empty(); }
};

std::ostream& operator<< (std::ostream&, const Forall&);
