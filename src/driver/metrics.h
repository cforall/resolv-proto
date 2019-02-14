#pragma once

#include <string>
#include "data/scoped_map.h"

struct Metrics {
	unsigned n_decls;          ///< Total count of declarations
	unsigned n_exprs;          ///< Total count of expressions
	unsigned n_subexprs;       ///< Total count of subexpressions
	unsigned max_depth;        ///< Maximum subexpression depth
	unsigned sum_depth;        ///< Summed maximum subexpression depth over all exprs
	unsigned n_assns;          ///< Total count of assertions
	unsigned n_poly_decls;     ///< Number of polymorphic declarations
	unsigned max_assns;        ///< Maximum assertions on a single declaration
	
private:
	unsigned local_depth;      ///< Local subexpression depth
	unsigned max_local_depth;  ///< Maximum depth seen during subexpression
	
	/// Count of declarations with a given name
	ScopedMap<std::string,unsigned> decls_per_name;
public:
	/// Maximum value of declarations per name
	unsigned max_decls_per_name;
	
public:
	Metrics() 
	: n_decls(0), n_exprs(0), n_subexprs(0), max_depth(0), sum_depth(0), 
	  n_assns(0), n_poly_decls(0), max_assns(0), local_depth(0), max_local_depth(0),
	  decls_per_name(), max_decls_per_name() {}
	
	/// Begin lexical scope for names
	void begin_lex_scope() { decls_per_name.beginScope(); }

	/// End lexical scope for names
	void end_lex_scope() { decls_per_name.endScope(); }

	/// Mark a monomorphic declaration
	void mark_decl(const std::string& name) {
		++n_decls;

		// count overloads, accounting for scope
		auto it = decls_per_name.findAt( decls_per_name.currentScope(), name );
		if ( it == decls_per_name.end() ) {
			decls_per_name.insert( name, 1 );  // new in scope
		} else {
			++it->second;                      // exists in last scope
		}
		// check for max decls
		unsigned decls = 0;
		for ( auto i : decls_per_name.findAll(name) ) { decls += i->second; }
		if ( decls > max_decls_per_name ) { max_decls_per_name = decls; }
	}

	/// Mark a polymorphic declaration with `assns` assertions
	void mark_decl(const std::string& name, unsigned assns) {
		mark_decl(name);
		++n_poly_decls;
		n_assns += assns;
		if ( assns > max_assns ) { max_assns = assns; }
	}

	/// Start parsing subexpression list
	void start_subs() { ++local_depth; }
	
	/// Finish parsing subexression list
	void end_subs() { --local_depth; }

	/// Seen a subexpression
	void mark_sub() {
		++n_subexprs;
		if ( local_depth > max_local_depth ) { max_local_depth = local_depth; }
	}

	/// Reset on local expression concluded
	void reset_expr() {
		local_depth = 0;
		max_local_depth = 0;
	}

	/// Seen a top-level expression
	void mark_expr() {
		++n_exprs;
		sum_depth += max_local_depth;
		if ( max_local_depth > max_depth ) { max_depth = max_local_depth; }
		reset_expr();
	}
};
