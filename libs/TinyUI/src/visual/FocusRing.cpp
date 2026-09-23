#include <tiny/ui/visual/FocusRing.h>

#include <tiny/graphics/Canvas.h>

#include <algorithm>
#include <cmath>

namespace tiny {
	void paintFocusRing(Canvas& canvas, const Rect& bounds, const Color& color, float width, float visibility) {
		if (!std::isfinite(width) || width <= 0.0f)
			return;

		if (!std::isfinite(visibility))
			return;

		float opacity = std::clamp(visibility, 0.0f, 1.0f);
		if (opacity <= 0.0f)
			return;

		Color focusColor = withOpacity(color, visibility);
		canvas.drawRect(bounds, focusColor, width);
	}
}