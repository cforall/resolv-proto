#pragma once

#include <limits>
#include <ostream>
#include <utility>

#include "data/compare.h"

/// Cost of an expression interpretation
struct Cost {
	unsigned unsafe;  ///< Unsafe conversion cost
	unsigned poly;    ///< Polymorphic binding conversion cost.
	unsigned safe;    ///< Safe conversion cost
	
	constexpr Cost() : unsafe(0), poly(0), safe(0) {}
	constexpr Cost(unsigned unsafe, unsigned poly, unsigned safe)
		: unsafe(unsafe), poly(poly), safe(safe) {}

	friend void swap(Cost& a, Cost& b) {
		using std::swap;

		swap(a.unsafe, b.unsafe);
		swap(a.poly, b.poly);
		swap(a.safe, b.safe);
	}

	/// Maximum cost value
	static constexpr Cost max() {
		return Cost{ std::numeric_limits<unsigned>::max(),
		             std::numeric_limits<unsigned>::max(),
					 std::numeric_limits<unsigned>::max() };
	}
	
	/// Generates a new cost from an integer difference; the cost will have 
	/// the same magnitude as diff, unsafe if diff is negative, safe if diff 
	/// is positive
	static constexpr Cost from_diff(int diff) {
		return diff < 0 ? Cost{ unsigned(-diff), 0, 0 } : Cost{ 0, 0, unsigned(diff) };
	}

	/// Cost with only unsafe component
	static constexpr Cost from_unsafe(unsigned unsafe) { return Cost{ unsafe, 0, 0 }; }

	/// Cost with only polymorphic component
	static constexpr Cost from_poly(unsigned poly) { return Cost{ 0, poly, 0 }; }

	/// Cost with only safe component
	static constexpr Cost from_safe(unsigned safe) { return Cost{ 0, 0, safe }; }
};

namespace std {
	/// Overload of std::numeric_limits for Cost.
	/// NOTE: Only specializes lowest(), max(), and min()
	template<> struct numeric_limits<Cost> {
		static constexpr Cost lowest() { return Cost{}; }
		static constexpr Cost max() { return Cost::max(); }
		static constexpr Cost min() { return Cost{}; }
	};
}

inline Cost operator+ (const Cost& a, const Cost& b) {
	return Cost{ a.unsafe + b.unsafe, a.poly + b.poly, a.safe + b.safe };
}

inline Cost& operator+= (Cost& a, const Cost& b) {
	a.unsafe += b.unsafe;
	a.poly += b.poly;
	a.safe += b.safe;
	return a;
}

inline Cost operator- (const Cost& a, const Cost& b) {
	return Cost{ a.unsafe - b.unsafe, a.poly - b.poly,  a.safe - b.safe };
}

inline Cost& operator-= (Cost& a, const Cost& b) {
	a.unsafe -= b.unsafe;
	a.poly -= b.poly;
	a.safe -= b.safe;
	return a;
}

/// Comparison between two costs
inline constexpr comparison compare(const Cost& a, const Cost& b) {
	comparison c = eq;
	if ( (c = compare(a.unsafe, b.unsafe)) != 0 ) return c;
	if ( (c = compare(a.poly, b.poly)) != 0 ) return c;
	return compare(a.safe, b.safe);
}

inline bool operator== (const Cost& a, const Cost& b) {
	return a.unsafe == b.unsafe && a.poly == b.poly && a.safe == b.safe;
}

inline bool operator!= (const Cost& a, const Cost& b) { return !(a == b); }

inline bool operator< (const Cost& a, const Cost& b) { return compare(a, b) < 0; }

inline bool operator<= (const Cost& a, const Cost& b) { return compare(a, b) <= 0; }

inline bool operator> (const Cost& a, const Cost& b) { return compare(a, b) > 0; }

inline bool operator>= (const Cost& a, const Cost& b) { return compare(a, b) >= 0; }

inline std::ostream& operator<< (std::ostream& out, const Cost& c) {
	return out << "(" << c.unsafe << "," << c.poly << "," << c.safe << ")";
}

/// Cost of all the bindings in an environment
struct EnvCost {
	unsigned vars;   ///< Number of type variables bound
	unsigned assns;  ///< Number of type assertions bound

	constexpr EnvCost() : vars(0), assns(0) {}
	constexpr EnvCost(unsigned vars, unsigned assns) : vars(vars), assns(assns) {}

	/// Cost with only type variable component
	static constexpr EnvCost from_vars(unsigned vars) { return EnvCost{vars, 0}; }

	/// Cost with only type assertion component
	static constexpr EnvCost from_assns(unsigned assns) { return EnvCost{0, assns}; }
};

inline EnvCost operator+ (const EnvCost& a, const EnvCost& b) {
	return EnvCost{ a.vars + b.vars, a.assns + b.assns };
}

inline EnvCost& operator+= (EnvCost& a, const EnvCost& b) {
	a.vars += b.vars;
	a.assns += b.assns;
	return a;
}

inline EnvCost operator- (const EnvCost& a, const EnvCost& b) {
	return EnvCost{ a.vars - b.vars, a.assns - b.assns };
}

inline EnvCost& operator-= (EnvCost& a, const EnvCost& b) {
	a.vars -= b.vars;
	a.assns -= b.assns;
	return a;
}

/// Comparison between two costs
inline constexpr comparison compare(const EnvCost& a, const EnvCost& b) {
	comparison c = eq;
	if ( (c = compare(a.vars, b.vars)) != 0 ) return c;
	return (comparison)-compare(a.assns, b.assns);
}

inline bool operator== (const EnvCost& a, const EnvCost& b) {
	return a.vars == b.vars && a.assns == b.assns;
}

inline bool operator!= (const EnvCost& a, const EnvCost& b) { return !(a == b); }

inline bool operator< (const EnvCost& a, const EnvCost& b) { return compare(a, b) < 0; }

inline bool operator<= (const EnvCost& a, const EnvCost& b) { return compare(a, b) <= 0; }

inline bool operator> (const EnvCost& a, const EnvCost& b) { return compare(a, b) > 0; }

inline bool operator>= (const EnvCost& a, const EnvCost& b) { return compare(a, b) >= 0; }

inline std::ostream& operator<< (std::ostream& out, const EnvCost& c) {
	return out << "(" << c.vars << "," << c.assns << ")";
}
