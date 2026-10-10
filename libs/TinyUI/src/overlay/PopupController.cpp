#include <tiny/ui/overlay/PopupController.h>

#include <utility>

#include "PopupAnchorState.h"

namespace tiny {
	PopupController::PopupController(UIRoot& root) : rootValue(&root), anchorStateValue(std::make_shared<detail::PopupAnchorState>()) { }
	PopupController::~PopupController() {
		close();
	}

	void PopupController::open(UIBuilder builder) {
		close();

		if (!rootValue)
			return;

		entryId = rootValue->insertOverlay(std::move(builder));
	}

	void PopupController::close() {
		if (!rootValue)
			return;

		if (entryId == 0)
			return;

		rootValue->removeOverlay(entryId);
		entryId = 0;
	}

	bool PopupController::isOpen() const {
		return entryId != 0;
	}

	bool PopupController::hasAnchor() const {
		return anchorStateValue && anchorStateValue->owner != nullptr;
	}

	Rect PopupController::anchorBounds() const {
		if (!hasAnchor())
			return Rect();

		return anchorStateValue->bounds;
	}

	std::shared_ptr<detail::PopupAnchorState> PopupController::anchorStateHandle() const {
		return anchorStateValue;
	}
}