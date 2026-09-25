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
			explicit ScrollViewElement(const ScrollView& widget) : SingleChildElement(widget, createChild(widget)), wheelStepValue(widget.wheelStep()) { }

		protected:
			void updateOverride(const Widget& widget) override {
				const ScrollView& scrollView = static_cast<const ScrollView&>(widget);

				wheelStepValue = scrollView.wheelStep();

				updateChild(scrollView.child());
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

				canvas.popClip();
			}

			bool acceptsPointerEvents() const override {
				return true;
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

		private:
			float wheelStepValue = 48.0f;

			ScrollModel scrollModel;
		};
	}

	ScrollView::ScrollView(std::unique_ptr<Widget> child, float wheelStep, Key key) : Widget(std::move(key)), childWidget(std::move(child)), wheelStepValue(std::max(wheelStep, 0.0f)) { }

	const Widget* ScrollView::child() const {
		return childWidget.get();
	}

	float ScrollView::wheelStep() const {
		return wheelStepValue;
	}

	std::unique_ptr<Element> ScrollView::createElement() const {
		return std::make_unique<ScrollViewElement>(*this);
	}
}