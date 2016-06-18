#include <cstdlib>
#include <iostream>
#include <string>

#include "conversion.h"
#include "decl.h"
#include "expr.h"
#include "type.h"
#include "utility.h"

/// Skips all whitespace at token; mutates parameter and returns true if any change.
/// token must not be null.
bool match_whitespace(char *&token) {
	if ( ' ' == *token || '\t' == *token ) ++token;
	else return false;
	
	while ( ' ' == *token || '\t' == *token ) ++token;
	
	return true;
}

/// Matches c at token, returning true and advancing token if found.
/// token must not be null.
bool match_char(char *&token, char c) {
	if ( *token == c ) {
		++token;
		return true;
	}
	return false;
}

/// Checks if a C string is empty
bool is_empty(char *token) { return *token == '\0'; }

/// Parses an integer, returning true, storing result into ret, 
/// and incrementing token if so. token must not be null. 
bool parse_int(char *&token, int& ret) {
	char *end = token;
	
	if ( '-' == *end ) ++end;
	
	if ( '0' <= *end && *end <= '9' ) ++end;
	else return false;
	
	while ( '0' <= *end && *end <= '9' ) ++end;
	
	char tmp = *end;
	*end = '\0';
	ret = std::atoi(token);
	*end = tmp;
	
	token = end;
	return true;
}

/// Parses a name (non-empty lowercase ASCII string), returning true, 
/// storing result into ret, and incrementing token if so. 
/// token must not be null.
bool parse_name(char *&token, std::string& ret) {
	char *end = token;
	
	if ( 'a' <= *end && *end <= 'z' ) ++end;
	else return false;
	
	while ( 'a' <= *end && *end <= 'z' ) ++end;
	
	ret.assign( token, (end-token) );
	
	token = end;
	return true;
}

/// Parses a declaration from line; returns true and adds the declaration to 
/// decls if found; will fail if given a valid decl that does not consume the 
/// whole line. line must not be null.
bool parse_decl(char *line, List<Decl>& decls, SortedSet<ConcType>& types) {
	RefList<Type> returns, params;
	std::string name;
	std::string tag;
	int t;
	bool saw_tag = false;
	
	// parse return types
	match_whitespace(line);
	while ( parse_int(line, t) ) {
		returns.push_back( get_ref( types, make_ptr<ConcType>( t ) ) );
		match_whitespace(line);
	}
	
	// parse name
	if ( ! parse_name(line, name) ) return false;
	
	// optionally parse tag
	if ( match_char(line, '-') ) {
		if ( parse_name(line, tag) ) {
			saw_tag = true;
		} else {
			--line;  // might have been subsequent negative-valued type
		}
	}
	
	// parse parameters
	match_whitespace(line);
	while( parse_int(line, t) ) {
		params.push_back( get_ref( types, make_ptr<ConcType>( t ) ) );
		match_whitespace(line);
	}
	
	// check line consumed
	if ( ! is_empty(line) ) return false;
	
	// pass completed declaration into return list
	if ( saw_tag ) {
		decls.push_back( make<FuncDecl>(name, tag, params, returns) );
	} else {
		decls.push_back( make<FuncDecl>(name, params, returns) );
	}
	
	return true;
}

/// Parses a subexpression; returns true and adds the expression to exprs if found.
/// line must not be null.
bool parse_subexpr(char *&token, List<Expr>& exprs, SortedSet<ConcType>& types) {
	char *end = token;
	
	// Check for type expression
	int t;
	if ( parse_int(end, t) ) {
		Ref<Type> ty = get_ref( types, make_ptr<ConcType>( t ) );
		exprs.push_back( make<VarExpr>( ty ) );
		token = end;
		return true;
	}
	
	// Check for function call
	std::string name;
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
	
	exprs.push_back( make<FuncExpr>( name, std::move(args) ) );
	token = end;
	return true;
}

/// Parses an expression from line; returns true and adds the expression to 
/// exprs if found; will fail if given a valid expr that does not consume the 
/// whole line. line must not be null.
bool parse_expr(char *line, List<Expr>& exprs, SortedSet<ConcType>& types) {
	match_whitespace(line);
	return parse_subexpr(line, exprs, types) && is_empty(line);
}

/// Parses input according to the format described on main.
/// Returns true and sets decls and exprs if appropriate, prints errors otherwise
bool parse_input( std::istream& in, List<Decl>& decls, List<Expr>& exprs, 
                  SortedSet<ConcType>& types ) {
	std::string line;
	std::string delim = "%%";
	unsigned n = 0;
	
	// parse declarations
	while ( std::getline(in, line) ) {
		++n;
		if ( line == delim ) break;
		
		bool ok = parse_decl(const_cast<char*>(line.data()), decls, types);
		if ( ! ok ) {
			std::cerr << "Invalid declaration [" << n << "]: \"" << line << "\"" << std::endl;
			return false;
		}
	}
	
	// parse expressions
	while ( std::getline(in, line) ) {
		++n;
		bool ok = parse_expr(const_cast<char*>(line.data()), exprs, types);
		if ( ! ok ) {
			std::cerr << "Invalid expression [" << n << "]: \"" << line << "\"" << std::endl;
			return false;
		}
	}
	
	return true;
}

/// Driver for resolver prototype;
/// Reads in files according to the following format:
///
/// start := <function_decl>+ "%%\n" <resolv_expr>+
/// function_decl := (<type>" ")* <name>("-"<tag>)? (" "<type>)* "\n"
/// resolv_expr := <subexpr> "\n"
/// subexpr := <type> | <name> " (" (" "<subexpr>)* " )"
/// name := [a-z]+
/// tag := [a-z]+
/// type := "-"?[0-9]+
///
/// Semantically, types are given numeric identifiers, and also stand in for 
/// variables of that type; the conversion cost from x => y is |x-y|; this is 
/// a safe conversion if x < y, unsafe if x > y. 
///
/// Functions are named conversions from one list of types to another (as in 
/// C, the list of types before the function name are the return types, while 
/// the list after are the parameter types); multiple functions may be given  
/// the same name, these may be disambiguated to the user of this program by  
/// providing a tag name that is unique; if no tag is provided one will be  
/// autogenerated from the parameter types. Otherwise, both function names and 
/// tags are lowercase ASCII strings.
///
/// Expressions to be resolved are (possibly recursive) function invocations 
/// with the leaf nodes represented by type identifiers corresponding to 
/// variables. 
int main(int argc, char **argv) {
	List<Decl> decls;
	List<Expr> exprs;
	SortedSet<ConcType> types;
	
	if ( ! parse_input( std::cin, decls, exprs, types ) ) return 1;
	
	std::cout << std::endl;
	for ( auto& decl : decls ) { std::cout << *decl << std::endl; }
	std::cout << "%%" << std::endl;
	for ( auto& expr : exprs ) { std::cout << *expr << std::endl; }
	
	ConversionGraph conversions = make_conversions( types );
	
	std::cout << std::endl << conversions;
}
