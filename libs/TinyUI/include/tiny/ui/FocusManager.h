#pragma once

namespace tiny {
	class Element;

	class FocusManager {
	public:
		FocusManager() = default;

		FocusManager(const FocusManager&) = delete;
		FocusManager& operator=(const FocusManager&) = delete;

		bool requestFocus(Element* element);

		void clearFocus();

		Element* focusedElement();
		const Element* focusedElement() const;

		void elementWillUnmount(Element* element);

	private:
		Element* focused = nullptr;
	};
}