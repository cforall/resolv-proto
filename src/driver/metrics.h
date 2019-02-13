#pragma once

struct Metrics {
	unsigned n_decls;   ///< Total count of declarations
	unsigned n_exprs;   ///< Total count of expressions
	
	Metrics() : n_decls(0), n_exprs(0) {}
};
