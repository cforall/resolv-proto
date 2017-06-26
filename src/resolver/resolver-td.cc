#include <cassert>

#include "resolver.h"

#include "env.h"
#include "interpretation.h"

#include "ast/expr.h"
#include "ast/type.h"

InterpretationList Resolver::resolve( const Expr* expr, const Env* env, 
                                      Resolver::Mode resolve_mode ) {
	assert(!"unimplemented");
	return {};
}

InterpretationList Resolver::resolveWithType( const Expr* expr, const Type* targetType,                                                 const Env* env ) {
	assert(!"unimplemented");
	return {};
}