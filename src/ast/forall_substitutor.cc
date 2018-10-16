#include "forall_substitutor.h"

#include "decl.h"

#include "data/cast.h"
#include "data/debug.h"
#include "data/mem.h"

Decl* ForallSubstitutor::operator() ( const Decl* d, unsigned& src ) {
	auto did = typeof(d);
	if ( did == typeof<FuncDecl>() ) {
		const FuncDecl* fd = as<FuncDecl>(d);

		// clone assertion foralls, recursively
		unique_ptr<Forall> forall{};
		if ( fd->forall() ) {
			// clone forall
			forall.reset( new Forall{} );
			forall->vars.reserve( fd->forall()->variables().size() );
			for ( const PolyType* v : fd->forall()->variables() ) {
				forall->vars.push_back( new PolyType{ v->name(), ++src } );
			}
			ctx.push_back( forall.get() );
			
			// clone assertions
			forall->assns.reserve( fd->forall()->assertions().size() );
			for ( const Decl* a : fd->forall()->assertions() ) {
				forall->assns.push_back( (*this)(a, src) );
			}
		}
		// clone parameters and returns
		List<Type> params = (*this)(fd->params());
		const Type* returns = (*this)(fd->returns());
		// remove forall from context
		if ( forall ) { ctx.pop_back(); }
		// build new function declaration
		return new FuncDecl{ fd->name(), fd->tag(), move(params), returns, move(forall) };
	} else if ( did == typeof<VarDecl>() ) {
		const VarDecl* vd = as<VarDecl>(d);
		// clone type
		const Type* vty = (*this)(vd->type());
		// build new declaration
		return new VarDecl{ vd->name(), vd->tag(), vty };
	}
	
	unreachable(!"invalid declaration type");
	return nullptr;
}
