#include "forall_substitutor.h"

#include "decl.h"

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
		forall.reset( new Forall{ d->name() } );
		foralls.emplace( d->forall(), forall.get() );
		
		for ( const PolyType* v : d->forall()->variables() ) {
			(*this)(v); // substitutor adds vars when seen
		}
		for ( const FuncDecl* a : d->forall()->assertions() ) {
			forall->assns.push_back( (*this)(a) );
		}
	}
	// build new function declaration
	return new FuncDecl{ d->name(), d->tag(), move(params), returns, move(forall) };
}
