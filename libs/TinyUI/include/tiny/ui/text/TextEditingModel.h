#pragma once

#include <cstddef>
#include <string>
#include <string_view>

#include <tiny/ui/text/TextSelection.h>

namespace tiny {
	class TextEditingModel {
	public:
		TextEditingModel() = default;

		explicit TextEditingModel(std::u32string text);

		const std::u32string& text() const;

		const TextSelection& selection() const;

		void setText(std::u32string text);

		bool moveCaretTo(std::size_t index, bool extendSelection);
		bool moveLeft(bool extendSelection);
		bool moveRight(bool extendSelection);
		bool moveHome(bool extendSelection);
		bool moveEnd(bool extendSelection);

		bool selectAll();

		bool eraseSelection();
		bool replaceSelection(std::u32string_view text);

		bool backspace();
		bool deleteForward();

		std::u32string selectedText() const;

		bool eraseRange(std::size_t start, std::size_t end);

	private:
		std::size_t clampIndex(std::size_t index) const;

	private:
		std::u32string textValue;

		TextSelection textSelection;
	};
}