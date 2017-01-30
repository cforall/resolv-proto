#include "args.h"
#include "parser.h"

#include "canonical_type_map.h"
#include "conversion.h"
#include "func_table.h"
#include "interpretation.h"
#include "resolver.h"
#include "ast/expr.h"
#include "ast/type.h"

int main(int argc, char **argv) {
	Args args(argc, argv);
	FuncTable funcs;
	List<Expr> exprs;
	CanonicalTypeMap types;
	
	if ( ! parse_input( args.in(), funcs, exprs, types ) ) return 1;
	
	if ( args.verbose() ) {
		args.out() << std::endl;
		for ( auto&& func : funcs ) { args.out() << *func << std::endl; }
		args.out() << "%%" << std::endl;
		for ( auto&& expr : exprs ) { args.out() << *expr << std::endl; }
	}
	
	ConversionGraph conversions = make_conversions( types );
	
	Resolver resolve{ conversions, funcs, 
					  [&args]( const Expr* e ) {
						  if ( args.quiet() ) return;
						  args.out() << "ERROR: no valid resolution for " << *e << std::endl;
					  }, 
					  [&args]( const Expr* e, List<TypedExpr>::const_iterator i, 
                               List<TypedExpr>::const_iterator end ) {
						  if ( args.quiet() ) return;
						  args.out() << "ERROR: ambiguous resolution for " << *e << "\n"
	                                 << "       candidates are:\n";
	
	                      for(; i != end; ++i) {
		                      args.out() << "\n" << **i;
	                      }
						  args.out() << std::endl;
					  },
					  [&args]( const Expr*, const TypeBinding& tb ) {
						  if ( args.quiet() ) return;
						  args.out() << "ERROR: unbound type variable"
						             << (tb.unbound() > 1 ? "s" : "") 
									 << " on " << tb.name << tb << std::endl;
					  } };
	
	for ( auto e = exprs.begin(); e != exprs.end(); ++e ) {
		if ( ! args.quiet() ) args.out() << "\n";
		const Interpretation *i = resolve( *e );
		if ( i->is_valid() ) {
			if ( ! args.quiet() ) args.out() << *i << std::endl;
			*e = i->expr;
		} 
	}
	collect();
}
