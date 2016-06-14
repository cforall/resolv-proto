
/// Driver for resolver prototype;
/// Reads in files according to the following format:
///
/// start := <function_decl>+ "%%\n" <resolv_expr>+
/// function_decl := <name>("-"<tag>)? (" "<type>)* " ->" (" "<type>)* "\n"
/// resolv_expr := (<type>" = ")? <subexpr> "\n"
/// subexpr := <type> | <name> " (" (" "<subexpr>)* " )"
/// name := [a-z]+
/// tag := [a-z]+
/// type := "0" | [1-9][0-9]*
///
/// Semantically, types are given numeric identifiers, and also stand in for 
/// variables of that type; the conversion cost from x => y is |x-y|; this is 
/// a safe conversion if x < y, unsafe if x > y. 
///
/// Functions are named conversions from one list of types to another; 
/// multiple functions may be given the same name, these may be disambiguated 
/// to the user of this program by providing a tag name that is unique; if no 
/// tag is provided one will be autogenerated from the parameter types. 
/// Otherwise, both function names and tags are lowercase ASCII strings.
///
/// Expressions to be resolved are (possibly recursive) function invocations 
/// with the leaf nodes represented by type identifiers corresponding to 
/// variables. These expressions may optionally be resolved in a typed context 
/// with the "<type> = " assignment form, simulating assignment to a variable. 
int main(int argc, char **argv) {
	
}
