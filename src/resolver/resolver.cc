#include <algorithm>
#include <cassert>
#include <vector>

#include "resolver.h"

#include "cost.h"
#include "env.h"
#include "env_substitutor.h"
#include "expand_conversions.h"
#include "func_table.h"
#include "interpretation.h"
#include "resolve_assertions.h"
#include "unify.h"

#include "ast/expr.h"
#include "ast/forall.h"
#include "ast/forall_substitutor.h"
#include "ast/mutator.h"
#include "ast/typed_expr_visitor.h"
#include "ast/typed_expr_mutator.h"
#include "data/cast.h"
#include "data/collections.h"
#include "data/debug.h"
#include "data/list.h"
#include "data/mem.h"
#include "data/option.h"
#include "merge/eager_merge.h"

/// Return an interpretation for all zero-arg functions in funcs
template<typename Funcs>
InterpretationList matchFuncs( Resolver& resolver, const Funcs& funcs, 
                               Resolver::Mode resolve_mode ) {
	InterpretationList results;
	
	// find functions with no parameters, skipping remaining checks if none
	auto withNParams = funcs.find( 0 );
	if ( withNParams == funcs.end() ) {
		DBG( "  with<0>params: []" );
		return results;
	}
	DBG( "  with<0>params: " << print_all_deref( withNParams() ) );
	
	// attempt to match functions to arguments
	for ( const FuncDecl* func : withNParams() ) {
		// skip functions returning no values, unless at top level
		if ( resolve_mode != Resolver::TOP_LEVEL && func->returns()->size() == 0 ) continue;

		Cost cost; // initialized to zero
		unique_ptr<Env> env; // initialized to nullptr
		
		CallExpr* call = new CallExpr{ func };

		// check type assertions if at top level
		if ( resolve_mode == Resolver::TOP_LEVEL ) {
			if ( ! resolveAssertions( resolver, call, cost, env ) ) continue;
		}
		
		results.push_back( new Interpretation{ call, move(cost), move(env) } );
	}
	
	return results;
}

/// Return an interpretation for all single-argument interpretations in funcs
template<typename Funcs>
InterpretationList matchFuncs( Resolver& resolver, 
                               const Funcs& funcs, InterpretationList&& args, 
                               Resolver::Mode resolve_mode ) {
	InterpretationList results;

	for ( const Interpretation* arg : args ) {
		DBG( "  arg: " << *arg );

		// find function with appropriate number of parameters, skipping arg if none
		auto n = arg->type()->size();
		auto withNParams = funcs.find( n );
		if ( withNParams == funcs.end() ) {
			DBG( "    with<" << n << ">params: []" );
			continue;
		}
		DBG( "    with<" << n << ">params: " << print_all_deref( withNParams() ) );

		// attempt to match functions to arguments
		for ( const FuncDecl* func : withNParams() ) {
			// skip functions returning no values, unless at top level
			if ( resolve_mode != Resolver::TOP_LEVEL && func->returns()->size() == 0 ) continue;

			// Environment for call bindings
			Cost cost = arg->cost;
			unique_ptr<Env> env = Env::from( arg->env );
			unique_ptr<Forall> forall = Forall::from( func->forall() );
			if ( forall ) {
				ForallSubstitutor replaceLocals{ func->forall(), forall.get() };
				func = replaceLocals( func );
			}
			
			// skip functions that don't match the parameter types
			if ( n == 1 ) { // concrete type arg
				if ( ! unify( func->params()[0], arg->type(), cost, env ) ) continue;
			} else {  // tuple type arg
				const List<Type>& argTypes = as<TupleType>( arg->type() )->types();
				for ( unsigned i = 0; i < n; ++i ) {
					if ( ! unify( func->params[i], argTypes[i], cost, env ) ) goto nextFunc;
				}
			}

			{ // extra scope so don't get goto errors for the labelled break simulation
				CallExpr* call = new CallExpr{ func, List<TypedExpr>{ arg->expr }, move(forall) };

				// check type assertions if at top level
				if ( resolve_mode == Resolver::TOP_LEVEL ) {
					if ( ! resolveAssertions( resolver, call, cost, env ) ) continue;
				}

				// create new interpretation for resolved call
				results.push_back( new Interpretation{ call, move(cost), move(env) } );
			}
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
                      Cost& cost, unique_ptr<Env>& env ) {
	unsigned i = 0;
	for ( unsigned j = 0; j < args.size(); ++j ) {
		// merge in argument environment
		if ( ! merge( env, args[j]->env ) ) return false;
		// test unification of parameters
		unsigned m = args[j]->type()->size();
		if ( m == 1 ) {
			if ( ! unify( params[i], args[j]->type(), cost, env ) ) return false;
			++i;
		} else {
			const List<Type>& argTypes = as<TupleType>( args[j]->type() )->types();
			for ( unsigned k = 0; k < m; ++k ) {
				if ( ! unify( params[i], argTypes[k], cost, env ) ) return false;
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
InterpretationList matchFuncs( Resolver& resolver, const Funcs& funcs, ComboList&& combos, 
                               Resolver::Mode resolve_mode ) {
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
		for ( const FuncDecl* func : withNParams() ) {
			// skip functions returning no values, unless at top level
			if ( resolve_mode != Resolver::TOP_LEVEL && func->returns()->size() == 0 ) continue;
			
			// Environment for call bindings
			Cost cost = combo.first;
			unique_ptr<Env> env{}; // initialized by argsMatchParams()
			unique_ptr<Forall> forall = Forall::from( func->forall() );
			if ( forall ) {
				ForallSubstitutor replaceLocals{ func->forall(), forall.get() };
				func = replaceLocals( func );
			}

			// skip functions that don't match the parameter types
			if ( ! argsMatchParams( combo.second, func->params(), cost, env ) ) continue;
			
			CallExpr* call = new CallExpr{ func, argsFrom( combo.second ), move(forall) };

			// check type assertions if at top level
			if ( resolve_mode == Resolver::TOP_LEVEL ) {
				if ( ! resolveAssertions( resolver, call, cost, env ) ) continue;
			}
			
			// create new interpretation for resolved call
			results.push_back( new Interpretation{ call, move(cost), move(env) } );
		}
	}
	
	return results;
}

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

InterpretationList Resolver::resolve( const Expr* expr, Resolver::Mode resolve_mode ) {
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
				results = matchFuncs( *this, withName(), resolve_mode );
			} break;
			case 1: {
				results = matchFuncs( *this, withName(), resolve( funcExpr->args().front() ), 
				                      resolve_mode );
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
				
				results = matchFuncs( *this, withName(), move(merged), resolve_mode );
			} break;
		}
	} else if ( const TypedExpr* typedExpr = as_derived_safe<TypedExpr>( expr ) ) {
		// do nothing for expressions which are already typed
		results.push_back( new Interpretation(typedExpr) );
	} else {
		assert(false && "Unsupported expression type");
	}
	
	if ( resolve_mode == ALL_NON_VOID ) {
		expandConversions( results, conversions );
	}
	
	return results;
}

InterpretationList Resolver::resolveWithType( const Expr* expr, const Type* targetType, 
	                                          const Env* env ) {
	return convertTo( targetType, resolve( expr, NO_CONVERSIONS ), conversions, env );
}

/// Ensures that resolved expressions have all their type variables bound and are not 
/// ambiguous.
class ResolvedExprInvalid : public TypedExprVisitor<ResolvedExprInvalid, bool> {
	AmbiguousEffect& on_ambiguous;

public:
	ResolvedExprInvalid( AmbiguousEffect& on_ambiguous ) : on_ambiguous(on_ambiguous) {}
	
	using TypedExprVisitor<ResolvedExprInvalid, bool>::visit;

	bool visit( const AmbiguousExpr* e, bool& invalid ) {
		on_ambiguous( e->expr(), e->alts().begin(), e->alts().end() );
		invalid = true;
		return false;
	}
};

const Interpretation* Resolver::operator() ( const Expr* expr ) {
	InterpretationList results = resolve( expr, Resolver::TOP_LEVEL );
	
	// return invalid interpretation on empty results
	if ( results.empty() ) {
		on_invalid( expr );
		return Interpretation::make_invalid();
	}

	if ( results.size() > 1 ) {
		// sort min-cost results to beginning
		InterpretationList::iterator min_pos = 
			sort_mins( results.begin(), results.end(), ByValueCompare<Interpretation>{} );
		
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
	
	// handle ambiguous candidate from conversion expansion TODO inline this again
	ResolvedExprInvalid is_invalid{ on_ambiguous };
	if ( is_invalid( candidate->expr ) ) {
		return Interpretation::make_invalid();
	}

	// check for unbound type variables in environment
	List<TypeClass> unbound = getUnbound( candidate->env.get() );
	if ( ! unbound.empty() ) {
		on_unbound( expr, unbound );
		return Interpretation::make_invalid();
	}
	
	return candidate;
}