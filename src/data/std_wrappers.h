// Copyright (c) 2015 University of Waterloo
//
// The contents of this file are covered under the licence agreement in 
// the file "LICENCE" distributed with this repository.

#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <vector>

/// Wrapper around std::map declaration with 2 template parameters
template<typename K, typename V>
using std_map = std::map<K, V>;

/// Wrapper around std::unordered_map declaration with 2 template parameters
template<typename K, typename V>
using std_unordered_map = std::unordered_map<K, V>;

/// Wrapper around std::set declaration with 1 template parameter
template<typename T>
using std_set = std::set<T>;

/// Wrapper around std::unordered_set with 1 template parameter
template<typename T>
using std_unordered_set = std::unordered_set<T>;

/// Wrapper around std::vector with 1 template parameter
template<typename T>
using std_vector = std::vector<T>;
