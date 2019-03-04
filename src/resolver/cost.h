#pragma once

// Copyright (c) 2015 University of Waterloo
//
// The contents of this file are covered under the licence agreement in 
// the file "LICENCE" distributed with this repository.

#include <algorithm>
#include <limits>
#include <ostream>

#include "data/compare.h"

/// Cost of an expression interpretation
struct Cost {
	typedef unsigned Element;

	Element unsafe;  ///< Unsafe conversion cost
	Element poly;    ///< Count of parameters and return values bound to some polymorphic type.
	Element vars;    ///< Count of polymorphic type variables
	Element spec;    ///< Count of polymorphic type specializations (type assertions), lowers cost
	Element safe;    ///< Safe conversion cost
	
	constexpr Cost() : unsafe(0), poly(0), vars(0), spec(0), safe(0) {}
	constexpr Cost(Element unsafe, Element poly, Element vars, Element spec, Element safe)
		: unsafe(unsafe), poly(poly), vars(vars), spec(spec), safe(safe) {}

	void swap(Cost& a, Cost& b) {
		char* ap = (char*)&a;
		std::swap_ranges(ap, ap + sizeof(Cost), (char*)&b);
	}

	/// Minimum cost value
	static constexpr Cost min() {
		return Cost{ 0,
			         0,
			         0,
			         std::numeric_limits<Element>::max(),
			         0 };
	}

	/// Zero cost value
	static constexpr Cost zero() { return Cost{}; }

	/// Maximum cost value
	static constexpr Cost max() {
		return Cost{ std::numeric_limits<Element>::max(),
		             std::numeric_limits<Element>::max(),
		             std::numeric_limits<Element>::max(),
		             0,
					 std::numeric_limits<Element>::max() };
	}
	
	/// Generates a new cost from an integer difference; the cost will have 
	/// the same magnitude as diff, unsafe if diff is negative, safe if diff 
	/// is positive
	static constexpr Cost from_diff(int diff) {
		return diff < 0 ? Cost{ (Element)-diff, 0, 0, 0, 0 } : Cost{ 0, 0, 0, 0, (Element)diff };
	}

	/// Cost with only unsafe component
	static constexpr Cost from_unsafe(Element unsafe) { return { unsafe, 0, 0, 0, 0 }; }

	/// Cost with only polymorphic component
	static constexpr Cost from_poly(Element poly) { return { 0, poly, 0, 0, 0 }; }

	/// Cost with only type variable component
	static constexpr Cost from_vars(Element vars) { return { 0, 0, vars, 0, 0}; }

	/// Cost with only type specialization component
	static constexpr Cost from_spec(Element spec) { return { 0, 0, 0, spec, 0 }; }

	/// Cost with only safe component
	static constexpr Cost from_safe(Element safe) { return { 0, 0, 0, 0, safe }; }
};

namespace std {
	/// Overload of std::numeric_limits for Cost.
	/// NOTE: Only specializes lowest(), max(), and min()
	template<> struct numeric_limits<Cost> {
		static constexpr Cost lowest() { return Cost::min(); }
		static constexpr Cost max() { return Cost::max(); }
		static constexpr Cost min() { return Cost::min(); }
	};
}

inline Cost operator+ (const Cost& a, const Cost& b) {
	return Cost{ a.unsafe + b.unsafe, 
				 a.poly + b.poly, 
				 a.vars + b.vars, 
				 a.spec + b.spec, 
				 a.safe + b.safe };
}

inline Cost& operator+= (Cost& a, const Cost& b) {
	a.unsafe += b.unsafe;
	a.poly += b.poly;
	a.vars += b.vars;
	a.spec += b.spec;
	a.safe += b.safe;
	return a;
}

inline Cost operator- (const Cost& a, const Cost& b) {
	return Cost{ a.unsafe - b.unsafe, 
				 a.poly - b.poly,
				 a.vars - b.vars,
				 a.spec - b.spec,
				 a.safe - b.safe };
}

inline Cost& operator-= (Cost& a, const Cost& b) {
	a.unsafe -= b.unsafe;
	a.poly -= b.poly;
	a.vars -= b.vars;
	a.spec -= b.spec;
	a.safe -= b.safe;
	return a;
}

/// Comparison between two costs
inline constexpr comparison compare(const Cost& a, const Cost& b) {
	comparison c = eq;
	if ( (c = compare(a.unsafe, b.unsafe)) != 0 ) return c;
	if ( (c = compare(a.poly, b.poly)) != 0 ) return c;
	if ( (c = compare(a.vars, b.vars)) != 0 ) return c;
	if ( (c = compare(a.spec, b.spec)) != 0 ) return (comparison)-c; // inverse ordering
	return compare(a.safe, b.safe);
}

inline bool operator== (const Cost& a, const Cost& b) { return compare(a, b) == 0; }

inline bool operator!= (const Cost& a, const Cost& b) { return compare(a, b) != 0; }

inline bool operator< (const Cost& a, const Cost& b) { return compare(a, b) < 0; }

inline bool operator<= (const Cost& a, const Cost& b) { return compare(a, b) <= 0; }

inline bool operator> (const Cost& a, const Cost& b) { return compare(a, b) > 0; }

inline bool operator>= (const Cost& a, const Cost& b) { return compare(a, b) >= 0; }

inline std::ostream& operator<< (std::ostream& out, const Cost& c) {
	out << "(" << c.unsafe << "," << c.poly << "," << c.vars << ",";
	if ( c.spec > 0 ) { out << "-"; }
	return out << c.spec << "," << c.safe << ")";
}
