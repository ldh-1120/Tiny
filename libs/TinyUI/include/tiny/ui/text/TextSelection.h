#pragma once

#include <algorithm>
#include <cstddef>

namespace tiny {
	struct TextSelection {
		std::size_t anchor = 0;
		std::size_t caret = 0;

		bool isCollapsed() const {
			return anchor == caret;
		}

		std::size_t start() const {
			return std::min(anchor, caret);
		}

		std::size_t end() const {
			return std::max(anchor, caret);
		}

		std::size_t length() const {
			return end() - start();
		}

		void collapse(std::size_t index) {
			anchor = index;
			caret = index;
		}

		void clamp(std::size_t textLength) {
			anchor = std::min(anchor, textLength);
			caret = std::min(caret, textLength);
		}
	};
}