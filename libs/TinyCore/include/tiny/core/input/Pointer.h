#pragma once

#include <tiny/core/Point.h>

namespace tiny {
	enum class PointerButton {
		None,
		Left,
		Right,
		Middle,
		X1,
		X2
	};

	struct PointerModifiers {
		bool shift = false;
		bool control = false;
		bool alt = false;
	};

	struct PointerEvent {
		Point position;

		PointerButton button = PointerButton::None;

		PointerModifiers modifiers;
	};

	struct PointerWheelEvent {
		Point position;

		float delta = 0.0f;
		
		bool handled = false;
	};
}