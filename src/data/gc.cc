#include "gc.h"

#include <algorithm>
#include <cassert>

GC& GC::get() {
	static GC gc;
	return gc;
}

GC::GC() : gens(1), static_roots(), mark(false), g(0) {
	gens[0].reserve(70000);
}

GC::~GC() {
	for ( unsigned i = 0; i <= g; ++i ) {
		for ( GC_Object* o : gens[i] ) {
			delete o;
		}
	}
}

const GC& GC::operator<< (const GC_Traceable* obj) const {
	if( obj )
	{
		if( obj->mark != this->mark ) {
			obj->mark = this->mark;
			obj->trace( *this );
		}
	}
	return *this;
}

// build with flag GC_TRAP to compile in a breakpoint on register and sweep of a dynamically 
// chosen object. Process to use:
//     break GC::register_object
//     run
//     set variable GC_TRAP_OBJ = <target>
//     disable <first breakpoint>
//     continue
#ifdef GC_TRAP
#include <csignal>

/// Set to object to check in debugger
void* GC_TRAP_OBJ = nullptr;
#endif

void GC::register_object(GC_Object* obj) {
	#ifdef GC_TRAP
		if ( obj == GC_TRAP_OBJ ) std::raise(SIGTRAP);
	#endif
	gens[g].push_back(obj);
	obj->mark = !this->mark;  // initialize as un-marked
}

void GC::register_static_root(GC_Traceable* root) {
	static_roots.push_back(root);
}

GC_Guard GC::new_generation() {
	if ( ++g == gens.size() ) { gens.emplace_back(); }  // ensure new generation available
	mark = !mark;  // switch mark so aged young objects will still be unmarked in old
	return { *this, g };
}

void GC::trace_static_roots() {
	for ( GC_Traceable* root : static_roots ) {
		root->trace( *this );
	}
}

void GC::collect_young() {
	// get generations and decrement generation
	assert(g > 0 && "Cannot collect_young without young generation");
	Generation& young = gens[g];
	Generation& old = gens[--g];

	// ensure static roots traced
	trace_static_roots();

	// collect young gen
	for ( GC_Object*& obj : young ) {
		if ( obj->mark != mark ) {
			#ifdef GC_TRAP
				if ( obj == GC_TRAP_OBJ ) std::raise(SIGTRAP);
			#endif
			delete obj;
			obj = nullptr;
		}
	}
	
	// move uncollected elements into old gen
	auto end_live = std::remove( young.begin(), young.end(), nullptr );
	old.insert( old.end(), young.begin(), end_live );
	
	// clear young gen and reset mark to return to old generation mark
	young.clear();
	mark = !mark;
}

void GC::collect() {
	// ensure not called when young gen is active
	assert(g == 0 && "Cannot do old collection when young generation is active");
	Generation& old = gens[0];

	// ensure static roots traced
	trace_static_roots();

	// collect old gen
	for ( GC_Object*& obj : old ) {
		if ( obj->mark != mark ) {
			#ifdef GC_TRAP
				if ( obj == GC_TRAP_OBJ ) std::raise(SIGTRAP);
			#endif
			delete obj;
			obj = nullptr;
		}
	}

	// clear collected elements
	old.erase( std::remove( old.begin(), old.end(), nullptr ), old.end() );

	// reset mark
	mark = !mark;
}

GC_Object::GC_Object() {
	GC::get().register_object( this );
}

GC_Object::GC_Object( const GC_Object& ) {
	GC::get().register_object( this );
}

GC_Object::GC_Object( GC_Object&& ) {
	GC::get().register_object( this );
}
