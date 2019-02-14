#pragma once

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
	
public:
	Metrics() 
	: n_decls(0), n_exprs(0), n_subexprs(0), max_depth(0), sum_depth(0), 
	  n_assns(0), n_poly_decls(0), max_assns(0), 
	  local_depth(0), max_local_depth(0) {}
	
	/// Mark a monomorphic declaration
	void mark_decl() {
		++n_decls;
	}

	/// Mark a polymorphic declaration with `assns` assertions
	void mark_decl(unsigned assns) {
		++n_decls;
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
