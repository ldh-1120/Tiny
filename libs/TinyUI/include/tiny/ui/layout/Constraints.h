#pragma once

#include <algorithm>
#include <limits>
#include <optional>

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

        static constexpr Constraints tightFor(std::optional<float> width = std::nullopt, std::optional<float> height = std::nullopt) {
            float minimumWidth = width.has_value() ? std::max(width.value(), 0.0f) : 0.0f;
            float maximumWidth = width.has_value() ? std::max(width.value(), 0.0f) : infinity();

            float minimumHeight = height.has_value() ? std::max(height.value(), 0.0f) : 0.0f;
            float maximumHeight = height.has_value() ? std::max(height.value(), 0.0f) : infinity();

            return Constraints(minimumWidth, maximumWidth, minimumHeight, maximumHeight);
        }

        static constexpr Constraints fixedWidth(float width) {
            return tightFor(width, std::nullopt);
        }

        static constexpr Constraints fixedHeight(float height) {
            return tightFor(std::nullopt, height);
        }

        static constexpr Constraints widthRange(float minimumWidth, float maximumWidth) {
            return Constraints(minimumWidth, maximumWidth, 0.0f, infinity());
        }

        static constexpr Constraints heightRange(float minimumHeight, float maximumHeight) {
            return Constraints(0.0f, infinity(), minimumHeight, maximumHeight);
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

        constexpr Constraints enforce(const Constraints& parent) {
            return Constraints(
                std::clamp(minimumWidth, parent.minimumWidth, parent.maximumWidth), std::clamp(maximumWidth, parent.minimumWidth, parent.maximumWidth),
                std::clamp(minimumHeight, parent.minimumHeight, parent.maximumHeight), std::clamp(maximumHeight, parent.minimumHeight, parent.maximumHeight));
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