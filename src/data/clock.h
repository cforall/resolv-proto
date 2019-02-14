#pragma once

#include <ctime>

static inline long ms_between(std::clock_t start, std::clock_t end) {
	return (end - start) / (CLOCKS_PER_SEC / 1000);
}
