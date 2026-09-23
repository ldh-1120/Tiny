#pragma once

#include <string_view>

#include <tiny/core/Color.h>
#include <tiny/core/Rect.h>

#include <tiny/graphics/TextStyle.h>

namespace tiny {
	class WindowRenderer;
	class TextLayout;

	class Canvas {
	public:
		Canvas(const Canvas&) = delete;
		Canvas& operator=(const Canvas&) = delete;

		void clear(const Color& color);

		void fillRect(const Rect& rect, const Color& color);
		void drawRect(const Rect& rect, const Color& color, float strokeWidth = 1.0f);
		void drawTextLayout(const TextLayout& layout, const Point& origin, const Color& color);

		void pushClip(const Rect& rect);
		void popClip();

	private:
		Canvas(void* renderTarget, void* textFactory, void* solidBrush);

	private:
		void* renderTarget = nullptr;
		void* textFactory = nullptr;
		void* solidBrush = nullptr;

		friend class WindowRenderer;
	};
}