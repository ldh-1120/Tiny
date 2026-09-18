#include <tiny/ui/widgets/Padding.h>

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
		class PaddingElement final : public SingleChildElement {
		public:
			explicit PaddingElement(const Padding& widget) : SingleChildElement(widget, createChild(widget)), paddingValue(widget.padding()) { }

		protected:
			void updateOverride(const Widget& widget) override {
				const Padding& padding = static_cast<const Padding&>(widget);
				paddingValue = padding.padding();

				updateChild(padding.child());
			}

			Size measureOverride(LayoutContext& context, const Constraints& constraints) override {
				float horizontalPadding = paddingValue.horizontal();
				float verticalPadding = paddingValue.vertical();

				float maxChildWidth = constraints.hasBoundedWidth() ? std::max(constraints.maxWidth() - horizontalPadding, 0.0f) : constraints.maxWidth();
				float maxCHildHeight = constraints.hasBoundedHeight() ? std::max(constraints.maxHeight() - verticalPadding, 0.0f) : constraints.maxHeight();

				Size childSize;
				if (hasChild()) {
					Constraints childConstraints(0.0f, maxChildWidth, 0.0f, maxCHildHeight);
					childSize = child()->measure(context, childConstraints);
				}

				return constraints.constrain(Size(childSize.width + horizontalPadding, childSize.height + verticalPadding));
			}

			void arrangeOverride(const Rect& bounds) override {
				if (!hasChild())
					return;

				float childWidth = std::max(bounds.width - paddingValue.horizontal(), 0.0f);
				float childHeight = std::max(bounds.height - paddingValue.vertical(), 0.0f);
				child()->arrange(Rect(bounds.x + paddingValue.left, bounds.y + paddingValue.top, childWidth, childHeight));
			}

		private:
			static std::unique_ptr<Element> createChild(const Padding& widget) {
				const Widget* child = widget.child();
				if (!child)
					return nullptr;

				return child->createElement();
			}

		private:
			Thickness paddingValue;
		};
	}

	Padding::Padding(const Thickness& padding, std::unique_ptr<Widget> child, Key key) : Widget(std::move(key)), paddingValue(padding), childWidget(std::move(child)) { }

	const Thickness& Padding::padding() const {
		return paddingValue;
	}

	const Widget* Padding::child() const {
		return childWidget.get();
	}

	std::unique_ptr<Element> Padding::createElement() const {
		return std::make_unique<PaddingElement>(*this);
	}
}