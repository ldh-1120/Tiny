#include <tiny/ui/FocusManager.h>

#include <tiny/ui/Element.h>

namespace tiny {
	bool FocusManager::requestFocus(Element* element) {
		if (!element)
			return false;

		if (!element->isMounted())
			return false;

		if (!element->canReceiveFocus())
			return false;

		if (focused == element)
			return true;

		if (focused)
			focused->setFocused(false);

		focused = element;

		focused->setFocused(true);

		return true;
	}

	void FocusManager::clearFocus() {
		if (!focused)
			return;

		Element* oldFocused = focused;

		focused = nullptr;

		oldFocused->setFocused(false);
	}

	Element* FocusManager::focusedElement() {
		return focused;
	}

	const Element* FocusManager::focusedElement() const {
		return focused;
	}

	void FocusManager::elementWillUnmount(Element* element) {
		if (focused != element)
			return;

		clearFocus();
	}
}