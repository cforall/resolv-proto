#pragma once

// Copyright (c) 2015 University of Waterloo
//
// The contents of this file are covered under the licence agreement in 
// the file "LICENCE" distributed with this repository.

#include <iostream>

#include "args.h"
#include "metrics.h"

#include "ast/expr.h"
#include "ast/type.h"
#include "data/list.h"
#include "resolver/canonical_type_map.h"
#include "resolver/func_table.h"

class Resolver;

/// Parses input according to the following format:
///
/// start := <scope>
/// scope := <decl>* "%%\n" <body> 
/// body := (<block> | <resolv_expr>)*
/// block := "{" NL+ <scope> "}" NL+
/// decl := <function_decl> | <var_decl>
/// function_decl := (<type>" ")* <name>("-"<tag>)? (" "<type>)* <type_assertion>* NL+
/// var_decl := <type> " &"<name>("-"<tag>)?
/// type_assertion := "|" (<type>" ")* ( <name> (" "<type>)* ) | ( "&"<name> )
/// resolv_expr := <subexpr> NL+
/// subexpr := <conc_type> | <named_type> | <func_type> 
///            | <name> " (" (" "<subexpr>)* " )" | "&"<name>
/// name := [a-z_$][A-Za-z_0-9]*
/// tag := <name>
/// type := <conc_type> | <named_type> | <poly_type> | <func_type>
/// conc_type := "-"?[0-9]+
/// named_type := "#"[A-Za-z_$][A-Za-z_0-9]*("<" <type> (" "<type>)* ">")?
/// poly_type := [A-Z][A-Za-z_0-9]*
/// func_type := "[" <type>* ":" <type>* "]"
/// NL := "\n" | "//" . ~ "\n" 
///
/// Semantically, types are given numeric identifiers, and also stand in for 
/// variables of that type; the conversion cost from x => y is |x-y|; this is 
/// a safe conversion if x < y, unsafe if x > y. 
///
/// Functions are named conversions from one list of types to another (as in 
/// C, the list of types before the function name are the return types, while 
/// the list after are the parameter types); multiple functions may be given  
/// the same name, these may be disambiguated to the user of this program by  
/// providing a tag name that is unique. Both function names and tags are 
/// lowercase alphanumeric ASCII strings starting with a letter.
///
/// Functions may declare polymorphic type variables; these are named by 
/// lowercase ASCII strings starting with an uppercase letter. Multiple 
/// instances of the same name in the same function declaration refer to the 
/// same type variable, while instances of the same name in different function 
/// declarations refer to distinct type variables. A type variable may bind to 
/// any concrete type in an expression, but this binding must be consistent 
/// across the arguments of a single function call in an expression.  
///
/// Expressions to be resolved are (possibly recursive) function invocations 
/// with the leaf nodes represented by type identifiers corresponding to 
/// variables. Using a function as a variable can be accomplished by prefixing 
/// it with an ampersand.
///
/// Functions and expressions may be grouped into lexically-scoped blocks by 
/// using curly braces. Only in-scope declarations can be used within a block
///
/// Calls resolver on a pipelined basis while parsing input
void run_input( std::istream& in, Resolver& resolver, Args& args, Metrics& metrics );
