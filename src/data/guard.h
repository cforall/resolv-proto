#pragma once

// Copyright (c) 2015 University of Waterloo
//
// The contents of this file are covered under the licence agreement in 
// the file "LICENCE" distributed with this repository.

#include <utility>

/// Swaps a value out of a variable while in scope
template<typename T>
class SwapGuard {
	T& var;   ///< variable to reset
	T store;  ///< storage for old value

public:
	SwapGuard( T& var, T&& newVal ) : var(var), store(std::move(var)) {
		var = std::move(newVal);
	}

	~SwapGuard() { var = std::move(store); }
};

/// Swaps a value out of a variable, restoring it when returned guard leaves scope
template<typename T>
SwapGuard<T> swap_in_scope( T& var, T&& newVal ) { return { var, std::move(newVal) }; }

/// Defers an arbitrary zero-arg function
template<typename F>
class DeferGuard {
	F f;  ///< Function to defer

	DeferGuard( F&& f ) : f(std::move(f)) {}

	~DeferGuard() { f(); }
};

/// Defers an arbitrary zero-arg function until the returned guard leaves scope
template<typename F>
DeferGuard<F> defer( F&& f ) { return { std::move(f) }; }
