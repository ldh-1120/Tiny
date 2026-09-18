#pragma once

namespace tiny {
	struct Thickness {
		float left = 0.0f;
		float top = 0.0f;
		float right = 0.0f;
		float bottom = 0.0f;

		constexpr Thickness() = default;

		constexpr Thickness(float value) : left(value), top(value), right(value), bottom(value) { }

		constexpr Thickness(float horizontal, float vertical) : left(horizontal), top(vertical), right(horizontal), bottom(vertical) { }

		constexpr Thickness(float left, float top, float right, float bottom) : left(left), top(top), right(right), bottom(bottom) { }

		constexpr float horizontal() const {
			return left + right;
		}

		constexpr float vertical() const {
			return top + bottom;
		}
	};
}