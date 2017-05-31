#include "forall_substitutor.h"

#include "ast/decl.h"
#include "data/cast.h"
#include "data/mem.h"

FuncDecl* ForallSubstitutor::operator() ( const FuncDecl* d ) {
	// clone parameters
	List<Type> params;
	params.reserve( d->params().size() );
	for ( const Type* param : d->params() ) {
		params.push_back( (*this)(param) );
	}
	// clone returns
	const Type* returns = (*this)(d->returns());
	// clone assertion foralls, recursively
	unique_ptr<Forall> forall{};
	if ( d->forall() ) {
		forall.reset( new Forall{} );
		foralls.emplace( d->forall(), forall.get() );
		
		for ( const PolyType* v : d->forall()->variables() ) {
			forall->vars.push_back( as<PolyType>((*this)(v)) );
		}
		for ( const FuncDecl* a : d->forall()->assertions() ) {
			forall->assns.push_back( (*this)(a) );
		}
	}
	// build new function declaration
	return new FuncDecl{ d->name(), d->tag(), move(params), returns, move(forall) };
}
