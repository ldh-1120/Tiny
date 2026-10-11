#include <tiny/ui/overlay/PopupController.h>

#include <algorithm>
#include <memory>
#include <stdexcept>
#include <utility>

#include <tiny/ui/Element.h>
#include <tiny/ui/LayoutContext.h>
#include <tiny/ui/SingleChildElement.h>
#include <tiny/ui/Widget.h>
#include <tiny/ui/layout/Constraints.h>

#include "PopupAnchorState.h"

namespace tiny {
	namespace {
		class AnchoredPopupWidget final : public Widget {
		public:
			AnchoredPopupWidget(std::shared_ptr<detail::PopupAnchorState> state, WidgetPtr child, float gap) : stateValue(std::move(state)), childWidget(std::move(child)), gapValue(gap) {}

			const std::shared_ptr<detail::PopupAnchorState>& state() const {
				return stateValue;
			}

			const Widget* content() const {
				return childWidget.get();
			}

			float gap() const {
				return gapValue;
			}

			std::unique_ptr<Element> createElement() const override;

		private:
			std::shared_ptr<detail::PopupAnchorState> stateValue;
			WidgetPtr childWidget;
			float gapValue = 0.0f;
		};

		class AnchoredPopupElement final : public SingleChildElement {
		public:
			explicit AnchoredPopupElement(const AnchoredPopupWidget& widget) : SingleChildElement(widget, widget.content() ? widget.content()->createElement() : nullptr), stateValue(widget.state()), gapValue(widget.gap()) {}

		protected:
			void updateOverride(const Widget& widget) override {
				const AnchoredPopupWidget& popup = static_cast<const AnchoredPopupWidget&>(widget);

				stateValue = popup.state();
				gapValue = popup.gap();

				updateChild(popup.content());
				markNeedsLayout();
			}

			Size measureOverride(LayoutContext& context, const Constraints& constraints) override {
				if (!constraints.hasBoundedWidth() || !constraints.hasBoundedHeight())
					throw std::logic_error("AnchoredPopup requires bounded constraints.");

				Size viewport = constraints.biggest();
				if (hasChild())
					child()->measure(context, Constraints::loose(viewport));

				return viewport;
			}

			void arrangeOverride(const Rect& bounds) override {
				if (!hasChild() || !stateValue || !stateValue->owner)
					return;

				Size popupSize = child()->desiredSize();

				float width = std::min(popupSize.width, bounds.width);
				float height = std::min(popupSize.height, bounds.height);

				const Rect& anchor = stateValue->bounds;

				float maximumX = std::max(bounds.x, bounds.right() - width);
				float x = std::clamp(anchor.left(), bounds.left(), maximumX);

				float belowY = anchor.bottom() + gapValue;
				float aboveY = anchor.top() - gapValue - height;

				float y = belowY;
				if (belowY + height > bounds.bottom() && aboveY >= bounds.top())
					y = aboveY;

				float maximumY = std::max(bounds.top(), bounds.bottom() - height);
				y = std::clamp(y, bounds.top(), maximumY);

				child()->arrange(Rect(x, y, width, height));
			}

			void paintOverride(Canvas& canvas) override {
				if (!stateValue || !stateValue->owner)
					return;

				SingleChildElement::paintOverride(canvas);
			}

			Element* hitTestChildren(const Point& position) override {
				if (!stateValue || !stateValue->owner)
					return nullptr;

				return SingleChildElement::hitTestChildren(position);
			}

		private:
			std::shared_ptr<detail::PopupAnchorState> stateValue;
			float gapValue;
		};

		std::unique_ptr<Element> AnchoredPopupWidget::createElement() const {
			return std::make_unique<AnchoredPopupElement>(*this);
		}
	}

	PopupController::PopupController(UIRoot& root) : rootValue(&root), anchorStateValue(std::make_shared<detail::PopupAnchorState>()) {}
	PopupController::~PopupController() {
		close();
	}

	void PopupController::open(UIBuilder builder) {
		close();

		if (!rootValue)
			return;

		entryId = rootValue->insertOverlay(std::move(builder));
		if (anchorStateValue)
			anchorStateValue->popupOpen = entryId != 0;
	}

	void PopupController::openAnchored(UIBuilder builder, float gap) {
		if (!builder.valid() || !hasAnchor())
			return;

		std::shared_ptr<detail::PopupAnchorState> state = anchorStateValue;

		float safeGap = std::max(gap, 0.0f);
		open(UIBuilder([
			builder = std::move(builder),
			state = std::move(state),
			safeGap
		]() -> WidgetPtr {
			return std::make_unique<AnchoredPopupWidget>(state, builder.build(), safeGap);
		}));
	}

	void PopupController::close() {
		if (anchorStateValue)
			anchorStateValue->popupOpen = false;

		if (!rootValue) {
			entryId = 0;
			return;
		}

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