#include <cassert>
#include <vector>

#include "resolver.h"

#include "cost.h"
#include "expr.h"
#include "func_table.h"
#include "interpretation.h"
#include "nway_merge.h"
#include "utility.h"

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

/// true iff types of args match parameter types; 
/// args and params should have the same length 
bool argsMatchParams( const InterpretationList& args, 
                      const List<Type, ByRef>& params ) {
	for (unsigned i = 0; i < args.size(); ++i) {
		if ( *args[i].type() != *params[i] ) return false;
	}
	return true;
}

/// Create a list of argument expressions from a list of interpretations
List<Expr, ByShared> args_from( const InterpretationList& is ) {
	List<Expr, ByShared> args;
	for ( auto& i : is ) {
		args.push_back( i.expr );
	}
	return args;
}

/// Recursively resolve interpretations, expanding conversions if not at the top 
/// level.
/// May return ambiguous interpretations, but otherwise will not return invalid 
/// interpretations.
InterpretationList resolve( Shared<Expr>& expr, bool topLevel = false ) {
	typedef NWaySumMerge< Interpretation, 
	                      Cost, 
						  interpretation_cost, 
						  InterpretationList,
						  interpretation_unambiguous > Merge;
	
	InterpretationList results;
	
	if ( Shared<TypedExpr> typedExpr = shared_as<TypedExpr>( expr ) ) {
		// do nothing for expressions which are already typed
		results.push_back( move( typedExpr ) );
	} else if ( Ref<FuncExpr> funcExpr = ref_as<FuncExpr>( expr ) ) {
		// find candidates with this function name, skipping if none
		auto withName = funcs.find( funcExpr->name() );
		if ( withName == funcs.end() ) return results;
		
		// get interpretations for subexpressions
		std::vector<InterpretationList> sub_results;
		for ( auto& sub : expr.args() ) {
			// break early if no subexpression results
			InterpretationList sresults = resolve( sub );
			if ( sresults.empty() ) return results;
			
			sub_results.push_back( move(sresults) );
		}
		
		// combine interpretations
		for ( Merge combos{ move(sub_results) }; ! combos.empty(); combos.pop() ) {
			auto cost_combo = combos.top();
			
			// count parameters from subexpressions
			unsigned n_params = 0;
			for ( const Interpretation& i : cost_combo.second ) {
				n_params += i.type()->size();
			}
			
			// find function with appropriate number of parameters, 
			// skipping combo if none
			auto withNParams = withName().find( n_params );
			if ( withNParams == withName().end() ) continue;
			
			// attempt to match functions to arguments
			for ( auto& func : withNParams() ) {
				// skip functions returning no values, unless at top level
				if ( ! topLevel && func.returns().size() == 0 ) continue;
				
				// skip functions that don't match the type
				if ( ! argsMatchParams( cost_combo.second, func.params() ) ) 
					continue;
				
				// create new interpretation for resolved call
				results.emplace_back( 
					make_shared<CallExpr>( ref(func), 
					                       args_from( cost_combo.second ) ),
					copy(cost_combo.first)
				);
			}
		}
	} else {
		assert(false && "Unsupported expression type");
	}
	
	if ( ! topLevel ) {
		// TODO expand conversions
	}
	
	return results;
}

Interpretation Resolver::operator() ( Shared<Expr>& expr ) {
	InterpretationList results = resolve( expr, true );
	
	// return invalid interpretation on empty results
	if ( results.empty() ) {
		on_invalid();
		return Interpretation::make_invalid();
	}
	
	// TODO handle ambiguous candidates from conversion expansion
	Interpretation& candidate = results.front();
	if ( results.size() == 1 ) return move(candidate);
	
	// check for multiple min-cost interpretations
	unsigned next_cost;
	for ( next_cost = 1; i < results.size(); ++next_cost ) {
		if ( results[next_cost].cost > candidate.cost ) break;
	}
	if ( next_cost > 1 ) {
		on_ambiguous( results.begin(), results.begin() + next_cost );
		return Interpretation::make_invalid();
	}
	
	return move(candidate);
}