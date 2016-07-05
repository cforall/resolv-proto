#include <cassert>

#include "resolver.h"

#include "expr.h"
#include "interpretation.h"
#include "utility.h"

List<Interpretation, ByShared> resolve( Ref<Expr> expr, bool expandConversions ) {
	List<Interpretation, ByShared> results;
	
	if ( Ref<VarExpr> varExpr = ref_as<VarExpr>( expr ) ) {
		// create new dummy variable
		vars.emplace_back( varExpr.ty() );
		// build interpretation for variable
	} else if ( Ref<FuncExpr> funcExpr = ref_as<FuncExpr>( expr ) ) {
		// TODO
	} else {
		assert(false && "Unsupported expression type");
	}
	
	if ( expandConversions ) {
		// TODO
	}
	
	return results;
}

Interpretation Resolver::operator() ( Ref<Expr> expr ) {
	List<Interpretation, ByShared> results = resolve( expr, false );
	
	if ( results.empty() ) return Interpretation::make_invalid();
	
	// TODO handle ambiguity, get interpretation out of sharedptr
}