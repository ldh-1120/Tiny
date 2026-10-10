#include <tiny/ui/overlay/PopupController.h>

#include <utility>

namespace tiny {
	PopupController::PopupController(UIRoot& root) : rootValue(&root) { }
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
}