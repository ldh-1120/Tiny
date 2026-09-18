#pragma once

namespace tiny {
	struct Point {
		float x = 0.0f;
		float y = 0.0f;

		constexpr Point() = default;

		constexpr Point(float x, float y) : x(x), y(y) { }

		constexpr Point operator+(const Point& other) const {
			return Point(x + other.x, y + other.y);
		}

		constexpr Point operator-(const Point& other) const {
			return Point(x - other.x, y - other.y);
		}

		constexpr Point& operator+=(const Point& other) {
			x += other.x;
			y += other.y;
			return *this;
		}

		constexpr Point& operator-=(const Point& other) {
			x -= other.x;
			y -= other.y;
			return *this;
		}

		constexpr bool operator==(const Point& other) const {
			return x == other.x && y == other.y;
		}
	};
}