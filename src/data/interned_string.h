#pragma once

// Copyright (c) 2015 University of Waterloo
//
// The contents of this file are covered under the licence agreement in 
// the file "LICENCE" distributed with this repository.

#include <functional>
#include <string>
#include <unordered_set>
#include <utility>

/// Keeps canonical copies of a std::string for quicker comparisons
class interned_string {
	/// Shared map of canonical string representations
	static std::unordered_set< std::string > canonical;

	/// Canonical representation of empty string
	static const std::string* empty_string() {
		static const std::string* mt = [](){
			return &*canonical.emplace( "" ).first;
		}();
		return mt;
	}

	/// Canonicalize string
	template<typename S>
	static const std::string* intern( S&& s ) {
		return &*canonical.emplace( std::forward<S>(s) ).first;
	}

	/// Pointer to stored string
	const std::string* s;
	
public:
	interned_string() : s{empty_string()} {}
	interned_string(const char* cs) : s{intern(cs)} {}
	interned_string(const std::string& ss) : s{intern(ss)} {}

	operator const std::string& () const { return *s; }

	bool operator== (const interned_string& o) const { return s == o.s; }
	bool operator!= (const interned_string& o) const { return s != o.s; }
};

inline std::ostream& operator<< (std::ostream& out, const interned_string& s) {
	return out << (const std::string&)s;
}

namespace std {
	template<> struct hash<interned_string> {
		std::size_t operator() (const interned_string& s) const {
			return std::hash<const std::string*>{}( &(const std::string&)s );
		}
	};
}
