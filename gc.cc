#include "gc.h"

#include <cmath>
#include <assert.h>
#include <iostream>

GC& GC::get() {
	static GC gc;
	return gc;
}

GC::GC() : mark(false), isCollecting(false), collectingObject(nullptr), objects() {
	objects.reserve(70000);
}

GC::~GC() {
//	std::cout << max_count << std::endl;
}

const GC& GC::operator<< (const GC_Object* obj) const {
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
	// assertf( std::find(objects.begin(), objects.end(), obj) == objects.end(), "GC ERROR: object %lX was registered more than once.\n", obj );
	objects.push_back(obj);
	obj->mark = this->mark;
//	max_count = std::max(max_count, objects.size());
}

void GC::unregister_object(GC_Object* obj) {
	assert( !isCollecting || obj == collectingObject );
	// auto removed_objs = std::remove( objects.begin(), objects.end(), obj );
	// objects.erase( removed_objs, objects.end() );
}

void GC::collect() {
	isCollecting = true;
	for(GC_Object*& obj : objects) {
		collectingObject = obj;
		bool isMarked = obj->mark == this->mark;
		if( !isMarked ) {
			delete obj;
			obj = nullptr;
		}
	}

	objects.erase( std::remove( objects.begin(), objects.end(), nullptr ), objects.end() );
	isCollecting = false;
	collectingObject = nullptr;

	mark = !mark;
}

GC_Object::GC_Object() {
	GC::get().register_object( this );
}

GC_Object::~GC_Object() {
	GC::get().unregister_object( this );
}
