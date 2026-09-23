#pragma once

#include <tiny/core/Color.h>
#include <tiny/core/Rect.h>

namespace tiny {
	class Canvas;

	void paintFocusRing(Canvas& canvas, const Rect& bounds, const Color& color, float width, float visibility);
}