#pragma once

#include <vector>

#include "gc.h"

/// List of const T*
template<typename T>
using List = std::vector<const T*>;

template<typename T>
inline const GC& operator<< (const GC& gc, const List<T>& container) {
	for(auto&& obj : container) {
		gc << obj;
	}
	return gc;
}
