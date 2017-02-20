#pragma once

#include <climits>
#include <cstdlib>
#include <string>

/// Skips all whitespace at token; mutates parameter and returns true if any change.
/// token must not be null.
static bool match_whitespace(char *&token) {
	if ( ' ' == *token || '\t' == *token ) ++token;
	else return false;
	
	while ( ' ' == *token || '\t' == *token ) ++token;
	
	return true;
}

/// Matches c at token, returning true and advancing token if found.
/// token must not be null.
static bool match_char(char *&token, char c) {
	if ( *token == c ) {
		++token;
		return true;
	}
	return false;
}

/// Matches s at token, returning true and advancing token if found. 
/// token must not be null.
static bool match_string(char *&token, const char *s) {
	char *end = token;
	
	while ( *s && *s == *end ) { ++s; ++end; }
	if ( *s ) return false;  // didn't match whole string

	token = end;
	return true;
}

/// Checks if a line is empty (includes ends with a comment)
static bool is_empty(const char *token) {
	return *token == '\0' || (token[0] == '/' && token[1] == '/');
}
static bool is_empty(const std::string& line) { return is_empty( line.data() ); }

/// Parses an integer, returning true, storing result into ret, 
/// and incrementing token if so. token must not be null. 
static bool parse_int(char *&token, int& ret) {
	char *end = token;
	
	long val = strtol( token, &end, 10 );
	if ( end == token ) return false;   // parse failed
	if ( val > INT_MAX ) return false;  // out of smaller range
	
	ret = val;
	token = end;
	return true;
}

/// Parses a non-negative integer, returning true, storing result into ret, 
/// and incrementing token if so. token must not be null. 
static bool parse_unsigned(char *&token, unsigned& ret) {
	char *end = token;
	
	unsigned long val = strtoul( token, &end, 10 );
	if ( end == token ) return false;   // parse failed
	if ( val > UINT_MAX ) return false;  // out of smaller range
	
	ret = val;
	token = end;
	return true;
}

/// Parses a floating point number, returning true, storing result into ret,
/// and incrementing token if so. token must not be null.
static bool parse_float(char *&token, double& ret) {
	char *end = token;

	ret = strtod( token, &end );
	if ( end == token ) return false;  // parse failed

	token = end;
	return true;
}
