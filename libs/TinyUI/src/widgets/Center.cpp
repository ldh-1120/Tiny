#include <tiny/ui/widgets/Center.h>

#include <algorithm>
#include <memory>
#include <utility>

#include <tiny/core/Rect.h>
#include <tiny/core/Size.h>

#include <tiny/ui/Element.h>
#include <tiny/ui/LayoutContext.h>
#include <tiny/ui/SingleChildElement.h>
#include <tiny/ui/layout/Constraints.h>

namespace tiny {
	namespace {
		class CenterElement final : public SingleChildElement {
		public:
			explicit CenterElement(const Center& widget) : SingleChildElement(widget, createChild(widget)) { }

		protected:
			void updateOverride(const Widget& widget) override {
				const Center& center = static_cast<const Center&>(widget);
				updateChild(center.child());
			}

			Size measureOverride(LayoutContext& context, const Constraints& constraints) override {
				Size childSize;
				if (hasChild()) {
					Constraints childConstraints = constraints.loosen();
					childSize = child()->measure(context, childConstraints);
				}

				float width = childSize.width;
				float height = childSize.height;
				if (constraints.hasBoundedWidth())
					width = constraints.maxWidth();

				if (constraints.hasBoundedHeight())
					height = constraints.maxHeight();

				return constraints.constrain(Size(width, height));
			}

			void arrangeOverride(const Rect& bounds) override {
				if (!hasChild())
					return;

				Size childSize = child()->desiredSize();
				float childWidth = std::min(childSize.width, bounds.width);
				float childHeight = std::min(childSize.height, bounds.height);

				float childX = bounds.x + (bounds.width - childWidth) * 0.5f;
				float childY = bounds.y + (bounds.height - childHeight) * 0.5f;
				child()->arrange(Rect(childX, childY, childWidth, childHeight));
			}

		private:
			static std::unique_ptr<Element> createChild(const Center& widget) {
				const Widget* child = widget.child();
				if (!child)
					return nullptr;

				return child->createElement();
			}
		};
	}

	Center::Center(std::unique_ptr<Widget> child, Key key) : Widget(key), childWidget(std::move(child)) { }

	const Widget* Center::child() const {
		return childWidget.get();
	}

	std::unique_ptr<Element> Center::createElement() const {
		return std::make_unique<CenterElement>(*this);
	}
}