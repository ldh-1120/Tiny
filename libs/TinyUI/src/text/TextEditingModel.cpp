#include <tiny/ui/text/TextEditingModel.h>

#include <algorithm>
#include <utility>

namespace tiny {
	TextEditingModel::TextEditingModel(std::u32string text) : textValue(std::move(text)) {
		textSelection.collapse(textValue.size());
	}

	const std::u32string& TextEditingModel::text() const {
		return textValue;
	}

	const TextSelection& TextEditingModel::selection() const {
		return textSelection;
	}

	void TextEditingModel::setText(std::u32string text) {
		textValue = std::move(text);

		textSelection.clamp(textValue.size());
	}

	bool TextEditingModel::moveCaretTo(std::size_t index, bool extendSelection) {
		std::size_t target = clampIndex(index);
		if (extendSelection) {
			if (textSelection.caret == target)
				return false;

			textSelection.caret = target;

			return true;
		}

		if (textSelection.isCollapsed() && textSelection.caret == target)
			return false;

		textSelection.collapse(target);

		return true;
	}

	bool TextEditingModel::moveLeft(bool extendSelection) {
		if (!extendSelection) {
			if (!textSelection.isCollapsed())
				return moveCaretTo(textSelection.start(), false);

			if (textSelection.caret == 0)
				return false;
		}

		if (textSelection.caret == 0)
			return false;

		return moveCaretTo(textSelection.caret - 1, extendSelection);
	}

	bool TextEditingModel::moveRight(bool extendSelection) {
		if (!extendSelection) {
			if (!textSelection.isCollapsed())
				return moveCaretTo(textSelection.end(), false);

			if (textSelection.caret >= textValue.size())
				return false;
		}

		if (textSelection.caret >= textValue.size())
			return false;

		return moveCaretTo(textSelection.caret + 1, extendSelection);
	}

	bool TextEditingModel::moveHome(bool extendSelection) {
		return moveCaretTo(0, extendSelection);
	}

	bool TextEditingModel::moveEnd(bool extendSelection) {
		return moveCaretTo(textValue.size(), extendSelection);
	}

	bool TextEditingModel::selectAll() {
		if (textValue.empty())
			return moveCaretTo(0, false);

		if (textSelection.anchor == 0 && textSelection.caret == textValue.size())
			return false;

		textSelection.anchor = 0;
		textSelection.caret = textValue.size();

		return true;
	}

	bool TextEditingModel::eraseSelection() {
		if (textSelection.isCollapsed())
			return false;

		std::size_t startIndex = textSelection.start();
		
		std::size_t length = textSelection.length();
		textValue.erase(startIndex, length);

		textSelection.collapse(startIndex);

		return true;
	}

	bool TextEditingModel::replaceSelection(std::u32string_view text) {
		bool changed = eraseSelection();
		if (text.empty())
			return changed;

		std::size_t insertIndex = textSelection.caret;
		textValue.insert(insertIndex, text.data(), text.size());

		textSelection.collapse(insertIndex + text.size());

		return true;
	}

	bool TextEditingModel::backspace() {
		if (eraseSelection())
			return true;

		if (textSelection.caret == 0)
			return false;

		std::size_t deleteIndex = textSelection.caret - 1;
		textValue.erase(deleteIndex, 1);

		textSelection.collapse(deleteIndex);

		return true;
	}

	bool TextEditingModel::deleteForward() {
		if (eraseSelection())
			return true;

		if (textSelection.caret >= textValue.size())
			return false;

		textValue.erase(textSelection.caret, 1);

		return true;
	}

	std::u32string TextEditingModel::selectedText() const {
		if (textSelection.isCollapsed())
			return std::u32string();

		return textValue.substr(textSelection.start(), textSelection.length());
	}

	bool TextEditingModel::eraseRange(std::size_t start, std::size_t end) {
		start = std::min(start, textValue.size());
		end = std::min(end, textValue.size());

		if (end < start)
			std::swap(start, end);

		if (start == end)
			return false;

		textValue.erase(start, end - start);
		
		textSelection.collapse(start);

		return true;
	}

	std::size_t TextEditingModel::clampIndex(std::size_t index) const {
		return std::min(index, textValue.size());
	}
}