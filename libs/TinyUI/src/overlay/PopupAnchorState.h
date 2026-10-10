#pragma once

#include <tiny/core/Rect.h>

namespace tiny::detail {
	struct PopupAnchorState {
		const void* owner = nullptr;

		Rect bounds;
		bool popupOpen = false;
	};
}