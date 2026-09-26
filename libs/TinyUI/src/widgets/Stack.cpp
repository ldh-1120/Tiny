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
#include <tiny/ui/widgets/Positioned.h>

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

					if (detail::positionedSpec(*child))
						continue;

					Size childSize = child->measure(context, childConstraints);
					maximumWidth = std::max(maximumWidth, childSize.width);
					maximumHeight = std::max(maximumHeight, childSize.height);
				}

				Size stackSize = constraints.constrain(Size(maximumWidth, maximumHeight));
				for (const std::unique_ptr<Element>& child : children()) {
					if (!child)
						continue;

					const PositionedSpec* spec = detail::positionedSpec(*child);
					if (!spec)
						continue;

					child->measure(context, positionedConstraints(*spec, stackSize));
				}

				return stackSize;
			}

			void arrangeOverride(const Rect& bounds) override {
				for (const std::unique_ptr<Element>& child : children()) {
					if (!child)
						continue;

					const PositionedSpec* spec = detail::positionedSpec(*child);
					if (!spec) {
						child->arrange(bounds);
						continue;
					}

					Size desired = child->desiredSize();

					float width = desired.width;
					float height = desired.height;

					if (spec->width.has_value())
						width = std::max(spec->width.value(), 0.0f);
					else if (spec->left.has_value() && spec->right.has_value())
						width = std::max(bounds.width - spec->left.value() - spec->right.value(), 0.0f);
					
					if (spec->height.has_value())
						height = std::max(spec->height.value(), 0.0f);
					else if (spec->top.has_value() && spec->bottom.has_value())
						height = std::max(bounds.height - spec->top.value() - spec->bottom.value(), 0.0f);

					float x = bounds.x;
					float y = bounds.y;

					if (spec->left.has_value())
						x += spec->left.value();
					else if (spec->right.has_value())
						x = bounds.x + bounds.width - spec->right.value() - width;

					if (spec->top.has_value())
						y += spec->top.value();
					else if (spec->bottom.has_value())
						y = bounds.y + bounds.height - spec->bottom.value() - height;

					child->arrange(Rect(x, y, width, height));
				}
			}

		private:
			Constraints positionedConstraints(const PositionedSpec& spec, const Size& stackSize) const {
				float minimumWidth = 0.0f;
				float maximumWidth = stackSize.width;

				float minimumHeight = 0.0f;
				float maximumHeight = stackSize.height;

				if (spec.width.has_value()) {
					float width = std::max(spec.width.value(), 0.0f);
					minimumWidth = width;
					maximumWidth = width;
				} else if (spec.left.has_value() && spec.right.has_value()) {
					float width = std::max(stackSize.width - spec.left.value() - spec.right.value(), 0.0f);
					minimumWidth = width;
					maximumWidth = width;
				} else {
					float inset = 0.0f;
					if (spec.left.has_value())
						inset += spec.left.value();

					if (spec.right.has_value())
						inset += spec.right.value();

					maximumWidth = std::max(stackSize.width - inset, 0.0f);
				}

				if (spec.height.has_value()) {
					float height = std::max(spec.height.value(), 0.0f);
					minimumHeight = height;
					maximumHeight = height;
				} else if (spec.left.has_value() && spec.right.has_value()) {
					float height = std::max(stackSize.height - spec.top.value() - spec.bottom.value(), 0.0f);
					minimumHeight = height;
					maximumHeight = height;
				} else {
					float inset = 0.0f;
					if (spec.top.has_value())
						inset += spec.left.value();

					if (spec.bottom.has_value())
						inset += spec.right.value();

					maximumHeight = std::max(stackSize.height - inset, 0.0f);
				}

				return Constraints(minimumWidth, minimumHeight, maximumWidth, maximumHeight);
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