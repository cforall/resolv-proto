#include "args.h"
#include "canonical_type_map.h"
#include "conversion.h"
#include "expr.h"
#include "func_table.h"
#include "interpretation.h"
#include "parser.h"
#include "resolver.h"
#include "type.h"

int main(int argc, char **argv) {
	Args args(argc, argv);
	FuncTable funcs;
	List<Expr> exprs;
	CanonicalTypeMap types;
	
	if ( ! parse_input( args.in(), funcs, exprs, types ) ) return 1;
	
	args.out() << std::endl;
	for ( auto&& func : funcs ) { args.out() << *func << std::endl; }
	args.out() << "%%" << std::endl;
	for ( auto&& expr : exprs ) { args.out() << *expr << std::endl; }
	
	ConversionGraph conversions = make_conversions( types );
	
	args.out() << std::endl << conversions;
	
	Resolver resolve{ conversions, funcs, 
					  [&args]( const Expr* e ){
						  args.out() << "ERROR: no valid resolution for " << *e << std::endl;
					  }, 
					  [&args]( const Expr* e, InterpretationList::iterator i, 
                               InterpretationList::iterator end ) {
						  std::cout << "ERROR: ambiguous resolution for " << *e << "\n"
	                                << "       candidates are:" << std::endl;
	
	                      for(; i != end; ++i) {
		                      std::cout << "\n" << *i;
	                      }
					  } };
	
	for ( auto e = exprs.begin(); e != exprs.end(); ++e ) {
		args.out() << "\n";
		const Interpretation *i = resolve( *e );
		if ( i->is_valid() ) {
			args.out() << *i << std::endl;
			*e = i->expr;
		} 
	}
	collect();
}
