#include "forall_substitutor.h"

#include "decl.h"

#include "data/cast.h"
#include "data/mem.h"

FuncDecl* ForallSubstitutor::operator() ( const FuncDecl* d, unsigned& src ) {
	// clone assertion foralls, recursively
	unique_ptr<Forall> forall{};
	if ( d->forall() ) {
		// clone forall
		forall.reset( new Forall{} );
		forall->vars.reserve( d->forall()->variables().size() );
		for ( const PolyType* v : d->forall()->variables() ) {
			forall->vars.push_back( new PolyType{ v->name(), ++src } );
		}
		ctx.push_back( forall.get() );
		
		// clone assertions
		forall->assns.reserve( d->forall()->assertions().size() );
		for ( const FuncDecl* a : d->forall()->assertions() ) {
			forall->assns.push_back( (*this)(a, src) );
		}
	}
	// clone parameters and returns
	List<Type> params = (*this)(d->params());
	const Type* returns = (*this)(d->returns());
	// remove forall from context
	if ( forall ) { ctx.pop_back(); }
	// build new function declaration
	return new FuncDecl{ d->name(), d->tag(), move(params), returns, move(forall) };
}
