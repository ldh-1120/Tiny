#pragma once

#include <cstddef>
#include <memory>
#include <vector>

#include <tiny/core/Point.h>
#include <tiny/core/Rect.h>
#include <tiny/core/Size.h>

namespace tiny {
	struct TextHitTestResult {
		std::size_t textPosition = 0;

		bool inside = false;
		bool trailing = false;
	};

	struct TextPositionMetrics {
		Point position;

		float height = 0.0f;
	};

	class Canvas;

	class TextLayout {
	public:
		~TextLayout();

		TextLayout(const TextLayout&) = delete;
		TextLayout& operator=(const TextLayout&) = delete;

		TextLayout(TextLayout&&) = delete;
		TextLayout& operator=(TextLayout&&) = delete;

		const Size& size() const;

		float baseline() const;

		std::size_t textLength() const;

		TextHitTestResult hitTestPoint(const Point& point) const;

		TextPositionMetrics hitTestTextPosition(std::size_t textPosition) const;

		std::vector<Rect> hitTestRange(std::size_t start, std::size_t length) const;

		std::size_t previousCaretPosition(std::size_t position) const;
		std::size_t nextCaretPosition(std::size_t position) const;

	private:
		struct Impl;

		explicit TextLayout(std::unique_ptr<Impl> impl);

		std::unique_ptr<Impl> impl;

		friend class GraphicsContext;
		friend class Canvas;
	};
}