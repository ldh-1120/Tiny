#pragma once

#include <tiny/core/Point.h>
#include <tiny/core/Size.h>

namespace tiny {
	struct Rect {
		float x = 0.0f;
		float y = 0.0f;
		float width = 0.0f;
		float height = 0.0f;

		constexpr Rect() = default;

		constexpr Rect(float x, float y, float width, float height) : x(x), y(y), width(width), height(height) { }

		constexpr Rect(Point position, Size size) : x(position.x), y(position.y), width(size.width), height(size.height) { }

		constexpr float left() const { 
			return x; 
		}

		constexpr float top() const {
			return y;
		}

		constexpr float right() const {
			return x + width;
		}

		constexpr float bottom() const {
			return y + height;
		}

		constexpr Point position() const {
			return Point(x, y);
		}

		constexpr Size size() const {
			return Size(width, height);
		}

		constexpr bool contains(Point point) const {
			return point.x >= left() && point.x <= right() && point.y >= top() && point.y <= bottom();
		}

		constexpr bool intersects(const Rect& other) const {
			return !(other.left() > right() || other.right() < left() || other.top() > bottom() || other.bottom() < top());
		}

		constexpr bool isEmpty() const {
			return width <= 0.0f || height <= 0.0f;
		}
	};
}