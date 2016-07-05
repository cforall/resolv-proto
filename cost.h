#pragma once

#include <ostream>

/// Cost of an expression interpretation
struct Cost {
	unsigned unsafe;  ///< Unsafe conversion cost
	unsigned safe;    ///< Safe conversion cost
	
	Cost() : unsafe(0), safe(0) {}
	Cost(unsigned unsafe, unsigned safe) : unsafe(unsafe), safe(safe) {}
	
	Cost(const Cost&) = default;
	Cost(Cost&&) = default;
	Cost& operator= (const Cost&) = default;
	Cost& operator= (Cost&&) = default;
	
	/// Generates a new cost from an integer difference; the cost will have 
	/// the same magnitude as diff, unsafe if diff is negative, safe if diff 
	/// is positive
	static Cost from_diff(int diff) {
		return diff < 0 ? Cost{ unsigned(-diff), 0 } : Cost{ 0, unsigned(diff) };
	} 
};

inline Cost operator+ (const Cost& a, const Cost& b) {
	return Cost{ a.unsafe + b.unsafe, a.safe + b.safe };
}

inline Cost& operator+= (Cost& a, const Cost& b) {
	a.unsafe += b.unsafe;
	a.safe += b.safe;
	return a;
}

inline Cost operator- (const Cost& a, const Cost& b) {
	return Cost{ a.unsafe - b.unsafe,  a.safe - b.safe };
}

inline bool operator== (const Cost& a, const Cost& b) {
	return a.unsafe == b.unsafe && a.safe == b.safe;
}

inline bool operator!= (const Cost& a, const Cost& b) {
	return !(a == b);
}

inline bool operator< (const Cost& a, const Cost& b) {
	return a.unsafe < b.unsafe 
		|| ( a.unsafe == b.unsafe && a.safe < b.safe );
}

inline bool operator<= (const Cost& a, const Cost& b) {
	return a.unsafe < b.unsafe 
		|| ( a.unsafe == b.unsafe && a.safe <= b.safe );
}

inline bool operator> (const Cost& a, const Cost& b) {
	return b < a;
}

inline bool operator>= (const Cost& a, const Cost& b) {
	return b <= a;
}

inline std::ostream& operator<< (std::ostream& out, const Cost& c) {
	out << "(" << c.unsafe << "," << c.safe << ")";
	
	return out;
}
