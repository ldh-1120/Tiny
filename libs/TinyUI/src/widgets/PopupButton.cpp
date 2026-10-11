#include <tiny/ui/widgets/PopupButton.h>

#include <algorithm>
#include <functional>
#include <memory>
#include <utility>

#include <tiny/ui/Element.h>
#include <tiny/ui/ProxyElement.h>
#include <tiny/ui/overlay/PopupAnchor.h>
#include <tiny/ui/overlay/PopupController.h>

namespace tiny {
	namespace {
		class PopupButtonElement final : public ProxyElement {
		public:
			explicit PopupButtonElement(const PopupButton& widget) : ProxyElement(widget, createChild(widget)) { }

		protected:
			void updateOverride(const Widget& widget) override {
				const PopupButton& popup = static_cast<const PopupButton&>(widget);

				updateChild(popup.anchor());
				markNeedsLayout();
			}

		private:
			static std::unique_ptr<Element> createChild(const PopupButton& widget) {
				const Widget* anchor = widget.anchor();
				if (!anchor)
					return nullptr;

				return anchor->createElement();
			}
		};
	}

	PopupButton::PopupButton(PopupController& controller, std::u32string label, UIBuilder content, ButtonStyle style, float gap, bool enabled, Key key) : Widget(std::move(key)) {
		PopupController* controllerPtr = &controller;

		float safeGap = std::max(gap, 0.0f);
		std::function<void()> onClick = [controllerPtr, content = std::move(content), safeGap]() {
			controllerPtr->toggleAnchored(content, safeGap);
		};

		WidgetPtr trigger = std::make_unique<Button>(std::move(label), std::move(onClick), std::move(style), Key(), enabled);
		anchorWidget = std::make_unique<PopupAnchor>(controller, std::move(trigger));
	}

	const Widget* PopupButton::anchor() const {
		return anchorWidget.get();
	}

	std::unique_ptr<Element> PopupButton::createElement() const {
		return std::make_unique<PopupButtonElement>(*this);
	}
}