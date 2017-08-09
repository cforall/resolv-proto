#pragma once

/// Three way comparison result; can be compared to 0 for actual result.
enum comparison {
	lt = -1,
	eq = 0,
	gt = 1
};

/// Bridge from defined operators to comparison
template<typename T>
constexpr comparison compare(T a, T b) { return a < b ? lt : b < a ? gt : eq; }
