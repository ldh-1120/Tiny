#pragma once

#include <tiny/core/Rect.h>

#include <tiny/graphics/Canvas.h>
#include <tiny/graphics/RenderSurface.h>

#include "EffectSurfaceCache.h"

namespace tiny::detail {
	class BackdropSnapshot {
	public:
		bool capture(Canvas& canvas, const Rect& bounds) {
			boundsValue = bounds;
			surfaceValue = nullptr;

			if (bounds.isEmpty())
				return true;

			surfaceValue = surfaceCache.ensure(canvas, bounds.size());
			if (!surfaceValue)
				return false;

			if (canvas.captureBackdropSurface(*surfaceValue, bounds))
				return true;

			surfaceValue = surfaceCache.recreate(canvas, bounds.size());
			if (!surfaceValue)
				return false;

			return canvas.captureBackdropSurface(*surfaceValue, bounds);
		}

		RenderSurface* surface() {
			return surfaceValue;
		}

		const RenderSurface* surface() const {
			return surfaceValue;
		}

		const Rect& bounds() const {
			return boundsValue;
		}

		bool available() const {
			return surfaceValue != nullptr && !boundsValue.isEmpty();
		}

	private:
		EffectSurfaceCache surfaceCache;

		RenderSurface* surfaceValue = nullptr;
		Rect boundsValue;
	};

	class BackdropScope {
	public:
		BackdropScope(Canvas& canvas, const BackdropSnapshot& snapshot) : canvas(canvas) {
			if (!snapshot.available())
				return;

			const RenderSurface* surface = snapshot.surface();
			if (!surface)
				return;

			canvas.pushBackdropSurface(*surface, snapshot.bounds());

			activeValue = true;
		}

		~BackdropScope() {
			if (activeValue)
				canvas.popBackdropSurface();
		}

		BackdropScope(const BackdropScope&) = delete;
		BackdropScope& operator=(const BackdropScope&) = delete;

	private:
		Canvas& canvas;

		bool activeValue = false;
	};
}