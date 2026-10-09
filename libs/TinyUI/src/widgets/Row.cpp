#include <tiny/ui/widgets/Row.h>

#include <algorithm>
#include <memory>
#include <utility>
#include <vector>

#include <tiny/core/Rect.h>
#include <tiny/core/Size.h>

#include <tiny/ui/Element.h>
#include <tiny/ui/MultiChildElement.h>
#include <tiny/ui/layout/FlexElement.h>

namespace tiny {
	namespace {
		std::vector<std::unique_ptr<Element>> createChildElements(const Row& widget) {
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

		std::vector<const Widget*> createChildWidgetPointers(const Row& widget) {
			std::vector<const Widget*> result;

			const std::vector<std::unique_ptr<Widget>>& widgets = widget.children();
			result.reserve(widgets.size());

			for (const std::unique_ptr<Widget>& child : widgets)
				result.push_back(child.get());

			return result;
		}

		class RowElement final : public FlexElement {
		public:
			explicit RowElement(const Row& widget) : FlexElement(widget, createChildElements(widget), Axis::Horizontal, widget.spacing(), widget.crossAxisAlignment()) {}

		protected:
			void updateOverride(const Widget& widget) override {
				const Row& row = static_cast<const Row&>(widget);

				setSpacing(row.spacing());
				setCrossAxisAlignment(row.crossAxisAlignment());

				updateChildren(createChildWidgetPointers(row));
			}

			void arrangeOverride(const Rect& bounds) override {
				float currentX = bounds.x;
				for (const std::unique_ptr<Element>& child : children()) {
					if (!child)
						continue;

					Size childSize = child->desiredSize();

					float childWidth = childSize.width;
					float childHeight = std::min(childSize.height, bounds.height);

					float childY = bounds.y;
					switch (crossAxisAlignment()) {
						case CrossAxisAlignment::Start:
							childY = bounds.y;
							break;

						case CrossAxisAlignment::Center:
							childY = bounds.y + (bounds.height - childHeight) * 0.5f;
							break;

						case CrossAxisAlignment::End:
							childY = bounds.y + bounds.height - childHeight;
							break;

						case CrossAxisAlignment::Stretch:
							childY = bounds.y;
							childHeight = bounds.height;
							break;
					}

					child->arrange(Rect(currentX, childY, childWidth, childHeight));

					currentX += childWidth + spacing();
				}
			}
		};
	}

	Row::Row(std::vector<std::unique_ptr<Widget>> children, float spacing, CrossAxisAlignment crossAxisAlignment, Key key)
		: Widget(std::move(key)), childWidgets(std::move(children)), childSpacing(std::max(spacing, 0.0f)), childAlignment(crossAxisAlignment) {}

	const std::vector<std::unique_ptr<Widget>>& Row::children() const {
		return childWidgets;
	}

	float Row::spacing() const {
		return childSpacing;
	}

	CrossAxisAlignment Row::crossAxisAlignment() const {
		return childAlignment;
	}

	std::unique_ptr<Element> Row::createElement() const {
		return std::make_unique<RowElement>(*this);
	}
}