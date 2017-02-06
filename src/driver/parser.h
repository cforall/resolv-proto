#pragma once

#include <iostream>

#include "ast/expr.h"
#include "ast/type.h"
#include "data/list.h"
#include "resolver/canonical_type_map.h"
#include "resolver/func_table.h"

/// Parses input according to the following format:
///
/// start := NL* <function_decl>* "%%\n" <resolv_expr>*
/// function_decl := (<type>" ")* <name>("-"<tag>)? (" "<type>)* <type_assertion>* NL+
/// type_assertion := "|" (<type>" ")* <name> (" "<type>)*
/// resolv_expr := <subexpr> NL+
/// subexpr := <conc_type> | <named_type> | <name> " (" (" "<subexpr>)* " )"
/// name := [a-z][a-z0-9]*
/// tag := <name>
/// type := <conc_type> | <named_type> | <poly_type>
/// conc_type := "-"?[0-9]+
/// named_type := "#"[A-Za-z_][A-Za-z_0-9]*
/// poly_type := [A-Z][a-z0-9]*
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
/// variables. 
///
/// Returns true and sets funcs, exprs & types if appropriate, 
/// prints errors otherwise.
bool parse_input( std::istream& in, FuncTable& funcs, List<Expr>& exprs, 
                  CanonicalTypeMap& types );
