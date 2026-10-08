#include <tiny/ui/widgets/ClipRect.h>

#include <memory>
#include <utility>

#include <tiny/core/Rect.h>
#include <tiny/core/Size.h>

#include <tiny/graphics/Canvas.h>

#include <tiny/ui/Element.h>
#include <tiny/ui/LayoutContext.h>
#include <tiny/ui/ProxyElement.h>
#include <tiny/ui/layout/Constraints.h>

namespace tiny {
	namespace {
		class ClipRectElement final : public ProxyElement {
		public:
			explicit ClipRectElement(const ClipRect& widget) : ProxyElement(widget, createChild(widget)) { }

		protected:
			void updateOverride(const Widget& widget) override {
				const ClipRect& clipRect = static_cast<const ClipRect&>(widget);

				updateChild(clipRect.child());
				markNeedsLayout();
			}

			void paintOverride(Canvas& canvas) override {
				if (!hasChild())
					return;

				canvas.pushClip(bounds());

				SingleChildElement::paintOverride(canvas);

				canvas.popClip();
			}

			Rect visualBoundsOverride() const override {
				if (!hasChild())
					return Rect();

				return intersectRect(bounds(), child()->visualBounds());
			}

			Rect backdropReadBoundsOverride() const override {
				if (!hasChild())
					return Rect();

				Rect childBounds = child()->backdropReadBounds();
				if (childBounds.isEmpty())
					return Rect();

				return intersectRect(bounds(), childBounds);
			}

			Element* hitTestChildren(const Point& position) override {
				if (!bounds().contains(position))
					return nullptr;

				return SingleChildElement::hitTestChildren(position);
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