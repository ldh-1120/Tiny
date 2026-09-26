#include <tiny/ui/widgets/ClipRect.h>

#include <memory>
#include <utility>

#include <tiny/core/Rect.h>
#include <tiny/core/Size.h>

#include <tiny/graphics/Canvas.h>

#include <tiny/ui/Element.h>
#include <tiny/ui/LayoutContext.h>
#include <tiny/ui/SingleChildElement.h>
#include <tiny/ui/layout/Constraints.h>

namespace tiny {
	namespace {
		class ClipRectElement final : public SingleChildElement {
		public:
			explicit ClipRectElement(const ClipRect& widget) : SingleChildElement(widget, createChild(widget)) { }

		protected:
			void updateOverride(const Widget& widget) override {
				const ClipRect& clipRect = static_cast<const ClipRect&>(widget);

				updateChild(clipRect.child());
				markNeedsLayout();
			}

			Size measureOverride(LayoutContext& context, const Constraints& constraints) override {
				if (!hasChild())
					return constraints.smallest();

				return child()->measure(context, constraints);
			}

			void arrangeOverride(const Rect& bounds) override {
				if (!hasChild())
					return;

				child()->arrange(bounds);
			}

			void paintOverride(Canvas& canvas) override {
				if (!hasChild())
					return;

				canvas.pushClip(bounds());

				SingleChildElement::paintOverride(canvas);

				canvas.popClip();
			}

		private:
			static std::unique_ptr<Element> createChild(const ClipRect& widget) {
				const Widget* childWidget = widget.child();
				if (!childWidget)
					return nullptr;

				return childWidget->createElement();
			}
		};
	}

	ClipRect::ClipRect(std::unique_ptr<Widget> child, Key key) : Widget(std::move(key)), childWidget(std::move(child)) { }

	const Widget* ClipRect::child() const {
		return childWidget.get();
	}

	std::unique_ptr<Element> ClipRect::createElement() const {
		return std::make_unique<ClipRectElement>(*this);
	}
}