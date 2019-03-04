#pragma once

// Copyright (c) 2015 University of Waterloo
//
// The contents of this file are covered under the licence agreement in 
// the file "LICENCE" distributed with this repository.

// Standard includes for dealing with memory

#include <memory>
#include <utility>

/// use unique_ptr
using std::unique_ptr;
using std::make_unique;

/// Take ownership of a value
using std::move;

/// Shallow copy of an object
template<typename T>
inline T copy(const T& x) { return x; }

/// Copy an element through a unique pointer
template<typename T>
inline unique_ptr<T> copy(const unique_ptr<T>& p) {
    return unique_ptr<T>{ p ? new T{*p} : nullptr };
}

/// forward arguments
using std::forward;
