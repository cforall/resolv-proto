#include "args.h"
#include "parser.h"

#include "ast/expr.h"
#include "ast/type.h"
#include "resolver/canonical_type_map.h"
#include "resolver/conversion.h"
#include "resolver/func_table.h"
#include "resolver/interpretation.h"
#include "resolver/resolver.h"

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
		[]( const Expr* e, List<TypedExpr>::const_iterator i, List<TypedExpr>::const_iterator end ) {};
	UnboundEffect on_unbound = []( const Expr* e, const TypeBinding& tb ) {};

	switch ( args.filter() ) {
	case Args::Filter::None:
		if ( args.quiet() ) break;

		on_invalid = [&out]( const Expr* e ) {
			out << "ERROR: no valid resolution for " << *e << std::endl;
		};

		on_ambiguous = [&out]( const Expr* e, List<TypedExpr>::const_iterator i, 
				List<TypedExpr>::const_iterator end ) {
			out << "ERROR: ambiguous resolution for " << *e << "\n"
				<< "       candidates are:\n";

			for(; i != end; ++i) {
				out << "\n" << **i;
			}
			out << std::endl;
		};

		on_unbound = [&out]( const Expr* e, const TypeBinding& tb ) {
			out << "ERROR: unbound type variable" << (tb.unbound() > 1 ? "s" : "") 
					<< " on " << tb.name << tb << std::endl;
		};
		break;
	case Args::Filter::Invalid:
		break;
	case Args::Filter::Unambiguous:
		on_ambiguous = [&out]( const Expr* e, List<TypedExpr>::const_iterator i, 
				List<TypedExpr>::const_iterator end ) {
			e->write( out, ASTNode::Print::InputStyle );
			out << std::endl;
		};
		break;
	case Args::Filter::Resolvable:
		on_invalid = [&out]( const Expr* e ) {
			e->write( out, ASTNode::Print::InputStyle );
			out << std::endl;
		};

		on_unbound = [&out]( const Expr* e, const TypeBinding& tb ) {
			e->write( out, ASTNode::Print::InputStyle );
			out << std::endl;
		};
		break;
	}

	Resolver resolve{ conversions, funcs, on_invalid, on_ambiguous, on_unbound };

	if ( args.quiet() || args.filter() != Args::Filter::None ) {
		// loop only printing un-filtered 
		for ( auto e = exprs.begin(); e != exprs.end(); ++e ) {
			const Interpretation *i = resolve( *e );
			if ( args.filter() == Args::Filter::Invalid && i->is_valid() ) {
				(*e)->write( out, ASTNode::Print::InputStyle );
				out << std::endl;
			}
		}
	} else {
		// loop printing all interpretations
		for ( auto e = exprs.begin(); e != exprs.end(); ++e ) {
			out << "\n";
			const Interpretation *i = resolve( *e );
			if ( i->is_valid() ) {
				out << *i << std::endl;
				*e = i->expr;
			}
		}
	}
	
	collect();
}
