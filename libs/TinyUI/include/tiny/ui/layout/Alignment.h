#pragma once

namespace tiny {
    enum class Alignment {
        TopLeft,
        TopCenter,
        TopRight,

        CenterLeft,
        Center,
        CenterRight,

        BottomLeft,
        BottomCenter,
        BottomRight
    };

	enum class CrossAxisAlignment {
		Start,
		Center,
		End,
		Stretch
	};
}