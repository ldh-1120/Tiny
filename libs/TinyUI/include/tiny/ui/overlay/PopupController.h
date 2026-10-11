#pragma once

#include <memory>
#include <functional>

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

		void openAnchored(UIBuilder builder, float gap = 4.0f);

	private:
		std::shared_ptr<detail::PopupAnchorState> anchorStateHandle() const;

		void openInternal(UIBuilder builder, std::function<bool(const Point&)> onOutsidePointerDown);

	private:
		UIRoot* rootValue = nullptr;
		OverlayEntryId entryId = 0;

		std::shared_ptr<detail::PopupAnchorState> anchorStateValue;

		friend class PopupAnchor;
	};
}