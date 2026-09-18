#pragma once

#include <cstddef>
#include <optional>
#include <string>

namespace tiny {
	enum class TextCompositionEventType {
		Started, Updated, Ended
	};

	struct TextCompositionEvent {
		TextCompositionEventType type = TextCompositionEventType::Updated;

		std::u32string text;

		std::optional<std::size_t> caretIndex = 0;

		bool handled = false;
	};
}