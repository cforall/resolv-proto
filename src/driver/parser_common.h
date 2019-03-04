#pragma once

// Copyright (c) 2015 University of Waterloo
//
// The contents of this file are covered under the licence agreement in 
// the file "LICENCE" distributed with this repository.

#include <climits>
#include <cstdlib>
#include <string>

/// Skips all whitespace at token; mutates parameter and returns true if any change.
/// token must not be null.
static bool match_whitespace(char const *&token) {
	if ( ' ' == *token || '\t' == *token ) ++token;
	else return false;
	
	while ( ' ' == *token || '\t' == *token ) ++token;
	
	return true;
}

/// Matches c at token, returning true and advancing token if found.
/// token must not be null.
static bool match_char(char const *&token, char c) {
	if ( *token == c ) {
		++token;
		return true;
	}
	return false;
}

/// Matches s at token, returning true and advancing token if found. 
/// token must not be null.
static bool match_string(char const *&token, const char *s) {
	const char *end = token;
	
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

/// Checks if a line is blank (empty line optionally preceded by whitespace)
static bool is_blank(const char *token) {
	match_whitespace( token );
	return is_empty( token );
}
static bool is_blank(const std::string& line) { return is_blank( line.data() ); }

/// Checks if a line only contains the given string (possibly preceded and followed by whitespace)
static bool line_matches(const char* token, const char* s) {
	match_whitespace( token );
	if ( ! match_string( token, s ) ) return false;
	match_whitespace( token );
	return is_empty( token );
}
static bool line_matches(const std::string& line, const char* s) {
	return line_matches( line.data(), s );
}

/// Parses an integer, returning true, storing result into ret, 
/// and incrementing token if so. token must not be null. 
static bool parse_int(char const *&token, int& ret) {
	char *end;
	
	long val = strtol( token, &end, 10 );
	if ( end == token ) return false;   // parse failed
	if ( val > INT_MAX ) return false;  // out of smaller range
	
	ret = val;
	token = end;
	return true;
}

/// Parses a non-negative integer, returning true, storing result into ret, 
/// and incrementing token if so. token must not be null. 
static bool parse_unsigned(char const *&token, unsigned& ret) {
	char *end;
	
	unsigned long val = strtoul( token, &end, 10 );
	if ( end == token ) return false;   // parse failed
	if ( val > UINT_MAX ) return false;  // out of smaller range
	
	ret = val;
	token = end;
	return true;
}

/// Parses a floating point number, returning true, storing result into ret,
/// and incrementing token if so. token must not be null.
static bool parse_float(char const *&token, double& ret) {
	char *end;

	ret = strtod( token, &end );
	if ( end == token ) return false;  // parse failed

	token = end;
	return true;
}
