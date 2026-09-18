#pragma once

#include <string>

namespace tiny {
	struct TextInputEvent {
		std::u32string text;

		bool handled = false;
	};
}