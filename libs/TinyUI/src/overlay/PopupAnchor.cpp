#include <tiny/ui/overlay/PopupAnchor.h>

#include <memory>
#include <utility>

#include <tiny/core/Rect.h>

#include <tiny/ui/Element.h>
#include <tiny/ui/ProxyElement.h>
#include <tiny/ui/overlay/PopupController.h>

#include "PopupAnchorState.h"

namespace tiny {
	namespace {
		bool sameRect(const Rect& first, const Rect& second) {
			return first.x == second.x && first.y == second.y && first.width == second.width && first.height == second.height;
		}
	}

	class PopupAnchorElement final : public ProxyElement {
	public:
		explicit PopupAnchorElement(const PopupAnchor& widget) : ProxyElement(widget, createChild(widget)), anchorStateValue(widget.anchorStateValue) {}
		~PopupAnchorElement() override {
			clearAnchor();
		}

	protected:
		void updateOverride(const Widget& widget) override {
			const PopupAnchor& anchor = static_cast<const PopupAnchor&>(widget);
			setAnchorState(anchor.anchorStateValue);

			updateChild(anchor.child());
			markNeedsLayout();
		}

		void arrangeOverride(const Rect& bounds) override {
			ProxyElement::arrangeOverride(bounds);
			if (!anchorStateValue)
				return;

			bool changed = anchorStateValue->owner != this || !sameRect(anchorStateValue->bounds, bounds);

			anchorStateValue->owner = this;
			anchorStateValue->bounds = bounds;

			if (changed && anchorStateValue->popupOpen)
				markNeedsLayout();
		}

	private:
		static std::unique_ptr<Element> createChild(const PopupAnchor& widget) {
			const Widget* childWidget = widget.child();
			if (!childWidget)
				return nullptr;

			return childWidget->createElement();
		}

		void setAnchorState(std::shared_ptr<detail::PopupAnchorState> state) {
			if (anchorStateValue == state)
				return;

			clearAnchor();
			anchorStateValue = std::move(state);
		}

		void clearAnchor() {
			if (anchorStateValue)
				return;

			if (anchorStateValue->owner != this)
				return;

			anchorStateValue->owner = nullptr;
			anchorStateValue->bounds = Rect();
		}

	private:
		std::shared_ptr<detail::PopupAnchorState> anchorStateValue;
	};

	PopupAnchor::PopupAnchor(PopupController& controller, std::unique_ptr<Widget> child, Key key) : Widget(std::move(key)), anchorStateValue(controller.anchorStateHandle()), childWidget(std::move(child)) {}

	const Widget* PopupAnchor::child() const {
		return childWidget.get();
	}

	std::unique_ptr<Element> PopupAnchor::createElement() const {
		return std::make_unique<PopupAnchorElement>(*this);
	}

	std::shared_ptr<detail::PopupAnchorState> PopupAnchor::anchorStateHandle() const {
		return anchorStateValue;
	}
}