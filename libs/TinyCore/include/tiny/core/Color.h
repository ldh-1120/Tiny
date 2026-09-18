#pragma once

#include <algorithm>

namespace tiny {
	struct Color {
		float r = 0.0f;
		float g = 0.0f;
		float b = 0.0f;
		float a = 1.0f;

		constexpr Color() = default;

		constexpr Color(float r, float g, float b, float a = 1.0f) : r(r), g(g), b(b), a(a) { }

		static constexpr Color fromRgb(unsigned char r, unsigned char g, unsigned char b) {
			return Color(static_cast<float>(r) / 255.0f, static_cast<float>(g) / 255.0f, static_cast<float>(b) / 255.0f, 1.0f);
		}

		static constexpr Color fromRgba(unsigned char r, unsigned char g, unsigned char b, unsigned char a) {
			return Color(static_cast<float>(r) / 255.0f, static_cast<float>(g) / 255.0f, static_cast<float>(b) / 255.0f, static_cast<float>(a) / 255.0f);
		}
	};
}