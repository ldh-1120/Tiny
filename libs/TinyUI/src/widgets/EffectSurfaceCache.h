#pragma once

#include <cmath>
#include <memory>

#include <tiny/core/Size.h>

#include <tiny/graphics/Canvas.h>
#include <tiny/graphics/RenderSurface.h>

namespace tiny::detail {
	class EffectSurfaceCache {
	public:
		RenderSurface* ensure(Canvas& canvas, const Size& size) {
			if (size.isEmpty()) {
				surface.reset();
				return nullptr;
			}

			float dpiScale = canvas.dpiScale();
			if (surface && matches(*surface, size, dpiScale))
				return surface.get();

			surface = canvas.createRenderSurface(size);
			return surface.get();
		}

		RenderSurface* recreate(Canvas& canvas, const Size& size) {
			surface.reset();
			return ensure(canvas, size);
		}

		void reset() {
			surface.reset();
		}

	private:
		static bool matches(const RenderSurface& surface, const Size& size, float dpiScale) {
			constexpr float Epsilon = 0.001f;

			const Size& surfaceSize = surface.size();
			if (std::abs(surfaceSize.width - size.width) > Epsilon)
				return false;

			if (std::abs(surfaceSize.height - size.height) > Epsilon)
				return false;

			if (std::abs(surface.dpiScale() - dpiScale) > Epsilon)
				return false;

			return true;
		}

	private:
		std::unique_ptr<RenderSurface> surface;
	};
}