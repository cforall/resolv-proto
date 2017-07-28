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

	List<PolyType> vars;  ///< Type variables owned by this forall clause
	List<FuncDecl> assns; ///< Type assertions owned by this forall clause

public:
	Forall() = default;

	/// Copies a given forall clause, rebinding its type variables to new indices starting after src
	Forall(const Forall& o, unsigned& src);

	/// Copies the given forall clause, returning a null pointer if o is null.
	static unique_ptr<Forall> from( const Forall* o ) {
		return unique_ptr<Forall>{ o ? new Forall{ *o } : nullptr };
	}

	/// Copies the given forall clause, rebound to src, returns null pointer if o is null
	static unique_ptr<Forall> from( const Forall* o, unsigned& src ) {
		return unique_ptr<Forall>{ o ? new Forall{ *o, src } : nullptr };
	}
	
	const List<PolyType>& variables() const { return vars; }
	const List<FuncDecl>& assertions() const { return assns; }

	/// Finds the existing type variable with the given name; returns null if none such
	const PolyType* get( const std::string& p ) const;

	/// Adds a new type variable with ID 0 in this forall if not present;
	/// returns existing var if present
	const PolyType* add( const std::string& p );

	/// Adds a new type variable with a new ID from the given source in this forall if not present; 
	/// returns existing var if present
	const PolyType* add( const std::string& p, unsigned& src );

	/// Adds a new assertion to this forall
	void addAssertion( const FuncDecl* f ) { assns.push_back( f ); }

	/// true iff no variables or assertions
	bool empty() const { return vars.empty() && assns.empty(); }
};

std::ostream& operator<< (std::ostream&, const Forall&);
