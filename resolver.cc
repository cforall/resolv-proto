#include <algorithm>
#include <cassert>
#include <vector>

#include "resolver.h"

#include "binding.h"
#include "cost.h"
#include "cow.h"
#include "data.h"
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

/// Checks the type assertions of a function call.
bool assertionsUnresolvable( Resolver& resolver, const TypeBinding* bindings, 
                             Cost& cost, cow_ptr<Environment>& env ) {
	if ( ! bindings || bindings->assertions().empty() ) return false;

	for ( auto it = bindings->assertions().begin(); it != bindings->assertions().end(); ++it ) {
		// Generate FuncExpr for assertion
		const FuncDecl* asnDecl = it->first;
		List<Expr> asnArgs;
		asnArgs.reserve( asnDecl->params().size() );
		for ( const Type* pType : asnDecl->params() ) {
			asnArgs.push_back( new VarExpr( pType ) );
		}
		const FuncExpr* asnFunc = new FuncExpr{ asnDecl->name(), move(asnArgs) };

		// check if it can be resolved
		const Interpretation* satisfying = 
			resolver.resolveWithType( asnFunc, asnDecl->returns(), env );
		if ( ! satisfying ) return true;

		// record resolution
		bind_assertion( it, satisfying );
		cost += satisfying->cost;
	}
	return false;
}

/// Checks the validity of all the type assertions in a resolved expression
class ExprAssertionsUnresolvable : public TypedExprVisitor<bool> {
	Resolver& resolver;
	Cost& cost;
	cow_ptr<Environment>& env;

public:
	ExprAssertionsUnresolvable( Resolver& resolver, Cost& cost, cow_ptr<Environment>& env ) 
		: resolver(resolver), cost(cost), env(env) {}

	bool visit( const CallExpr* e, bool& invalid ) final {
		invalid = assertionsUnresolvable( resolver, e->forall(), cost, env );
		return invalid;
	}
}

/// Return an interpretation for all zero-arg functions in funcs
template<typename Funcs>
InterpretationList matchFuncs( Resolver& resolver, 
                               const Funcs& funcs, Resolver::Mode resolve_mode ) {
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
		if ( resolve_mode != TOP_LEVEL && func->returns()->size() == 0 ) continue;

		Cost cost; // initialized to zero
		unique_ptr<TypeBinding> localEnv = copy(func->tyVars());
		cow_ptr<Environment> env; // initialized to nullptr
		
		CallExpr call = new CallExpr{ func, List<TypedExpr>{}, move(localEnv) };

		// check type assertions if at top level
		if ( resolve_mode == TOP_LEVEL ) {
			if ( assertionsUnresolvable( resolver, call->forall(), cost, env ) ) continue;
		}
		
		// create new zero-cost interpretation for resolved call
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
		for ( const auto& func : withNParams() ) {
			// skip functions returning no values, unless at top level
			if ( resolve_mode != TOP_LEVEL && func->returns()->size() == 0 ) continue;

			// Environment for call bindings
			Cost cost = arg->cost;
			unique_ptr<TypeBinding> localEnv = copy(func->tyVars());
			cow_ptr<Environment> env = arg->env;

			// skip functions that don't match the parameter types
			if ( n == 1 ) { // concrete type arg
				if ( ! unify( func->params()[0], arg->type(), cost, *localEnv, env ) ) continue;
			} else {  // tuple type arg
				const List<Type>& argTypes = as<TupleType>( arg->type() )->types();
				for ( unsigned i = 0; i < n; ++i ) {
					if ( ! unify( func->params()[i], argTypes[i], cost, *localEnv, env ) ) 
						goto nextFunc;
				}
			}

			CallExpr* call = new CallExpr{ func, List<TypedExpr>{ arg->expr }, move(localEnv) };

			// check type assertions if at top level
			if ( resolve_mode == TOP_LEVEL ) {
				if ( ExprAssertionsUnresolvable{ resolver, cost, env }( call ) ) continue;
			}

			// create new interpretation for resolved call
			results.push_back( new Interpretation{ call, move(cost), move(env) } );
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
InterpretationList matchFuncs( Resolver& resolver, 
                               const Funcs& funcs, ComboList&& combos, 
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
		for ( auto&& func : withNParams() ) {
			// skip functions returning no values, unless at top level
			if ( resolve_mode != TOP_LEVEL && func->returns()->size() == 0 ) continue;
			
			// Environment for call bindings
			Cost cost = combo.first;
			unique_ptr<TypeBinding> localEnv = copy(func->tyVars());
			cow_ptr<Environment> env; // initialized by argsMatchParams()

			// skip functions that don't match the parameter types
			if ( ! argsMatchParams( combo.second, func->params(), cost, *localEnv, env ) ) 
				continue;
			
			CallExpr* call = new CallExpr{ func, argsFrom( combo.second ), move(localEnv) };

			// check type assertions if at top level
			if ( resolve_mode == TOP_LEVEL ) {
				if ( ExprAssertionsUnresolvable{ resolver, cost, env }( call ) ) continue;
			}
			
			// create new interpretation for resolved call
			results.push_back( new Interpretation{ call, move(cost), move(env) } );
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
				results = matchFuncs( *this, withName(), resolve( funcExpr->args().front() ), resolve_mode );
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

const Interpretation* Resolver::resolveWithType( const Expr* expr, const Type* targetType, 
	                                             Cost& cost, cow_ptr<Environment>& env ) {
	return convertTo( targetType, resolve( expr, NO_CONVERSIONS ), conversions, env );
}

/// Ensures that resolved expressions have all their type variables bound and are not 
/// ambiguous.
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
	InterpretationList results = resolve( expr, Resolver::TOP_LEVEL );
	
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

	// apply interpretations in environment to individual calls
	apply( candidate->env );
	
	// handle ambiguous candidate from conversion expansion
	if ( ResolvedExprInvalid{ on_ambiguous, on_unbound }( candidate->expr ) ) {
		return Interpretation::make_invalid();
	}
	
	return candidate;
}