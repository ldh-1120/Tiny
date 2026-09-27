#pragma once

#include <memory>

#include <tiny/core/Size.h>
#include <tiny/core/Color.h>

namespace tiny {
	class Canvas;

	class RenderSurface {
	public:
		~RenderSurface();

		RenderSurface(const RenderSurface&) = delete;
		RenderSurface& operator=(const RenderSurface&) = delete;

		RenderSurface(RenderSurface&&) = delete;
		RenderSurface& operator=(RenderSurface&&) = delete;

		const Size& size() const;

		float dpiScale() const;

	private:
		class Impl;

		RenderSurface(const Size& size, float dpiScale, void* context, void* bitmap, void* brush);

		void* contextHandle() const;
		void* bitmapHandle() const;
		void* brushHandle() const;

		void* gaussianBlurEffectHandle(float standardDeviation) const;
		void* shadowEffectHandle(float standardDeviation, const Color& color) const;

	private:
		std::unique_ptr<Impl> impl;

		friend class Canvas;
	};
}