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

		void pushBackdropSurface(const RenderSurface& surface, const Rect& bounds);
		void popBackdropSurface();

		bool captureBackdropSurface(RenderSurface& surface, const Rect& sourceBounds);

		void drawImage(const Image& image, const Rect& destination, ImageInterpolation interpolation = ImageInterpolation::Linear);
		void drawImage(const Image& image, const Rect& destination, const Rect& source, ImageInterpolation interpolation = ImageInterpolation::Linear);

		std::unique_ptr<RenderSurface> createRenderSurface(const Size& size);

		bool pushRenderSurface(RenderSurface& surface, const Point& origin, bool clear = true);
		void popRenderSurface();

		void drawRenderSurface(const RenderSurface& surface, const Rect& destination, float opacity = 1.0f);
		void drawBlurredRenderSurface(const RenderSurface& surface, const Point& origin, float standardDeviation);
		void drawShadowRenderSurface(const RenderSurface& surface, const Point& origin, const Point& offset, float standardDeviation, const Color& color);

		bool captureRenderSurface(RenderSurface& surface, const Rect& sourceBounds);

		float dpiScale() const;

		Rect currentPaintBounds() const;

	private:
		Canvas(void* renderTarget, void* textFactory, void* solidBrush);

	private:
		void* renderTarget = nullptr;
		void* textFactory = nullptr;
		void* solidBrush = nullptr;

		std::vector<Rect> clipRects;

		Point renderTargetOrigin;

		struct RenderSurfaceState {
			void* renderTarget = nullptr;
			void* solidBrush = nullptr;

			Point origin;

			std::vector<Rect> clipRects;
		};

		std::vector<RenderSurfaceState> renderSurfaceStates;

		struct BackdropSourceState {
			const RenderSurface* surface = nullptr;
			Rect bounds;
		};

		std::vector<BackdropSourceState> backdropSources;

		friend class WindowRenderer;
	};
}