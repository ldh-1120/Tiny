#pragma once

#include <functional>

#include <tiny/core/Point.h>
#include <tiny/core/Size.h>

#include <tiny/graphics/Canvas.h>
#include <tiny/graphics/RenderSurface.h>

#include "EffectSurfaceCache.h"
#include "OffscreenPass.h"

namespace tiny::detail {
	class OffscreenLayer {
	public:
		using PrepareCallback = std::function<bool(RenderSurface&)>;

		OffscreenLayer(Canvas& canvas, EffectSurfaceCache& cache) : canvas(canvas), cache(cache), pass(canvas) {}

		~OffscreenLayer() = default;

		OffscreenLayer(const OffscreenLayer&) = delete;
		OffscreenLayer& operator=(const OffscreenLayer&) = delete;

		OffscreenLayer(OffscreenLayer&&) = delete;
		OffscreenLayer& operator=(OffscreenLayer&&) = delete;

		bool begin(const Size& size, const Point& origin, bool clear = true) {
			return begin(size, origin, clear, PrepareCallback());
		}

		bool begin(const Size& size, const Point& origin, bool clear, const PrepareCallback& prepare) {
			if (pass.active())
				return false;

			surfaceValue = cache.ensure(canvas, size);
			if (!surfaceValue)
				return false;

			if (tryBegin(*surfaceValue, origin, clear, prepare))
				return true;

			surfaceValue = cache.recreate(canvas, size);
			if (!surfaceValue)
				return false;

			return tryBegin(*surfaceValue, origin, clear, prepare);
		}

		void end() {
			pass.end();
		}

		bool active() const {
			return pass.active();
		}

		RenderSurface* surface() {
			return surfaceValue;
		}

		const RenderSurface* surface() const {
			return surfaceValue;
		}

	private:
		bool tryBegin(RenderSurface& surface, const Point& origin, bool clear, const PrepareCallback& prepare) {
			if (prepare && !prepare(surface))
				return false;

			return pass.begin(surface, origin, clear);
		}

	private:
		Canvas& canvas;
		EffectSurfaceCache& cache;

		OffscreenPass pass;

		RenderSurface* surfaceValue = nullptr;
	};
}