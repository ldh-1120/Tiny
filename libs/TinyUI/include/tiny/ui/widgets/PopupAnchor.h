#pragma once

#include <memory>

#include <tiny/ui/Key.h>
#include <tiny/ui/Widget.h>

namespace tiny {
	namespace detail {
		struct PopupAnchorState;
	}

	class PopupController;
	class PopupAnchorElement;

	class PopupAnchor final : public Widget {
	public:
		PopupAnchor(PopupController& controller, std::unique_ptr<Widget> child, Key key = Key());

		const Widget* child() const;

		std::unique_ptr<Element> createElement() const override;

	private:
		std::shared_ptr<detail::PopupAnchorState> anchorStateValue;

		std::unique_ptr<Widget> childWidget;

		friend class PopupAnchorElement;
	};
}