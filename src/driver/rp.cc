#include "args.h"
#include "metrics.h"
#include "parser.h"

#include "ast/expr.h"
#include "ast/type.h"
#include "data/clock.h"
#include "data/list.h"
#include "resolver/canonical_type_map.h"
#include "resolver/conversion.h"
#include "resolver/env.h"
#include "resolver/func_table.h"
#include "resolver/interpretation.h"
#include "resolver/resolver.h"

int main(int argc, char **argv) {
	Args args{argc, argv};
	std::ostream& out = args.out();
	
	ValidEffect on_valid = []( const Expr* e, const Interpretation* i ) {};
	InvalidEffect on_invalid = []( const Expr* e ) {};
	AmbiguousEffect on_ambiguous = 
		[]( const Expr* e, List<Interpretation>::const_iterator i, 
		    List<Interpretation>::const_iterator end ) {};
	UnboundEffect on_unbound = []( const Expr* e, const std::vector<TypeClass>& cs ) {};

	switch ( args.filter() ) {
	case Args::Filter::None:
		if ( args.quiet() ) break;

		on_invalid = [&out]( const Expr* e ) {
			out << "\nERROR: no valid resolution for " << *e << std::endl;
		};

		if ( args.testing() ) {
			on_ambiguous = [&out]( const Expr* e, List<Interpretation>::const_iterator i, 
					List<Interpretation>::const_iterator end ) {
				out << "\nERROR: ambiguous resolution for " << *e << std::endl;
			};
		} else {
			on_ambiguous = [&out]( const Expr* e, List<Interpretation>::const_iterator i, 
					List<Interpretation>::const_iterator end ) {
				out << "\nERROR: ambiguous resolution for " << *e << "\n"
					<< "       candidates are:";

				for(; i != end; ++i) {
					out << "\n";
					(*i)->write( out, ASTNode::Print::Default );
				}
				out << std::endl;
			};
		}

		on_unbound = [&out]( const Expr* e, const std::vector<TypeClass>& cs ) {
			out << "\nERROR: unbound type variables in " << *e << ":";
			for ( const TypeClass& c : cs ) { out << ' ' << c; }
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

		on_unbound = [&out]( const Expr* e, const std::vector<TypeClass>& cs ) {
			e->write( out, ASTNode::Print::InputStyle );
			out << std::endl;
		};
		break;
	}

	if ( args.per_prob() ) {
		on_valid = [&out]( const Expr* e, const Interpretation* i ) {
			out << i->env.numAssertions();
		};
	} else if ( args.quiet() ) {
		// empty valid effect is correct in this case
	} else if ( args.testing() ) {
		on_valid = [&out]( const Expr* e, const Interpretation* i ) {
			out << "\n";
			i->write( out, ASTNode::Print::Concise );
			out << std::endl;
		};
	} else if ( args.filter() != Args::Filter::None ) {
		if ( args.filter() == Args::Filter::Invalid ) {
			on_valid = [&out]( const Expr* e, const Interpretation* i ) {
				e->write( out, ASTNode::Print::InputStyle );
				out << std::endl;
			};
		}
	} else {
		on_valid = [&out]( const Expr* e, const Interpretation* i ) {
			out << "\n" << *i << std::endl;
		};
	}

	volatile std::clock_t start, end;
	Resolver resolver{ on_valid, on_invalid, on_ambiguous, on_unbound };
	Metrics metrics{};
	start = std::clock();
	run_input( args.in(), resolver, args, metrics );
	end = std::clock();

	if ( args.bench() ) {
		if ( ! args.metrics_only() ) {
			out << "\n" << ms_between(start, end) << ",";
		}
		out << metrics.n_decls << "," 
		    << metrics.max_decls_per_name << ","
		    << metrics.n_exprs << "," 
		    << metrics.n_subexprs << "," 
			<< (((double)metrics.sum_depth)/metrics.n_exprs) << "," // avg. max depth
			<< metrics.max_depth << ","
			<< metrics.n_assns << ","
			<< metrics.n_poly_decls << ","
			<< metrics.max_assns << std::endl;
	}
	
	collect();
}
