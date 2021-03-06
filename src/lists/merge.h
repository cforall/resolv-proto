#pragma once

// Copyright (c) 2015 University of Waterloo
//
// The contents of this file are covered under the licence agreement in 
// the file "LICENCE" distributed with this repository.

/// Common code for merge functions.

/// Type of index vector for combinations
typedef std::vector<unsigned> Indices;

/// Identity functor for T
template<typename T>
struct identity {
	const T& operator() ( const T& x ) { return x; }
};

/// Default validator for n-way merge.
///
/// Validators are functors which, given a reference to the queues and a set of 
/// indices for those queues (guaranteed to be addressable), return whether or not 
/// the combination is valid.
///
/// The default validator always returns valid.
/// 
/// @param Q  The underlying queue type for for the n-way merge
template<typename Q>
struct always_valid {
	/// Returns true.
	bool operator() ( const std::vector<Q>& queues, 
	                  const std::vector<unsigned>& inds ) {
		return true;
	}
};
