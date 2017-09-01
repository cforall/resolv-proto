#include "gc.h"

#include <algorithm>

GC& GC::get() {
	static GC gc;
	return gc;
}

GC::GC() : mark(false), old(), young(), using_young(false) {
	old.reserve(70000);
}

const GC& GC::operator<< (const GC_Traceable* obj) const {
	if( obj )
	{
		bool isMarked = obj->mark == this->mark;
		if( !isMarked ) {
			obj->mark = this->mark;
			obj->trace( *this );
		}
	}
	return *this;
}

void GC::register_object(GC_Object* obj) {
	(using_young ? young : old).push_back(obj);
	obj->mark = this->mark;
}

void GC::new_generation() {
	using_young = true;
}

void GC::collect_young() {
	// check young generation, just reset mark if not using
	if ( ! using_young ) {
		mark = !mark;
		return;
	}

	// collect young gen
	for ( GC_Object*& obj : young ) {
		if ( obj->mark != mark ) {
			delete obj;
			obj = nullptr;
		}
	}
	
	// move uncollected elements into old gen
	auto end_live = std::remove( young.begin(), young.end(), nullptr );
	old.insert( old.end(), young.begin(), end_live );
	
	// clear young gen
	using_young = false;
	young.clear();

	// reset mark for next collection
	mark = !mark;
}

void GC::collect() {
	// collect old gen
	for ( GC_Object*& obj : old ) {
		if ( obj->mark != mark ) {
			delete obj;
			obj = nullptr;
		}
	}

	// clear collected elements
	old.erase( std::remove( old.begin(), old.end(), nullptr ), old.end() );

	// collect young gen (also resets mark)
	collect_young();
}

GC_Object::GC_Object() {
	GC::get().register_object( this );
}
