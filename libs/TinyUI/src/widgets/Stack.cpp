#include <tiny/ui/widgets/Stack.h>

#include <algorithm>
#include <memory>
#include <utility>
#include <vector>

#include <tiny/core/Rect.h>
#include <tiny/core/Size.h>

#include <tiny/ui/Element.h>
#include <tiny/ui/LayoutContext.h>
#include <tiny/ui/MultiChildElement.h>
#include <tiny/ui/layout/Constraints.h>

namespace tiny {
	namespace {
		std::vector<std::unique_ptr<Element>> createChildElements(const Stack& widget) {
			std::vector<std::unique_ptr<Element>> result;

			const std::vector<std::unique_ptr<Widget>>& widgets = widget.children();

			result.reserve(widgets.size());
			for (const std::unique_ptr<Widget>& child : widgets) {
				if (!child) {
					result.push_back(nullptr);
					continue;
				}

				result.push_back(child->createElement());
			}

			return result;
		}

		std::vector<const Widget*> createChildWidgetPointers(const Stack& widget) {
			std::vector<const Widget*> result;

			const std::vector<std::unique_ptr<Widget>>& widgets = widget.children();

			result.reserve(widgets.size());
			for (const std::unique_ptr<Widget>& child : widgets)
				result.push_back(child.get());

			return result;
		}

		class StackElement final : public MultiChildElement {
		public:
			explicit StackElement(const Stack& widget) : MultiChildElement(widget, createChildElements(widget)) {}

		protected:
			void updateOverride(const Widget& widget) override {
				const Stack& stack = static_cast<const Stack&>(widget);

				updateChildren(createChildWidgetPointers(stack));
				markNeedsLayout();
			}

			Size measureOverride(LayoutContext& context, const Constraints& constraints) override {
				float maximumWidth = 0.0f;
				float maximumHeight = 0.0f;

				Constraints childConstraints = constraints.loosen();
				for (const std::unique_ptr<Element>& child : children()) {
					if (!child)
						continue;

					Size childSize = child->measure(context, childConstraints);
					maximumWidth = std::max(maximumWidth, childSize.width);
					maximumHeight = std::max(maximumHeight, childSize.height);
				}

				return constraints.constrain(Size(maximumWidth, maximumHeight));
			}

			void arrangeOverride(const Rect& bounds) override {
				for (const std::unique_ptr<Element>& child : children()) {
					if (!child)
						continue;

					child->arrange(bounds);
				}
			}
		};
	}

	Stack::Stack(std::vector<std::unique_ptr<Widget>> children, Key key) : Widget(std::move(key)), childWidgets(std::move(children)) { }

	const std::vector<std::unique_ptr<Widget>>& Stack::children() const {
		return childWidgets;
	}

	std::unique_ptr<Element> Stack::createElement() const {
		return std::make_unique<StackElement>(*this);
	}
}