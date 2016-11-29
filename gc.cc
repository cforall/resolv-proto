#include "gc.h"

#include <cmath>
#include <assert.h>
#include <iostream>

GC& GC::get() {
	static GC gc;
	return gc;
}

GC::GC() : mark(false), objects() {
	objects.reserve(70000);
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
	objects.push_back(obj);
	obj->mark = this->mark;
}

void GC::collect() {
	for(GC_Object*& obj : objects) {
		bool isMarked = obj->mark == this->mark;
		if( !isMarked ) {
			delete obj;
			obj = nullptr;
		}
	}

	objects.erase( std::remove( objects.begin(), objects.end(), nullptr ), objects.end() );
	
	mark = !mark;
}

GC_Object::GC_Object() {
	GC::get().register_object( this );
}
