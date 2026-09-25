#include <tiny/ui/widgets/ScrollView.h>

#include <algorithm>
#include <limits>
#include <memory>
#include <utility>

#include <tiny/core/Rect.h>
#include <tiny/core/Size.h>

#include <tiny/graphics/Canvas.h>

#include <tiny/ui/Element.h>
#include <tiny/ui/LayoutContext.h>
#include <tiny/ui/SingleChildElement.h>
#include <tiny/ui/layout/Constraints.h>
#include <tiny/ui/scroll/ScrollModel.h>

namespace tiny {
	namespace {
		class ScrollViewElement final : public SingleChildElement {
		public:
			explicit ScrollViewElement(const ScrollView& widget) : SingleChildElement(widget, createChild(widget)), wheelStepValue(widget.wheelStep()), scrollViewStyle(widget.style()) { }

		protected:
			void updateOverride(const Widget& widget) override {
				const ScrollView& scrollView = static_cast<const ScrollView&>(widget);

				wheelStepValue = scrollView.wheelStep();
				scrollViewStyle = scrollView.style();

				updateChild(scrollView.child());
				markNeedsPaint();
			}

			Size measureOverride(LayoutContext& context, const Constraints& constraints) override {
				Size childSize;
				
				if (hasChild()) {
					Constraints childConstraints(0.0f, constraints.maxWidth(), 0.0f, std::numeric_limits<float>::infinity());
					childSize = child()->measure(context, childConstraints);
				}

				return constraints.constrain(childSize);
			}

			void arrangeOverride(const Rect& bounds) override {
				arrangeChild(bounds);
			}

			void paintOverride(Canvas& canvas) override {
				if (!hasChild())
					return;

				canvas.pushClip(bounds());
				
				child()->paint(canvas);
				paintScrollbar(canvas);

				canvas.popClip();
			}

			bool acceptsPointerEvents() const override {
				return true;
			}

			bool pointerDownOverride(const PointerEvent& event) override {
				if (event.button != PointerButton::Left)
					return false;

				if (!scrollbarInteractive())
					return false;

				Rect thumbHit = scrollbarThumbHitBounds();
				if (!thumbHit.contains(event.position))
					return false;

				draggingScrollbar = true;

				dragPointerStartY = event.position.y;
				dragOffsetStart = scrollModel.offset();

				markNeedsPaint();

				return true;
			}

			void pointerMoveOverride(const PointerEvent& event) override {
				if (!draggingScrollbar)
					return;

				Rect track = scrollbarTrackBounds();
				Rect thumb = scrollbarThumbBounds();

				float travelExtent = track.height - thumb.height;
				if (travelExtent <= 0.0f)
					return;

				float pointerDelta = event.position.y - dragPointerStartY;
				float scrollDelta = pointerDelta / travelExtent * scrollModel.maximumOffset();

				bool changed = scrollModel.scrollTo(dragOffsetStart + scrollDelta);
				if (!changed)
					return;

				arrangeChild(bounds());
				markNeedsPaint();
			}

			void pointerUpOverride(const PointerEvent& event) override {
				if (event.button != PointerButton::Left)
					return;

				if (!draggingScrollbar)
					return;

				draggingScrollbar = false;

				markNeedsPaint();
			}

			void pointerCancelOverride() override {
				if (!draggingScrollbar)
					return;

				draggingScrollbar = false;

				markNeedsPaint();
			}

			bool pointerWheelOverride(const PointerWheelEvent& event) override {
				if (!hasChild())
					return false;

				if (event.delta == 0.0f)
					return false;

				bool changed = scrollModel.scrollBy(-event.delta * wheelStepValue);
				if (!changed)
					return false;

				arrangeChild(bounds());
				markNeedsPaint();

				return true;
			}

			Element* hitTestChildren(const Point& position) {
				if (scrollbarInteractive() && scrollbarHitBounds().contains(position))
					return nullptr;

				return SingleChildElement::hitTestChildren(position);
			}

		private:
			static std::unique_ptr<Element> createChild(const ScrollView& widget) {
				const Widget* childWidget = widget.child();

				if (!childWidget)
					return nullptr;

				return childWidget->createElement();
			}

			void arrangeChild(const Rect& viewport) {
				if (!hasChild()) {
					scrollModel.setExtents(viewport.height, 0.0f);
					return;
				}

				float childHeight = child()->desiredSize().height;
				scrollModel.setExtents(viewport.height, childHeight);

				child()->arrange(Rect(viewport.x, viewport.y - scrollModel.offset(), viewport.width, childHeight));
			}

			Rect scrollbarTrackBounds() const {
				const Rect& area = bounds();

				float margin = std::max(scrollViewStyle.scrollbarMargin, 0.0f);
				float availableWidth = std::max(area.width - margin * 2.0f, 0.0f);

				float width = std::min(std::max(scrollViewStyle.scrollbarWidth, 0.0f), availableWidth);
				float height = std::max(area.height - margin * 2.0f, 0.0f);

				return Rect(area.x + area.width - margin - width, area.y + margin, width, height);
			}

			Rect scrollbarHitBounds() const {
				const Rect& area = bounds();

				Rect track = scrollbarTrackBounds();

				float hitWidth = std::max(scrollViewStyle.scrollbarHitWidth, track.width);
				hitWidth = std::min(hitWidth, area.width);

				float centerX = track.x + track.width * 0.5f;
				float x = centerX - hitWidth * 0.5f;

				return Rect(x, area.y, hitWidth, area.height);
			}

			Rect scrollbarThumbBounds() const {
				Rect track = scrollbarTrackBounds();
				if (track.width <= 0.0f || track.height <= 0.0f)
					return Rect();

				float thumbExtent = track.height * scrollModel.viewportFraction();

				float minimumThumbExtent = std::min(std::max(scrollViewStyle.minimumThumbExtent, 0.0f), track.height);
				thumbExtent = std::clamp(thumbExtent, minimumThumbExtent, track.height);

				float travelExtent = std::max(track.height - thumbExtent, 0.0f);
				float thumbY = track.y + travelExtent * scrollModel.offsetFraction();

				return Rect(track.x, thumbY, track.width, thumbExtent);
			}

			Rect scrollbarThumbHitBounds() const {
				Rect hitArea = scrollbarHitBounds();
				Rect thumb = scrollbarThumbBounds();

				return Rect(hitArea.x, thumb.y, hitArea.y, thumb.height);
			}

			void paintScrollbar(Canvas& canvas) {
				if (!scrollViewStyle.showScrollbar)
					return;

				if (!scrollModel.canScroll())
					return;

				Rect track = scrollbarTrackBounds();
				if (track.width <= 0.0f || track.height <= 0.0f)
					return;

				canvas.fillRect(track, scrollViewStyle.trackColor);

				Rect thumb = scrollbarThumbBounds();
				if (thumb.width <= 0.0f || thumb.height <= 0.0f)
					return;

				Color thumbColor = draggingScrollbar ? scrollViewStyle.pressedThumbColor : scrollViewStyle.thumbColor;
				canvas.fillRect(thumb, thumbColor);
			}

			bool scrollbarInteractive() const {
				return scrollViewStyle.showScrollbar && scrollModel.canScroll();
			}

		private:
			float wheelStepValue = 48.0f;

			ScrollModel scrollModel;

			ScrollViewStyle scrollViewStyle;

			bool draggingScrollbar = false;

			float dragPointerStartY = 0.0f;
			float dragOffsetStart = 0.0f;
		};
	}

	ScrollView::ScrollView(std::unique_ptr<Widget> child, float wheelStep, Key key, ScrollViewStyle style) : Widget(std::move(key)), childWidget(std::move(child)), wheelStepValue(std::max(wheelStep, 0.0f)), scrollViewStyle(std::move(style)) { }

	const Widget* ScrollView::child() const {
		return childWidget.get();
	}

	float ScrollView::wheelStep() const {
		return wheelStepValue;
	}

	const ScrollViewStyle& ScrollView::style() const {
		return scrollViewStyle;
	}

	std::unique_ptr<Element> ScrollView::createElement() const {
		return std::make_unique<ScrollViewElement>(*this);
	}
}