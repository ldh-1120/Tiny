#pragma once

namespace tiny {
	struct Size {
		float width = 0.0f;
		float height = 0.0f;

		constexpr Size() = default;

		constexpr Size(float width, float height) : width(width), height(height) { }

		constexpr bool isEmpty() const {
			return width <= 0.0f || height <= 0.0f;
		}

		constexpr bool operator==(const Size& other) const {
			return width == other.width && height == other.height;
		}
	};
 }