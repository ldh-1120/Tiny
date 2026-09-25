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
#include <tiny/ui/animation/AnimationController.h>
#include <tiny/ui/animation/Easing.h>

namespace tiny {
	namespace {
		class ScrollViewElement final : public SingleChildElement {
		public:
			explicit ScrollViewElement(const ScrollView& widget) : SingleChildElement(widget, createChild(widget)), wheelStepValue(widget.wheelStep()), scrollViewStyle(widget.style()), scrollbarOpacity(widget.style().autoHideScrollbar ? 0.0f : 1.0f) { }

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

				Rect scrollbarHit = scrollbarHitBounds();
				if (!scrollbarHit.contains(event.position))
					return false;

				Rect thumbHit = scrollbarThumbHitBounds();
				if (thumbHit.contains(event.position)) {
					draggingScrollbar = true;

					dragPointerStartY = event.position.y;
					dragOffsetStart = scrollModel.offset();

					revealScrollbar();

					return true;
				}

				Rect thumb = scrollbarThumbBounds();
				if (event.position.y < thumb.y) {
					scrollPage(-1.0f);
					return true;
				}

				if (event.position.y > thumb.y + thumb.height) {
					scrollPage(1.0f);
					return true;
				}

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

				revealScrollbar();
				updateScrollbarVisibility();

				markNeedsPaint();
			}

			void pointerEnterOverride(const PointerEvent& event) override {
				(void)event;

				if (pointerInside)
					return;

				pointerInside = true;

				updateScrollbarVisibility();
				markNeedsPaint();
			}

			void pointerLeaveOverride() override {
				if (!pointerInside)
					return;

				pointerInside = false;

				updateScrollbarVisibility();
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
				revealScrollbar();

				return true;
			}

			Element* hitTestChildren(const Point& position) override {
				if (scrollbarInteractive() && scrollbarHitBounds().contains(position))
					return nullptr;

				return SingleChildElement::hitTestChildren(position);
			}

			void frameOverride(const FrameEvent& event) override {
				float deltaSeconds = static_cast<float>(event.delta.count());

				bool needsPaint = false;
				if (scrollbarActivityRemaining > 0.0f && deltaSeconds > 0.0f) {
					scrollbarActivityRemaining = std::max(scrollbarActivityRemaining - deltaSeconds, 0.0f);
					if (scrollbarActivityRemaining == 0.0f)
						updateScrollbarVisibility();
				}

				if (scrollbarOpacity.advance(deltaSeconds))
					needsPaint = true;

				if (needsPaint)
					markNeedsPaint();

				updateFrameDemand();
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
					updateScrollbarVisibility();

					return;
				}

				float childHeight = child()->desiredSize().height;
				scrollModel.setExtents(viewport.height, childHeight);

				child()->arrange(Rect(viewport.x, viewport.y - scrollModel.offset(), viewport.width, childHeight));

				updateScrollbarVisibility();
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
				x = std::clamp(x, area.x, area.x + area.width - hitWidth);

				return Rect(x, track.y, hitWidth, track.height);
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

				return Rect(hitArea.x, thumb.y, hitArea.width, thumb.height);
			}

			void paintScrollbar(Canvas& canvas) {
				if (!scrollViewStyle.showScrollbar)
					return;

				if (!scrollModel.canScroll())
					return;

				Rect track = scrollbarTrackBounds();
				if (track.width <= 0.0f || track.height <= 0.0f)
					return;

				float opacity = scrollbarOpacity.value();
				if (opacity <= 0.001f)
					return;

				canvas.fillRect(track, withOpacity(scrollViewStyle.trackColor, opacity));

				Rect thumb = scrollbarThumbBounds();
				if (thumb.width <= 0.0f || thumb.height <= 0.0f)
					return;

				Color thumbColor = draggingScrollbar ? scrollViewStyle.pressedThumbColor : scrollViewStyle.thumbColor;
				canvas.fillRect(thumb, withOpacity(thumbColor, opacity));
			}

			bool scrollbarInteractive() const {
				if (!scrollViewStyle.showScrollbar)
					return false;

				if (!scrollModel.canScroll())
					return false;

				if (!scrollViewStyle.autoHideScrollbar)
					return true;

				return pointerInside || draggingScrollbar || scrollbarOpacity.value() > 0.01f;
			}

			bool scrollPage(float direction) {
				float pageExtent = scrollModel.viewportExtent() * 0.9f;

				bool changed = scrollModel.scrollBy(pageExtent * direction);
				if (!changed)
					return false;

				arrangeChild(bounds());
				revealScrollbar();

				return true;
			}

			void updateScrollbarVisibility() {
				bool available = scrollViewStyle.showScrollbar && scrollModel.canScroll();
				bool shouldShow = available && (!scrollViewStyle.autoHideScrollbar || pointerInside || draggingScrollbar || scrollbarActivityRemaining > 0.0f);

				float targetOpacity = shouldShow ? 1.0f : 0.0f;
				float duration = shouldShow ? scrollViewStyle.scrollbarFadeInDuration : scrollViewStyle.scrollbarFadeOutDuration;

				scrollbarOpacity.animateTo(targetOpacity, std::max(duration, 0.0f), Easing::EaseOutCubic);
				updateFrameDemand();
			}

			void updateFrameDemand() {
				bool needsFrames = scrollbarOpacity.isRunning() || scrollbarActivityRemaining > 0.0f;
				setFrameUpdatesEnabled(needsFrames);
			}

			void revealScrollbar() {
				if (!scrollViewStyle.showScrollbar || !scrollModel.canScroll())
					return;

				scrollbarActivityRemaining = std::max(scrollViewStyle.scrollbarHideDelay, 0.0f);
				scrollbarOpacity.animateTo(1.0f, std::max(scrollViewStyle.scrollbarFadeInDuration, 0.0f), Easing::EaseOutCubic);

				updateFrameDemand();
				markNeedsPaint();
			}

		private:
			float wheelStepValue = 48.0f;

			ScrollModel scrollModel;

			ScrollViewStyle scrollViewStyle;

			bool draggingScrollbar = false;

			float dragPointerStartY = 0.0f;
			float dragOffsetStart = 0.0f;

			bool pointerInside = false;

			float scrollbarActivityRemaining = 0.0f;

			AnimationController scrollbarOpacity;
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