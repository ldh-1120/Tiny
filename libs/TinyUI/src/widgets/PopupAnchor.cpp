#include <tiny/ui/widgets/PopupAnchor.h>

#include <memory>
#include <utility>

#include <tiny/ui/ProxyElement.h>
#include <tiny/ui/overlay/PopupController.h>

#include "../overlay/PopupAnchorState.h"

namespace tiny {
	class PopupAnchorElement final : public ProxyElement {
	public:
		explicit PopupAnchorElement(const PopupAnchor& widget) : ProxyElement(widget, createChild(widget)), anchorStateValue(widget.anchorStateValue) {}
		~PopupAnchorElement() override {
			detachAnchor();
		}

	protected:
		void updateOverride(const Widget& widget) override {
			const PopupAnchor& anchor = static_cast<const PopupAnchor&>(widget);
			if (anchorStateValue != anchor.anchorStateValue) {
				detachAnchor();

				anchorStateValue = anchor.anchorStateValue;
			}

			updateChild(anchor.child());
			markNeedsLayout();
		}

		void arrangeOverride(const Rect& bounds) override {
			ProxyElement::arrangeOverride(bounds);
			if (!anchorStateValue)
				return;

			anchorStateValue->owner = this;
			anchorStateValue->bounds = bounds;
		}

	private:
		static std::unique_ptr<Element> createChild(const PopupAnchor& widget) {
			const Widget* childWidget = widget.child();
			if (!childWidget)
				return nullptr;

			return childWidget->createElement();
		}

		void detachAnchor() {
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
}