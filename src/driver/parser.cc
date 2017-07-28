#include "parser.h"

#include <iostream>
#include <string>

#include "args.h"
#include "parser_common.h"

#include "ast/decl.h"
#include "ast/expr.h"
#include "ast/forall.h"
#include "ast/type.h"
#include "data/list.h"
#include "data/mem.h"
#include "resolver/func_table.h"

/// Parses a name (lowercase alphanumeric ASCII string starting with a 
/// lowercase letter), returning true, storing result into ret, and  
/// incrementing token if so. token must not be null.
bool parse_name(char *&token, std::string& ret) {
	char *end = token;
	
	if ( 'a' <= *end && *end <= 'z' ) ++end;
	else return false;
	
	while ( ('a' <= *end && *end <= 'z') 
	        || ('0' <= *end && *end <= '9') ) ++end;
	
	ret.assign( token, (end-token) );
	
	token = end;
	return true;
}

/// Parses a named type name (hash followed by ASCII alphanumeric string 
/// starting with a letter or underscore), returning true, storing result 
/// (not including hash) into ret, and incrementing token if so. 
/// token must not be null
bool parse_named_type(char *&token, std::string& ret) {
	char *end = token;

	if ( ! match_char(end, '#') ) return false;

	if ( ('A' <= *end && *end <= 'Z')
	     || ('a' <= *end && *end <= 'z')
		 || '_' == *end ) ++end;
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
bool parse_poly_type(char *&token, std::string& ret) {
	char *end = token;
	
	if ( 'A' <= *end && *end <= 'Z' ) ++end;
	else return false;

	while ( ('a' <= *end && *end <= 'z') 
	        || ('0' <= *end && *end <= '9') ) ++end;
	
	ret.assign( token, (end-token) );

	token = end;
	return true;
}

/// Parses a type name, returning true, appending the result into out, and 
/// incrementing token if so. Concrete types will be canonicalized according 
/// to types. token must not be null.  
bool parse_type(char *&token, CanonicalTypeMap& types, unique_ptr<Forall>& forall, List<Type>& out) {
	int t;
	std::string n;

	if ( parse_int(token, t) ) {
		out.push_back( get_canon<ConcType>( types, t ) );
		return true;
	} else if ( parse_named_type(token, n) ) {
		out.push_back( get_canon<NamedType>( types, n ) );
		return true;
	} else if ( parse_poly_type(token, n) ) {
		if ( ! forall ) { forall.reset( new Forall{} ); }
		out.push_back( forall->add( n ) );
		return true;
	} else return false;
}

/// Parses a type assertion, returning true and adding the assertion into 
/// binding if so. Concrete types will be canonicalized according to types. 
/// token must not be null.
bool parse_assertion(char*&token, CanonicalTypeMap& types, unique_ptr<Forall>& forall) {
	char* end = token;

	// look for type assertion
	if ( ! match_char(end, '|') ) return false;
	
	List<Type> returns, params;
	std::string name;

	// parse return types
	match_whitespace(end);
	while ( parse_type(end, types, forall, returns) ) {
		match_whitespace(end);
	}

	// parse name
	if ( ! parse_name(end, name) ) return false;
	
	// parse parameters
	match_whitespace(end);
	while ( parse_type(end, types, forall, params) ) {
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
bool parse_decl(char *line, FuncTable& funcs, CanonicalTypeMap& types) {
	List<Type> returns, params;
	std::string name;
	std::string tag = "";
	unique_ptr<Forall> forall;
	
	// parse return types
	match_whitespace(line);
	while ( parse_type(line, types, forall, returns) ) {
		match_whitespace(line);
	}
	
	// parse name
	if ( ! parse_name(line, name) ) return false;
	
	// optionally parse tag
	if ( match_char(line, '-') ) {
		// might have been subsequent negative-valued type
		if ( ! parse_name(line, tag) ) { --line; }
	}
	
	// parse parameters
	match_whitespace(line);
	while ( parse_type(line, types, forall, params) ) {
		match_whitespace(line);
	}

	// parse type assertions
	while ( parse_assertion(line, types, forall) );
	
	// check line consumed
	if ( ! is_empty(line) ) return false;
	
	// pass completed declaration into return list
	funcs.insert( new FuncDecl{ name, tag, move(params), move(returns), move(forall) } );
	
	return true;
}

/// Parses a subexpression; returns true and adds the expression to exprs if found.
/// line must not be null.
bool parse_subexpr( char *&token, List<Expr>& exprs, CanonicalTypeMap& types ) {
	char *end = token;
	
	// Check for concrete type expression
	int t;
	if ( parse_int(end, t) ) {
		const ConcType* ty = get_canon<ConcType>( types, t );
		exprs.push_back( new VarExpr( ty ) );
		token = end;
		return true;
	}
	
	std::string name;

	// Check for named type expression
	if ( parse_named_type(end, name) ) {
		exprs.push_back( new VarExpr( get_canon<NamedType>( types, name ) ) );
		token = end;
		return true;
	}
	
	// Check for function call
	if ( ! parse_name(end, name) ) return false;
	match_whitespace(end);
	if ( ! match_char(end, '(') ) return false;
	
	// Read function args
	List<Expr> args;
	match_whitespace(end);
	while ( parse_subexpr(end, args, types) ) {
		match_whitespace(end);
	}
	
	// Find closing bracket
	if ( ! match_char(end, ')') ) return false;
	match_whitespace(end);
	
	exprs.push_back( new FuncExpr( name, move(args) ) );
	token = end;
	return true;
}

/// Parses an expression from line; returns true and adds the expression to 
/// exprs if found; will fail if given a valid expr that does not consume the 
/// whole line. line must not be null.
bool parse_expr( char *line, List<Expr>& exprs, CanonicalTypeMap& types ) {
	match_whitespace(line);
	return parse_subexpr(line, exprs, types) && is_empty(line);
}

bool parse_input( std::istream& in, FuncTable& funcs, List<Expr>& exprs, 
                  CanonicalTypeMap& types, Args& args ) {
	std::string line;
	std::string delim = "%%";
	unsigned n = 0;
	
	// parse declarations
	while ( std::getline(in, line) ) {
		++n;
		if ( is_empty(line) ) {
			if ( args.verbosity() == Args::Verbosity::Filtered 
					|| args.verbosity() == Args::Verbosity::Verbose ) {
				args.out() << line << std::endl;
			}
			continue;
		}
		if ( line == delim ) {
			if ( args.verbosity() == Args::Verbosity::Filtered 
					|| args.verbosity() == Args::Verbosity::Verbose ) {
				args.out() << line << std::endl;
			}
			break;
		}
		
		bool ok = parse_decl(const_cast<char*>(line.data()), funcs, types);
		if ( ! ok ) {
			std::cerr << "Invalid declaration [" << n << "]: \"" << line << "\"" << std::endl;
			return false;
		}

		if ( args.verbosity() == Args::Verbosity::Filtered 
				|| args.verbosity() == Args::Verbosity::Verbose ) {
			args.out() << line << std::endl;
		}
	}
	
	// parse expressions
	while ( std::getline(in, line) ) {
		++n;
		if ( is_empty(line) ) {
			if ( args.verbosity() == Args::Verbosity::Verbose ) {
				args.out() << line << std::endl;
			}
			continue;
		}
		
		bool ok = parse_expr(const_cast<char*>(line.data()), exprs, types);
		if ( ! ok ) {
			std::cerr << "Invalid expression [" << n << "]: \"" << line << "\"" << std::endl;
			return false;
		}

		if ( args.verbosity() == Args::Verbosity::Verbose ) {
			args.out() << line << std::endl;
		}
	}
	
	return true;
}
