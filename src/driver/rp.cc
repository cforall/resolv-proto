#include <ctime>

#include "args.h"
#include "parser.h"

#include "ast/expr.h"
#include "ast/type.h"
#include "data/list.h"
#include "resolver/canonical_type_map.h"
#include "resolver/conversion.h"
#include "resolver/env.h"
#include "resolver/func_table.h"
#include "resolver/interpretation.h"
#include "resolver/resolver.h"

long ms_between(std::clock_t start, std::clock_t end) {
	return (end - start) / (CLOCKS_PER_SEC / 1000);
}

int main(int argc, char **argv) {
	Args args(argc, argv);
	FuncTable funcs;
	List<Expr> exprs;
	CanonicalTypeMap types;
	std::ostream& out = args.out();
	
	if ( ! parse_input( args.in(), funcs, exprs, types, args ) ) return 1;
	
	ConversionGraph conversions = make_conversions( types );
	
	InvalidEffect on_invalid = []( const Expr* e ) {};
	AmbiguousEffect on_ambiguous = 
		[]( const Expr* e, List<Interpretation>::const_iterator i, 
		    List<Interpretation>::const_iterator end ) {};
	UnboundEffect on_unbound = []( const Expr* e, const List<TypeClass>& cs ) {};

	switch ( args.filter() ) {
	case Args::Filter::None:
		if ( args.quiet() ) break;

		on_invalid = [&out]( const Expr* e ) {
			out << "ERROR: no valid resolution for " << *e << std::endl;
		};

		on_ambiguous = [&out]( const Expr* e, List<Interpretation>::const_iterator i, 
				List<Interpretation>::const_iterator end ) {
			out << "ERROR: ambiguous resolution for " << *e << "\n"
				<< "       candidates are:\n";

			for(; i != end; ++i) {
				out << "\n" << **i;
			}
			out << std::endl;
		};

		on_unbound = [&out]( const Expr* e, const List<TypeClass>& cs ) {
			out << "ERROR: unbound type variables in " << *e << ":";
			for ( const TypeClass* c : cs ) { out << ' ' << *c; }
			out << std::endl;
		};
		break;
	case Args::Filter::Invalid:
		break;
	case Args::Filter::Unambiguous:
		on_ambiguous = [&out]( const Expr* e, List<Interpretation>::const_iterator i, 
				List<Interpretation>::const_iterator end ) {
			e->write( out, ASTNode::Print::InputStyle );
			out << std::endl;
		};
		break;
	case Args::Filter::Resolvable:
		on_invalid = [&out]( const Expr* e ) {
			e->write( out, ASTNode::Print::InputStyle );
			out << std::endl;
		};

		on_unbound = [&out]( const Expr* e, const List<TypeClass>& cs ) {
			e->write( out, ASTNode::Print::InputStyle );
			out << std::endl;
		};
		break;
	}

	Resolver resolve{ conversions, funcs, on_invalid, on_ambiguous, on_unbound };

	volatile std::clock_t start, end;

	if ( args.quiet() ) {
		start = std::clock();
		// loop without printing
		for ( auto e = exprs.begin(); e != exprs.end(); ++e ) {
			resolve( *e );
		}
		end = std::clock();
	} else if ( args.filter() != Args::Filter::None ) {
		start = std::clock();
		// loop only printing un-filtered 
		for ( auto e = exprs.begin(); e != exprs.end(); ++e ) {
			const Interpretation *i = resolve( *e );
			if ( args.filter() == Args::Filter::Invalid && i->is_valid() ) {
				(*e)->write( out, ASTNode::Print::InputStyle );
				out << std::endl;
			}
		}
		end = std::clock();
	} else {
		start = std::clock();
		// loop printing all interpretations
		for ( auto e = exprs.begin(); e != exprs.end(); ++e ) {
			out << "\n";
			const Interpretation *i = resolve( *e );
			if ( i->is_valid() ) {
				out << *i << std::endl;
				*e = i->expr;
			}
		}
		end = std::clock();
	}

	if ( args.bench() ) {
		// num_decls,num_exprs,runtime(ms)
		out << funcs.size() << "," << exprs.size() << "," << ms_between(start, end) << std::endl;
	}
	
	collect();
}
