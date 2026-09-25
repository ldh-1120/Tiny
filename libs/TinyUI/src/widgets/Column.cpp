#include <tiny/ui/widgets/Column.h>

#include <algorithm>
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>
#include <limits>

#include <tiny/core/Rect.h>
#include <tiny/core/Size.h>

#include <tiny/ui/Element.h>
#include <tiny/ui/LayoutContext.h>
#include <tiny/ui/MultiChildElement.h>
#include <tiny/ui/layout/Constraints.h>

namespace tiny {
	namespace {
		std::vector<std::unique_ptr<Element>> createChildElements(const Column& widget) {
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

		std::vector<const Widget*> createChildWidgetPointers(const Column& widget) {
			std::vector<const Widget*> result;

			const std::vector<std::unique_ptr<Widget>>& widgets = widget.children();
			result.reserve(widgets.size());

			for (const std::unique_ptr<Widget>& child : widgets)
				result.push_back(child.get());

			return result;
		}

		class ColumnElement final : public MultiChildElement {
		public:
			explicit ColumnElement(const Column& widget) : MultiChildElement(widget, createChildElements(widget)), childSpacing(widget.spacing()), childAlignment(widget.crossAxisAlignment()) { }

		protected:
			void updateOverride(const Widget& widget) override {
				const Column& column = static_cast<const Column&>(widget);

				childSpacing = column.spacing();
				childAlignment = column.crossAxisAlignment();

				updateChildren(createChildWidgetPointers(column));
			}

			Size measureOverride(LayoutContext& context, const Constraints& constraints) override {
				float maximumChildWidth = 0.0f;
				float totalHeight = 0.0f;

				std::size_t visibleChildCount = 0;

				Constraints childConstraints(0.0f, constraints.maxWidth(), 0.0f, std::numeric_limits<float>::infinity());
				for (const std::unique_ptr<Element>& child : children()) {
					if (!child)
						continue;

					Size childSize = child->measure(context, childConstraints);
					maximumChildWidth = std::max(maximumChildWidth, childSize.width);

					totalHeight += childSize.height;
					++visibleChildCount;
				}

				if (visibleChildCount > 1)
					totalHeight += childSpacing * static_cast<float>(visibleChildCount - 1);

				return constraints.constrain(Size(maximumChildWidth, totalHeight));
			}

			void arrangeOverride(const Rect& bounds) override {
				float currentY = bounds.y;
				for (const std::unique_ptr<Element>& child : children()) {
					if (!child)
						continue;

					Size childSize = child->desiredSize();

					float childWidth = std::min(childSize.width, bounds.width);
					float childHeight = childSize.height;

					float childX = bounds.x;
					switch (childAlignment) {
						case CrossAxisAlignment::Start:
							childX = bounds.x;
							break;

						case CrossAxisAlignment::Center:
							childX = bounds.x + (bounds.width - childWidth) * 0.5f;
							break;

						case CrossAxisAlignment::End:
							childX = bounds.x + bounds.width - childWidth;
							break;

						case CrossAxisAlignment::Stretch:
							childX = bounds.x;
							childWidth = bounds.width;
							break;
					}

					child->arrange(Rect(childX, currentY, childWidth, childHeight));

					currentY += childSize.height + childSpacing;
				}
			}

		private:
			float childSpacing = 0.0f;

			CrossAxisAlignment childAlignment = CrossAxisAlignment::Start;
		};
	}

	Column::Column(std::vector<std::unique_ptr<Widget>> children, float spacing, CrossAxisAlignment crossAxisAlignment, Key key) : Widget(std::move(key)), childWidgets(std::move(children)), childSpacing(std::max(0.0f, spacing)), childAlignment(crossAxisAlignment) { }

	const std::vector<std::unique_ptr<Widget>>& Column::children() const {
		return childWidgets;
	}

	float Column::spacing() const {
		return childSpacing;
	}

	CrossAxisAlignment Column::crossAxisAlignment() const {
		return childAlignment;
	}

	std::unique_ptr<Element> Column::createElement() const {
		return std::make_unique<ColumnElement>(*this);
	}
}