#pragma once

#include <algorithm>
#include <limits>

#include <tiny/core/Size.h>

namespace tiny {
	class Constraints {
	public:
		constexpr Constraints() : minimumWidth(0.0f), maximumWidth(infinity()), minimumHeight(0.0f), maximumHeight(infinity()) { }

		constexpr Constraints(float minimumWidth, float maximumWidth, float minimumHeight, float maximumHeight)
            : minimumWidth(std::max(minimumWidth, 0.0f)), maximumWidth(std::max(maximumWidth, std::max(minimumWidth, 0.0f))), minimumHeight(std::max(minimumHeight, 0.0f)), maximumHeight(std::max(maximumHeight, std::max(minimumHeight, 0.0f))) { }

		static constexpr Constraints tight(const Size& size) {
			float width = std::max(size.width, 0.0f);
			float height = std::max(size.height, 0.0f);

			return Constraints(width, width, height, height);
		}

		static constexpr Constraints loose(const Size& size) {
			return Constraints(0.0f, std::max(size.width, 0.0f), 0.0f, std::max(size.height, 0.0f));
		}

		static constexpr Constraints unbounded() {
			return Constraints();
		}

        constexpr Constraints loosen() const {
			return Constraints(0.0f, maximumWidth, 0.0f, maximumHeight);
        }

		constexpr Size constrain(const Size& size) const {
			return Size(std::clamp(size.width, minimumWidth, maximumWidth), std::clamp(size.height, minimumHeight, maximumHeight));
		}

		constexpr Size smallest() const {
			return Size(minimumWidth, minimumHeight);
		}

		constexpr Size biggest() const {
			return Size(maximumWidth, maximumHeight);
		}

        constexpr float minWidth() const {
            return minimumWidth;
        }

        constexpr float maxWidth() const {
            return maximumWidth;
        }

        constexpr float minHeight() const {
            return minimumHeight;
        }

        constexpr float maxHeight() const {
            return maximumHeight;
        }

        constexpr bool hasBoundedWidth() const {
            return maximumWidth != infinity();
        }

        constexpr bool hasBoundedHeight() const {
            return maximumHeight != infinity();
        }

        constexpr bool hasTightWidth() const {
            return minimumWidth == maximumWidth;
        }

        constexpr bool hasTightHeight() const {
            return minimumHeight == maximumHeight;
        }

        constexpr bool isTight() const {
            return hasTightWidth() && hasTightHeight();
        }

    private:
        static constexpr float infinity() {
            return std::numeric_limits<float>::infinity();
        }

    private:
        float minimumWidth;
        float maximumWidth;

        float minimumHeight;
        float maximumHeight;
	};
}