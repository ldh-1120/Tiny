#pragma once

#include <string_view>
#include <vector>
#include <memory>

#include <tiny/core/Color.h>
#include <tiny/core/Rect.h>

#include <tiny/graphics/TextStyle.h>
#include <tiny/graphics/Image.h>

namespace tiny {
	class WindowRenderer;
	class TextLayout;
	class Image;
	class RenderSurface;

	class Canvas {
	public:
		~Canvas();

		Canvas(const Canvas&) = delete;
		Canvas& operator=(const Canvas&) = delete;

		void clear(const Color& color);

		void fillRect(const Rect& rect, const Color& color);
		void drawRect(const Rect& rect, const Color& color, float strokeWidth = 1.0f);
		void drawTextLayout(const TextLayout& layout, const Point& origin, const Color& color);

		void pushClip(const Rect& rect);
		void popClip();

		bool pushOpacity(float opacity);
		void popOpacity();

		void drawImage(const Image& image, const Rect& destination, ImageInterpolation interpolation = ImageInterpolation::Linear);
		void drawImage(const Image& image, const Rect& destination, const Rect& source, ImageInterpolation interpolation = ImageInterpolation::Linear);

		std::unique_ptr<RenderSurface> createRenderSurface(const Size& size);

		bool pushRenderSurface(RenderSurface& surface, const Point& origin);
		void popRenderSurface();

		void drawRenderSurface(const RenderSurface& surface, const Rect& destination, float opacity = 1.0f);

		float dpiScale() const;

	private:
		Canvas(void* renderTarget, void* textFactory, void* solidBrush);

	private:
		void* renderTarget = nullptr;
		void* textFactory = nullptr;
		void* solidBrush = nullptr;

		std::vector<void*> opacityLayers;

		struct RenderSurfaceState {
			void* renderTarget = nullptr;
			void* solidBrush = nullptr;
		};

		std::vector<RenderSurfaceState> renderSurfaceStates;

		friend class WindowRenderer;
	};
}