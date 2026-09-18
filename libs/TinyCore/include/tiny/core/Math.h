#pragma once

#include <algorithm>

namespace tiny {
	template<typename T>
	constexpr T clamp(T value, T minimum, T maximum) {
		return std::clamp(value, minimum, maximum);
	}

	template<typename T>
	constexpr T lerp(T start, T end, float amount) {
		return static_cast<T>(start + (end - start) * amount);
	}
}