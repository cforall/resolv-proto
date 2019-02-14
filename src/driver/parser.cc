#include "parser.h"

#include <cstdlib>
#include <iostream>
#include <string>

#include "args.h"
#include "parser_common.h"

#include "ast/decl.h"
#include "ast/expr.h"
#include "ast/forall.h"
#include "ast/type.h"
#include "data/debug.h"
#include "data/list.h"
#include "data/mem.h"
#include "data/option.h"
#include "resolver/func_table.h"
#include "resolver/resolver.h"

/// Parses a name (lowercase alphanumeric ASCII string starting with a 
/// lowercase letter), returning true, storing result into ret, and  
/// incrementing token if so. token must not be null.
bool parse_name(char const *&token, std::string& ret) {
	const char *end = token;
	
	if ( ('a' <= *end && *end <= 'z') 
	     || '_' == *end
		 || '$' == *end  ) ++end;
	else return false;
	
	while ( ('A' <= *end && *end <= 'Z')
		    || ('a' <= *end && *end <= 'z') 
	        || ('0' <= *end && *end <= '9')
			|| '_' == *end ) ++end;
	
	ret.assign( token, (end-token) );
	
	token = end;
	return true;
}

/// Parses a named type name (hash followed by ASCII alphanumeric string 
/// starting with a letter or underscore), returning true, storing result 
/// (not including hash) into ret, and incrementing token if so. 
/// token must not be null
bool parse_named_type(char const *&token, std::string& ret) {
	const char *end = token;

	if ( ! match_char(end, '#') ) return false;

	if ( ('A' <= *end && *end <= 'Z')
	     || ('a' <= *end && *end <= 'z')
		 || '_' == *end
		 || '$' == *end ) ++end;
	else return false;

	while ( ('A' <= *end && *end <= 'Z')
	        || ('a' <= *end && *end <= 'z')
			|| ('0' <= *end && *end <= '9')
		    || '_' == *end ) ++end;
	
	ret.assign( token+1, (end-token-1) );

	token = end;
	return true;
}

/// Parses a polymorphic type name (lowercase alphanumeric ASCII string 
/// starting with an uppercase letter), returning true, storing result into 
/// ret, and incrementing token if so. token must not be null.
bool parse_poly_type(char const *&token, std::string& ret) {
	const char *end = token;
	
	if ( 'A' <= *end && *end <= 'Z' ) ++end;
	else return false;

	while ( ('A' <= *end && *end <= 'Z')
		    || ('a' <= *end && *end <= 'z') 
	        || ('0' <= *end && *end <= '9')
			|| '_' == *end ) ++end;
	
	ret.assign( token, (end-token) );

	token = end;
	return true;
}

/// Parses a type name, returning true, appending the result into out, and 
/// incrementing token if so. Concrete types will be canonicalized according 
/// to types and polymorphic types according to forall. token must not be null.  
bool parse_type(char const *&token, Resolver& resolver, Args& args, 
	CanonicalTypeMap& types, unique_ptr<Forall>& forall, List<Type>& out);

/// Parses an angle-bracket surrounded type list for the paramters of a generic named type
bool parse_generic_params(char const *&token, Resolver& resolver, Args& args, 
		CanonicalTypeMap& types, unique_ptr<Forall>& forall, List<Type>& out) {
	const char *end = token;

	if ( ! match_char(end, '<') ) return false;
	match_whitespace(end);

	if ( ! parse_type(end, resolver, args, types, forall, out) ) return false;

	while ( match_whitespace(end) ) {
		if ( ! parse_type(end, resolver, args, types, forall, out) ) break;
	}

	if ( ! match_char(end, '>') ) return false;

	token = end;
	return true;
}

/// Parses a function type, returning true, appending the result into out, and 
/// incrementing token if so. Concrete types will be canonicalized according 
/// to types and polymorphic types according to forall. token must not be null
bool parse_func_type(const char*& token, Resolver& resolver, Args& args, 
		CanonicalTypeMap& types, unique_ptr<Forall>& forall, List<Type>& out ) {
	List<Type> returns, params;
	const char* end = token;

	// match opening token
	if ( ! match_char(end, '[') ) return false;
	match_whitespace(end);

	// match return types
	while ( parse_type(end, resolver, args, types, forall, returns) ) {
		match_whitespace(end);
	}

	// match split token
	if ( ! match_char(end, ':') ) return false;
	match_whitespace(end);

	// match parameters
	while ( parse_type(end, resolver, args, types, forall, params) ) {
		match_whitespace(end);
	}

	// match closing token
	if ( ! match_char(end, ']') ) return false;
	match_whitespace(end);

	out.push_back( new FuncType{ move(params), move(returns) } );

	token = end;
	return true;
}

bool parse_type(char const *&token, Resolver& resolver, Args& args, 
		CanonicalTypeMap& types, unique_ptr<Forall>& forall, List<Type>& out) {
	int t;
	std::string n;

	if ( parse_int(token, t) ) {
		auto it = get_canon<ConcType>( types, t );
		if ( it.second && ! args.metrics_only() ) resolver.addType( it.first );
		out.push_back( it.first );
		return true;
	} else if ( parse_named_type(token, n) ) {
		List<Type> params;
		parse_generic_params( token, resolver, args, types, forall, params );
		auto it = get_canon( types, n, move(params) );
		if ( it.second && ! args.metrics_only() ) resolver.addType( it.first );
		out.push_back( it.first );
		return true;
	} else if ( parse_poly_type(token, n) ) {
		if ( ! forall ) { forall.reset( new Forall{} ); }
		out.push_back( forall->add( n ) );
		return true;
	} else if ( parse_func_type(token, resolver, args, types, forall, out) ) {
		return true;
	} else return false;
}

/// Parses a type assertion, returning true and adding the assertion into 
/// binding if so. Concrete types will be canonicalized according to types. 
/// token must not be null.
bool parse_assertion(char const *&token, Resolver& resolver, Args& args, 
		CanonicalTypeMap& types, unique_ptr<Forall>& forall) {
	const char* end = token;

	// look for type assertion
	if ( ! match_char(end, '|') ) return false;
	
	// parse return types
	List<Type> returns;
	match_whitespace(end);
	while ( parse_type(end, resolver, args, types, forall, returns) ) {
		match_whitespace(end);
	}

	std::string name;

	// try for variable assertion
	if ( match_char(end, '&') ) {
		if ( ! parse_name(end, name) ) return false;

		if ( ! forall ) { forall.reset( new Forall{} ); }
		forall->addAssertion( new VarDecl{ name, move(returns) } );
		token = end;
		return true;
	}

	// function assertion -- parse name
	if ( ! parse_name(end, name) ) return false;
	
	// parse parameters
	List<Type> params;
	match_whitespace(end);
	while ( parse_type(end, resolver, args, types, forall, params) ) {
		match_whitespace(end);
	}

	if ( ! forall ) { forall.reset( new Forall{} ); }
	forall->addAssertion( new FuncDecl{ name, move(params), move(returns) } );
	token = end;
	return true;
}

/// Parses a declaration from line; returns true and adds the declaration to 
/// funcs if found; will fail if given a valid func that does not consume the 
/// whole line. line must not be null.
bool parse_decl( char const *line, Resolver& resolver, CanonicalTypeMap& types, 
		Args& args, Metrics& metrics ) {
	List<Type> returns;
	std::string name;
	std::string tag = "";
	unique_ptr<Forall> forall;
	
	// parse return types
	match_whitespace(line);
	while ( parse_type(line, resolver, args, types, forall, returns) ) {
		match_whitespace(line);
	}

	// check for variable decl
	if ( ! returns.empty() && match_char(line, '&') ) {
		if ( ! parse_name(line, name) ) return false;

		// optionally parse tag
		if ( match_char(line, '-') ) {
			if ( ! parse_name(line, tag) ) return false;
		}

		// check line consumed
		if ( ! is_blank(line) ) return false;

		if ( ! args.metrics_only() ) {
			resolver.addDecl( new VarDecl{ name, tag, move(returns) } );
		}
		metrics.mark_decl( name );
		return true;
	}
	
	// parse function decl
	List<Type> params;
	if ( ! parse_name(line, name) ) return false;
	
	// optionally parse tag
	if ( match_char(line, '-') ) {
		// might have been subsequent negative-valued type
		if ( ! parse_name(line, tag) ) { --line; }
	}
	
	// parse parameters
	match_whitespace(line);
	while ( parse_type(line, resolver, args, types, forall, params) ) {
		match_whitespace(line);
	}

	// parse type assertions
	while ( parse_assertion(line, resolver, args, types, forall) );
	
	// check line consumed
	if ( ! is_empty(line) ) return false;
	
	// complete declaration
	if ( forall ) {
		metrics.mark_decl( name, forall->assertions().size() );
	} else {
		metrics.mark_decl( name );
	}
	if ( ! args.metrics_only() ) {
		resolver.addDecl( 
			new FuncDecl{ name, tag, move(params), move(returns), move(forall) } );
	}
		
	return true;
}

/// Parses a concrete type name, returning true, appending the result into out, and 
/// incrementing token if so. Concrete types will be canonicalized according 
/// to types and polymorphic types forbidden. token must not be null.  
bool parse_conc_type(char const *&token, Resolver& resolver, Args& args, 
	CanonicalTypeMap& types, List<Type>& out);

/// Parses an angle-bracket surrounded type list for the paramters of a concrete generic named type
bool parse_conc_generic_params(char const *&token, Resolver& resolver, 
		Args& args, CanonicalTypeMap& types, List<Type>& out) {
	const char *end = token;

	if ( ! match_char(end, '<') ) return false;
	match_whitespace(end);

	if ( ! parse_conc_type(end, resolver, args, types, out) ) return false;

	while ( match_whitespace(end) ) {
		if ( ! parse_conc_type(end, resolver, args, types, out) ) break;
	}

	if ( ! match_char(end, '>') ) return false;

	token = end;
	return true;
}

bool parse_conc_type(char const *&token, Resolver& resolver, Args& args, 
		CanonicalTypeMap& types, List<Type>& out) {
	int t;
	std::string n;

	if ( parse_int(token, t) ) {  // ConcType
		auto it = get_canon<ConcType>( types, t );
		if ( it.second && ! args.metrics_only() ) resolver.addType( it.first );
		out.push_back( it.first );
		return true;
	} else if ( parse_named_type(token, n) ) {  // concrete NamedType
		List<Type> params;
		parse_conc_generic_params( token, resolver, args, types, params );
		auto it = get_canon( types, n, move(params) );
		if ( it.second && ! args.metrics_only() ) resolver.addType( it.first );
		out.push_back( it.first );
		return true;
	} else if ( match_char(token, '[') ) {  // concrete FuncType
		match_whitespace(token);

		// match return types
		List<Type> returns;
		while ( parse_conc_type(token, resolver, args, types, returns) ) {
			match_whitespace(token);
		}

		// match split token
		if ( ! match_char(token, ':') ) return false;
		match_whitespace(token);

		// match parameters
		List<Type> params;
		while ( parse_conc_type(token, resolver, args, types, params) ) {
			match_whitespace(token);
		}

		// match closing token
		if ( ! match_char(token, ']') ) return false;
		match_whitespace(token);

		// return type
		out.push_back( new FuncType{ move(params), move(returns) } );
		return true;
	} else return false;
}

/// Parses a subexpression; returns true and adds the expression to exprs if found.
/// line must not be null.
bool parse_subexpr( char const *&token, List<Expr>& exprs, Resolver& resolver, 
		Args& args, Metrics& metrics, CanonicalTypeMap& types ) {
	const char *end = token;
	
	// Check for a concrete type expression
	List<Type> cty;
	if ( parse_conc_type( end, resolver, args, types, cty ) ) {
		exprs.push_back( new ValExpr( cty.front() ) );
		metrics.mark_sub();
		token = end;
		return true;
	}

	// Check for name expression
	if ( match_char(end, '&') ) {
		std::string name;
		if ( ! parse_name(end, name) ) return false;
		match_whitespace(end);

		exprs.push_back( new NameExpr( name ) );
		metrics.mark_sub();
		token = end;
		return true;
	}
	
	// Check for function call
	std::string name;
	if ( ! parse_name(end, name) ) return false;
	match_whitespace(end);
	if ( ! match_char(end, '(') ) return false;
	
	// Read function args
	metrics.start_subs();
	List<Expr> eargs;
	match_whitespace(end);
	while ( parse_subexpr(end, eargs, resolver, args, metrics, types) ) {
		match_whitespace(end);
	}
	metrics.end_subs();
	
	// Find closing bracket
	if ( ! match_char(end, ')') ) return false;
	match_whitespace(end);
	
	exprs.push_back( new FuncExpr( name, move(eargs) ) );
	metrics.mark_sub();
	token = end;
	return true;
}

/// Parses an expression from line; returns true and adds the expression to 
/// exprs if found; will fail if given a valid expr that does not consume the 
/// whole line. line must not be null.
bool parse_expr( char const *line, Resolver& resolver, Args& args, 
		Metrics& metrics, CanonicalTypeMap& types ) {
	match_whitespace(line);
	List<Expr> exprs;
	if ( parse_subexpr(line, exprs, resolver, args, metrics, types) && is_empty(line) ) {
		assume( exprs.size() == 1, "successful expression parse results in single expression" );
		if ( ! args.metrics_only() ) { resolver.addExpr( exprs[0] ); }
		metrics.mark_expr();
		return true;
	} else {
		metrics.reset_expr();
		return false;
	}
}

/// Mode for echo_line -- declarations are echoed for Filtered verbosity, expressions are not
enum EchoMode { EXPR, DECL };

/// Echos line if in appropriate mode
void echo_line( std::string& line, Args& args, EchoMode mode = EXPR ) {
	if ( args.verbosity() == Args::Verbosity::Verbose
			|| ( args.verbosity() == Args::Verbosity::Filtered && mode == DECL ) ) {
		args.out() << line << std::endl;
	}
}

/// Parses a scope from a series of lines (excluding opening "{" line if in block), 
/// continuing until end-of-input or terminating "}" line is found.
/// Prints an error and exits if invalid declaration or expression found
void parse_block( std::istream& in, unsigned& n, unsigned& scope, Resolver& resolver, 
		CanonicalTypeMap& types, Args& args, Metrics& metrics ) {
	std::string line;
	
	// parse declarations and delimiter
	while ( std::getline( in, line ) ) {
		++n;
		
		if ( is_blank( line ) ) {
			echo_line( line, args, DECL );
			continue;
		}

		// break when finished declarations
		if ( line_matches( line, "%%" ) ) {
			echo_line( line, args, DECL );
			break;
		}

		bool ok = parse_decl( line.data(), resolver, types, args, metrics );
		if ( ! ok ) {
			std::cerr << "Invalid declaration [" << n << "]: \"" << line << "\"" << std::endl;
			std::exit(1);
		}

		echo_line( line, args, DECL );
	}

	while ( std::getline( in, line ) ) {
		++n;

		if ( is_blank( line ) ) {
			echo_line( line, args );
			continue;
		}

		// break when finished block
		if ( line_matches( line, "}" ) ) {
			--scope;
			resolver.endScope();
			metrics.end_lex_scope();
			break;
		}

		// recurse when starting new block
		if ( line_matches( line, "{" ) ) {
			++scope;
			resolver.beginScope();
			metrics.begin_lex_scope();
			parse_block( in, n, scope, resolver, types, args, metrics );
			continue;
		}

		// parse and resolve expression
		if ( args.line_nos() ) { args.out() << n << ": " << std::flush; }
		bool ok = parse_expr( line.data(), resolver, args, metrics, types );
		if ( ! ok ) {
			std::cerr << "Invalid expression [" << n << "]: \"" << line << "\"" << std::endl;
			std::exit(1);
		}

		echo_line( line, args );
	}
}

void run_input( std::istream& in, Resolver& resolver, Args& args, Metrics& metrics ) {
	CanonicalTypeMap types;
	unsigned n = 0, scope = 0;

	parse_block(in, n, scope, resolver, types, args, metrics);

	if ( scope != 0 ) {
		std::cerr << "Unmatched braces" << std::endl;
		std::exit(1);
	}
}
