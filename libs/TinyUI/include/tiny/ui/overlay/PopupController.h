#pragma once

#include <memory>

#include <tiny/core/Rect.h>
#include <tiny/ui/UIBuilder.h>
#include <tiny/ui/UIRoot.h>

namespace tiny {
	namespace detail {
		struct PopupAnchorState;
	}

	class PopupAnchor;

	class PopupController {
	public:
		explicit PopupController(UIRoot& root);
		~PopupController();

		PopupController(const PopupController&) = delete;
		PopupController& operator=(const PopupController&) = delete;

		void open(UIBuilder builder);
		void close();

		bool isOpen() const;

		bool hasAnchor() const;
		Rect anchorBounds() const;

	private:
		std::shared_ptr<detail::PopupAnchorState> anchorStateHandle() const;

		UIRoot* rootValue = nullptr;
		OverlayEntryId entryId = 0;

		std::shared_ptr<detail::PopupAnchorState> anchorStateValue;

		friend class PopupAnchor;
	};
}