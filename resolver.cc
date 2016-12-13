#include <algorithm>
#include <cassert>
#include <vector>

#include "resolver.h"

#include "binding.h"
#include "cost.h"
#include "cow.h"
#include "debug.h"
#include "eager_merge.h"
#include "environment.h"
#include "expand_conversions.h"
#include "expr.h"
#include "func_table.h"
#include "interpretation.h"
#include "nway_merge.h"
#include "typed_expr_visitor.h"
#include "unify.h"

/// Return an interpretation for all zero-arg functions in funcs
template<typename Funcs>
InterpretationList matchFuncs( const Funcs& funcs, bool topLevel = false ) {
	InterpretationList results;
	
	// find functions with no parameters, skipping remaining checks if none
	auto withNParams = funcs.find( 0 );
	if ( withNParams == funcs.end() ) {
		DBG( "  with<0>params: []" );
		return results;
	}
	DBG( "  with<0>params: " << print_all_deref( withNParams() ) );
	
	// attempt to match functions to arguments
	for ( auto&& func : withNParams() ) {
		// skip functions returning no values, unless at top level
		if ( ! topLevel && func->returns()->size() == 0 ) continue;
			
		// create new zero-cost interpretation for resolved call
		results.push_back( new Interpretation( new CallExpr( func ) ) );
	}
	
	return results;
}

/// Return an interpretation for all single-argument interpretations in funcs
template<typename Funcs>
InterpretationList matchFuncs( const Funcs& funcs, InterpretationList&& args, 
                               bool topLevel = false ) {
	InterpretationList results;

	for ( const Interpretation* arg : args ) {
		DBG( "  arg: " << *arg );

		// find function with appropriate number of parameters, skipping  *argTypes[k] != *params[i]arg if none
		auto n = arg->type()->size();
		auto withNParams = funcs.find( n );
		if ( withNParams == funcs.end() ) {
			DBG( "    with<" << n << ">params: []" );
			continue;
		}
		DBG( "    with<" << n << ">params: " << print_all_deref( withNParams() ) );

		// attempt to match functions to arguments
		for ( const auto& func : withNParams() ) {
			// skip functions returning no values, unless at top level
			if ( ! topLevel && func->returns()->size() == 0 ) continue;

			// Environment for call bindings
			Cost cost = arg->cost;
			unique_ptr<TypeBinding> localEnv{ make_binding( func ) };
			cow_ptr<Environment> env = arg->env;

			// skip functions thaCost& costt don't match the parameter types
			if ( n == 1 ) { // concrete type arg
				if ( ! unify( func->params()[0], arg->type(), cost, *localEnv, env ) ) continue;
			} else {  // tuple type arg
				const List<Type>& argTypes = as<TupleType>( arg->type() )->types();
				for ( unsigned i = 0; i < n; ++i ) {
					if ( ! unify( func->params()[i], argTypes[i], cost, *localEnv, env ) ) 
						goto nextFunc;
				}
			}

			// create new interpretation for resolved call
			results.push_back( new Interpretation(
				new CallExpr( func, List<TypedExpr>{ arg->expr }, move(localEnv) ),
				move(cost),
				move(env)
			) );
		nextFunc:; }
	}

	return results;
}

/// List of argument combinations
typedef std::vector< std::pair<Cost, InterpretationList> > ComboList;

/// Checks if a list of argument types match a list of parameter types.
/// The argument types may contain tuple types, which should be flattened; the parameter 
/// types will not. The two lists should be the same length after flattening.
bool argsMatchParams( const InterpretationList& args, const List<Type>& params, 
                      Cost& cost, TypeBinding& localEnv, cow_ptr<Environment>& env ) {
	unsigned i = 0;
	for ( unsigned j = 0; j < args.size(); ++j ) {
		// merge in argument environment
		if ( ! merge( env, args[j]->env ) ) return false;
		// test unification of parameters
		unsigned m = args[j]->type()->size();
		if ( m == 1 ) {
			if ( ! unify( params[i], args[j]->type(), cost, localEnv, env ) ) return false;
			++i;
		} else {
			const List<Type>& argTypes = as<TupleType>( args[j]->type() )->types();
			for ( unsigned k = 0; k < m; ++k ) {
				if ( ! unify( params[i], argTypes[k], cost, localEnv, env ) ) return false;
				++i;
			}
		}
	}
	return true;
}

/// Create a list of argument expressions from a list of interpretations
List<TypedExpr> argsFrom( const InterpretationList& is ) {
	List<TypedExpr> args;
	for ( const Interpretation* i : is ) {
		args.push_back( i->expr );
	}
	return args;
}

/// Return an interpretation for all matches between functions in funcs and 
/// argument packs in combos
template<typename Funcs>
InterpretationList matchFuncs( const Funcs& funcs, ComboList&& combos, bool topLevel = false ) {
	InterpretationList results;
	
	for ( const auto& combo : combos ) {
		DBG( "  combo: " << print_all_deref( combo.second ) );
		// find function with appropriate number of parameters, 
		// skipping combo if none
		auto n = 0;
		for ( const Interpretation* arg : combo.second ) {
			n += arg->type()->size();
		}
		auto withNParams = funcs.find( n );
		if ( withNParams == funcs.end() ) {
			DBG( "    with<" << n << ">Params: []" );
			continue;
		}
		DBG( "    with<" << n << ">Params: " << print_all_deref( withNParams() ) );
		
		// attempt to match functions to arguments
		for ( auto&& func : withNParams() ) {
			// skip functions returning no values, unless at top level
			if ( ! topLevel && func->returns()->size() == 0 ) continue;
			
			// Environment for call bindings
			Cost cost = combo.first;
			unique_ptr<TypeBinding> localEnv{ make_binding( func ) };
			cow_ptr<Environment> env; // initialized by argsMatchParams()

			// skip functions that don't match the parameter types
			if ( ! argsMatchParams( combo.second, func->params(), cost, *localEnv, env ) ) 
				continue;
			
			// create new interpretation for resolved call
			results.push_back( new Interpretation( 
				new CallExpr( func, argsFrom( combo.second ), move(localEnv) ),
				move(cost),
				move(env)
			) );
		}
	}
	
	return results;
}

/// Extracts the cost from an interpretation
struct interpretation_cost {
	const Cost& operator() ( const Interpretation* i ) { return i->cost; }	
};

/// Combination validator that fails on ambiguous interpretations in combinations
struct interpretation_unambiguous {
	bool operator() ( const std::vector< InterpretationList >& queues,
	                  const std::vector< unsigned >& inds ) {
		for( unsigned i = 0; i < queues.size(); ++i ) {
			if ( queues[ i ][ inds[i] ]->is_ambiguous() ) return false;
		}
		return true;
	}
};

InterpretationList Resolver::resolve( const Expr* expr, bool topLevel ) {
	InterpretationList results;
	
	if ( const FuncExpr* funcExpr = as_safe<FuncExpr>( expr ) ) {
		// find candidates with this function name, skipping if none
		auto withName = funcs.find( funcExpr->name() );
		if ( withName == funcs.end() ) {
			DBG( "withName: []" );
			return results;
		}
		DBG( "withName: " << print_all_deref( withName() ) );
		
		// get interpretations for subexpressions (if any)
		switch( funcExpr->args().size() ) {
			case 0: {
				results = matchFuncs( withName(), topLevel );
			} break;
			case 1: {
				results = matchFuncs( withName(), resolve( funcExpr->args().front() ), topLevel );
			} break;
			default: {
				std::vector<InterpretationList> sub_results;
				for ( auto& sub : funcExpr->args() ) {
					// break early if no subexpression results
					InterpretationList sresults = resolve( sub );
					if ( sresults.empty() ) return results;
					
					sub_results.push_back( move(sresults) );
				}
				
				interpretation_unambiguous valid;
				auto merged = unsorted_eager_merge<
					const Interpretation*, Cost, interpretation_cost>( sub_results, valid );
				
				results = matchFuncs( withName(), move(merged), topLevel );
			} break;
		}
	} else if ( const TypedExpr* typedExpr = as_derived_safe<TypedExpr>( expr ) ) {
		// do nothing for expressions which are already typed
		results.push_back( new Interpretation(typedExpr) );
	} else {
		assert(false && "Unsupported expression type");
	}
	
	// Expand results by applying user conversions (except at top level)
	if ( ! topLevel ) {
		expandConversions( results, conversions );
	}
	
	return results;
}

/// Ensures that resolved expressions have all are not ambiguous, 
class ResolvedExprInvalid : public TypedExprVisitor<bool> {
	AmbiguousEffect& on_ambiguous;
	UnboundEffect& on_unbound;

public:
	ResolvedExprInvalid( AmbiguousEffect& on_ambiguous, UnboundEffect& on_unbound )
		: on_ambiguous(on_ambiguous), on_unbound(on_unbound) {}

	bool visit( const CallExpr* e, bool& invalid ) final {
		if ( e->forall() && e->forall()->unbound() ) {
			on_unbound( e, *e->forall() );
			invalid = true;
			return false;
		}
		return true;
	}

	bool visit( const AmbiguousExpr* e, bool& invalid ) final {
		on_ambiguous( e->expr(), e->alts().begin(), e->alts().end() );
		invalid = true;
		return false;
	}
};

const Interpretation* Resolver::operator() ( const Expr* expr ) {
	InterpretationList results = resolve( expr, true );
	
	// return invalid interpretation on empty results
	if ( results.empty() ) {
		on_invalid( expr );
		return Interpretation::make_invalid();
	}

	if ( results.size() > 1 ) {
		// sort min-cost results to beginning
		InterpretationList::iterator min_pos = results.begin();
		for ( InterpretationList::iterator i = results.begin() + 1;
		      i != results.end(); ++i ) {
			if ( (*i)->cost == (*min_pos)->cost ) {
				// duplicate minimum cost; swap into next minimum position
				++min_pos;
				std::iter_swap( min_pos, i );
			} else if ( (*i)->cost < (*min_pos)->cost ) {
				// new minimum cost; swap into first position
				min_pos = results.begin();
				std::iter_swap( min_pos, i );
			}
		}

		// ambiguous if more than one min-cost interpretation
		if ( min_pos != results.begin() ) {
			List<TypedExpr> options;
			auto it = results.begin();
			while (true) {
				options.push_back( (*it)->expr );
				if ( it == min_pos ) break;
				++it;
			}
			on_ambiguous( expr, options.begin(), options.end() );
			return Interpretation::make_invalid();
		}
	}
	
	const Interpretation* candidate = results.front();

	// apply interpretations to environment to individual calls
	apply( candidate->env );
	
	// handle ambiguous candidate from conversion expansion
	if ( ResolvedExprInvalid{ on_ambiguous, on_unbound }( candidate->expr ) ) {
		return Interpretation::make_invalid();
	}
	
	return candidate;
}