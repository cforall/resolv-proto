#include <iostream>

#include "conversion.h"
#include "expr.h"
#include "func_table.h"
#include "interpretation.h"
#include "parser.h"
#include "resolver.h"
#include "type.h"

/// Effect for invalid expression
void on_invalid( const Expr* e ) {
	std::cout << "ERROR: no valid resolution for " << *e << std::endl;
}

/// Effect for ambiguous resolution
void on_ambiguous( const Expr* e, InterpretationList::iterator i, 
                   InterpretationList::iterator end ) {
	std::cout << "ERROR: ambiguous resolution for " << *e << "\n"
	          << "       candidates are:" << std::endl;
	
	for(; i != end; ++i) {
		std::cout << "\n" << *i;
	}
}

int main(int argc, char **argv) {
	FuncTable funcs;
	List<Expr> exprs;
	SortedSet<ConcType> types;
	
	if ( ! parse_input( std::cin, funcs, exprs, types ) ) return 1;
	
	std::cout << std::endl;
	for ( auto&& func : funcs ) { std::cout << *func << std::endl; }
	std::cout << "%%" << std::endl;
	for ( auto&& expr : exprs ) { std::cout << *expr << std::endl; }
	
	ConversionGraph conversions = make_conversions( types );
	
	std::cout << std::endl << conversions;
	
	Resolver resolve{ conversions, funcs, on_invalid, on_ambiguous };
	for ( auto e = exprs.begin(); e != exprs.end(); ++e ) {
		std::cout << "\n";
		const Interpretation *i = resolve( *e );
		if ( i->is_valid() ) {
			std::cout << i;
			*e = i->expr;
		} 
	}
	collect();
}
