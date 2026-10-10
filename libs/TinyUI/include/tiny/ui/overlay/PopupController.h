#pragma once

#include <tiny/ui/UIBuilder.h>
#include <tiny/ui/UIRoot.h>

namespace tiny {
	class PopupController {
	public:
		explicit PopupController(UIRoot& root);
		~PopupController();

		PopupController(const PopupController&) = delete;
		PopupController& operator=(const PopupController&) = delete;

		void open(UIBuilder builder);
		void close();

		bool isOpen() const;

	private:
		UIRoot* rootValue = nullptr;

		OverlayEntryId entryId = 0;
	};
}