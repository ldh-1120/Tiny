#pragma once

#include <tiny/core/Point.h>

#include <tiny/graphics/Canvas.h>
#include <tiny/graphics/RenderSurface.h>

namespace tiny::detail {
	class OffscreenPass {
	public:
		explicit OffscreenPass(Canvas& canvas) : canvas(canvas) {}

		~OffscreenPass() {
			end();
		}

		OffscreenPass(const OffscreenPass&) = delete;
		OffscreenPass& operator=(const OffscreenPass&) = delete;

		OffscreenPass(OffscreenPass&&) = delete;
		OffscreenPass& operator=(OffscreenPass&&) = delete;

		bool begin(RenderSurface& surface, const Point& origin, bool clear = true) {
			if (activeValue)
				return false;

			activeValue = canvas.pushRenderSurface(surface, origin, clear);
			return activeValue;
		}

		void end() {
			if (!activeValue)
				return;

			canvas.popRenderSurface();
			activeValue = false;
		}

		bool active() const {
			return activeValue;
		}

	private:
		Canvas& canvas;

		bool activeValue = false;
	};
}