#include <cassert>
#include <vector>

#include "resolver.h"

#include "cost.h"
#include "eager_merge.h"
#include "expr.h"
#include "func_table.h"
#include "interpretation.h"
#include "nway_merge.h"
#include "utility.h"

/// Return an interpretation for all zero-arg functions in funcs
template<typename Funcs>
InterpretationList matchFuncs( const Funcs& funcs, bool topLevel = false ) {
	InterpretationList results;
	
	// find functions with no parameters, skipping remaining checks if none
	auto withNParams = funcs.find( 0 );
	if ( withNParams == funcs.end() ) return results;
	
	// attempt to match functions to arguments
	for ( auto& func : withNParams() ) {
		// skip functions returning no values, unless at top level
		if ( ! topLevel && func.returns()->size() == 0 ) continue;
			
		// create new zero-cost interpretation for resolved call
		results.emplace_back( 
			make_shared<CallExpr>( brw(func) ),
			Cost{}
		);
	}
	
	return results;
}

/// Gets the number of elemental types represented by an interpretation
unsigned argNParams( const Interpretation& arg ) { return arg.type()->size(); }

unsigned argNParams( const InterpretationList& args ) {
	unsigned n_params = 0;
	for ( const Interpretation& i : args ) {
		n_params += i.type()->size();
	}
	return n_params;
}

/// true iff type of arg matches params[0]; params should have a single element
bool argsMatchParams( const Interpretation& arg, const List<Type, ByBrw>& params ) {
	return *arg.type() == *params[0];
} 

/// true iff types of args match parameter types; 
/// args and params should have the same length 
bool argsMatchParams( const InterpretationList& args, 
                      const List<Type, ByBrw>& params ) {
	for (unsigned i = 0; i < args.size(); ++i) {
		if ( *args[i].type() != *params[i] ) return false;
	}
	return true;
}

/// Create a list of argument expressions from a single interpretation
List<Expr, ByShared> argsFrom( const Interpretation& i ) {
	return List<Expr, ByShared>{ 1, i.expr };
}

/// Create a list of argument expressions from a list of interpretations
List<Expr, ByShared> argsFrom( const InterpretationList& is ) {
	List<Expr, ByShared> args;
	for ( auto& i : is ) {
		args.push_back( i.expr );
	}
	return args;
}

/// Return an interpretation for all matches between functions in funcs and 
/// argument packs in combos; args should return a reference to an Interpretation or 
/// InterpretationList representing the argument pack, cost should return a Cost by 
/// value.
template<typename Funcs, typename Combos, typename GetArgs, typename GetCost>
InterpretationList matchFuncs( const Funcs& funcs, Combos&& combos, 
                               GetArgs&& args, GetCost&& cost, 
                               bool topLevel = false ) {
	InterpretationList results;
	
	for ( auto& combo : combos ) {
		// find function with appropriate number of parameters, 
		// skipping combo if none
		auto withNParams = funcs.find( argNParams( args( combo ) ) );
		if ( withNParams == funcs.end() ) continue;
		
		// attempt to match functions to arguments
		for ( auto& func : withNParams() ) {
			// skip functions returning no values, unless at top level
			if ( ! topLevel && func.returns()->size() == 0 ) continue;
			
			// skip functions that don't match the type
			if ( ! argsMatchParams( args( combo ), func.params() ) )
				continue;
			
			// create new interpretation for resolved call
			results.emplace_back( 
				make_shared<CallExpr>( brw(func), 
				                       argsFrom( args( combo ) ) ),
				cost( combo )
			);
		}
	}
	
	return results;
}

/// Extracts the cost from an interpretation
struct interpretation_cost {
	const Cost& operator() ( const Interpretation& i ) { return i.cost; }	
};

/// Combination validator that fails on ambiguous interpretations in combinations
struct interpretation_unambiguous {
	bool operator() ( const std::vector< InterpretationList >& queues,
	                  const std::vector< unsigned >& inds ) {
		for( unsigned i = 0; i < queues.size(); ++i ) {
			if ( queues[ i ][ inds[i] ].is_ambiguous() ) return false;
		}
		return true;
	}
};

InterpretationList Resolver::resolve( const Shared<Expr>& expr, bool topLevel ) {
	InterpretationList results;
	
	if ( Shared<TypedExpr> typedExpr = shared_as<TypedExpr>( expr ) ) {
		// do nothing for expressions which are already typed
		results.push_back( move( typedExpr ) );
	} else if ( Brw<FuncExpr> funcExpr = brw_as<FuncExpr>( expr ) ) {
		// find candidates with this function name, skipping if none
		auto withName = funcs.find( funcExpr->name() );
		if ( withName == funcs.end() ) return results;
		
		// get interpretations for subexpressions (if any)
		switch( funcExpr->args().size() ) {
			case 0: {
				results = matchFuncs( withName(), topLevel );
			} break;
			case 1: {
				results = 
					matchFuncs( withName(),
				                resolve( funcExpr->args().front() ),
								[](const Interpretation& i) -> const Interpretation& {
									return i;
								},
								[](const Interpretation& i) -> Cost {
									return i.cost;
								}, 
								topLevel );
			} break;
			default: {
				std::vector<InterpretationList> sub_results;
				for ( auto& sub : funcExpr->args() ) {
					// break early if no subexpression results
					InterpretationList sresults = resolve( sub );
					if ( sresults.empty() ) return results;
					
					sub_results.push_back( move(sresults) );
				}
				
				auto merged = unsorted_eager_merge<
					Interpretation,
					Cost,
					interpretation_cost,
					InterpretationList,
					interpretation_unambiguous>( move(sub_results) );
				
				typedef std::pair<Cost, InterpretationList> merge_el; 
				results = 
					matchFuncs( withName(), 
					            move(merged),
								[](const merge_el& e) -> const InterpretationList& {
									return e.second;
								},
								[](const merge_el& e) -> Cost {
									return e.first;
								},
								topLevel );
			} break;
		}
	} else {
		assert(false && "Unsupported expression type");
	}
	
	if ( ! topLevel ) {
		// TODO expand conversions
	}
	
	return results;
}

Interpretation Resolver::operator() ( const Shared<Expr>& expr ) {
	InterpretationList results = resolve( expr, true );
	
	// return invalid interpretation on empty results
	if ( results.empty() ) {
		on_invalid( brw(expr) );
		return Interpretation::make_invalid();
	}
	
	// sort results by cost
	if ( results.size() > 1 ) {
		std::sort( results.begin(), results.end(), 
			[](const Interpretation& a, const Interpretation& b) {
				return a.cost < b.cost;
			});
	}
	
	// TODO handle ambiguous candidates from conversion expansion
	Interpretation& candidate = results.front();
	if ( results.size() == 1 ) return move(candidate);
	
	// check for multiple min-cost interpretations
	unsigned next_cost;
	for ( next_cost = 1; next_cost < results.size(); ++next_cost ) {
		if ( results[next_cost].cost > candidate.cost ) break;
	}
	if ( next_cost > 1 ) {
		on_ambiguous( brw(expr), results.begin(), results.begin() + next_cost );
		return Interpretation::make_invalid();
	}
	
	return move(candidate);
}