#pragma once

#include <cmath>
#include <algorithm>

#include <tiny/core/Point.h>
#include <tiny/core/Rect.h>

namespace tiny {
	struct AffineTransform {
		float m11 = 1.0f;
		float m12 = 0.0f;

		float m21 = 0.0f;
		float m22 = 1.0f;

		float dx = 0.0f;
		float dy = 0.0f;

		constexpr AffineTransform() = default;

		constexpr AffineTransform(float m11, float m12, float m21, float m22, float dx, float dy) : m11(m11), m12(m12), m21(m21), m22(m22), dx(dx), dy(dy) {}

		static constexpr AffineTransform identity() {
			return AffineTransform();
		}

		static constexpr AffineTransform translation(float x, float y) {
			return AffineTransform(1.0f, 0.0f, 0.0f, 1.0f, x, y);
		}

		static constexpr AffineTransform scale(float x, float y) {
			return AffineTransform(x, 0.0f, 0.0f, y, 0.0f, 0.0f);
		}

		static AffineTransform rotation(float degrees) {
			constexpr float Pi = 3.14159265358979323846f;
			float radians = degrees * Pi / 180.0f;

			float cosine = std::cos(radians);
			float sine = std::sin(radians);

			return AffineTransform(cosine, sine, -sine, cosine, 0.0f, 0.0f);
		}

		constexpr Point transformPoint(const Point& point) const {
			return Point(point.x * m11 + point.y * m21 + dx, point.x * m12 + point.y * m22 + dy);
		}

		Rect transformBounds(const Rect& rect) const {
			if (rect.isEmpty())
				return Rect();

			Point topLeft = transformPoint(Point(rect.left(), rect.top()));
			Point topRight = transformPoint(Point(rect.right(), rect.top()));

			Point bottomLeft = transformPoint(Point(rect.left(), rect.bottom()));
			Point bottomRight = transformPoint(Point(rect.right(), rect.bottom()));

			float left = std::min(std::min(topLeft.x, topRight.x), std::min(bottomLeft.x, bottomRight.x));
			float top = std::min(std::min(topLeft.y, topRight.y), std::min(bottomLeft.y, bottomRight.y));

			float right = std::max(std::max(topLeft.x, topRight.x), std::max(bottomLeft.x, bottomRight.x));
			float bottom = std::max(std::max(topLeft.y, topRight.y), std::max(bottomLeft.y, bottomRight.y));

			return Rect(left, top, right - left, bottom - top);
		}

		constexpr AffineTransform operator*(const AffineTransform& other) const {
			return AffineTransform(
				m11 * other.m11 + m12 * other.m12, 
				m11 * other.m12 + m12 * other.m22, 

				m21 * other.m11 + m22 * other.m21, 
				m21 * other.m12 + m22 * other.m22, 

				dx * other.m11 + dy * other.m21 + other.dx, 
				dx * other.m12 + dy * other.m22 + other.dy);
		}

		bool tryInverse(AffineTransform& result) const {
			float determinant = m11 * m22 - m12 * m21;

			constexpr float Epsilon = 0.000001f;
			if (std::abs(determinant) <= Epsilon)
				return false;

			float inverseDeterminant = 1.0f / determinant;
			result = AffineTransform(
				m22 * inverseDeterminant,
				-m12 * inverseDeterminant,

				-m21 * inverseDeterminant,
				m11 * inverseDeterminant,

				(dy * m21 - dx * m22) * inverseDeterminant,
				(dx * m12 - dy * m11) * inverseDeterminant);

			return true;
		}
	};
}