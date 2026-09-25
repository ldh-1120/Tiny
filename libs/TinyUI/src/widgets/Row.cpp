#include <tiny/ui/widgets/Row.h>

#include <algorithm>
#include <cstddef>
#include <limits>
#include <memory>
#include <utility>
#include <vector>

#include <tiny/core/Rect.h>
#include <tiny/core/Size.h>

#include <tiny/ui/Element.h>
#include <tiny/ui/LayoutContext.h>
#include <tiny/ui/MultiChildElement.h>
#include <tiny/ui/layout/Constraints.h>
#include <tiny/ui/widgets/Flexible.h>

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

		class RowElement final : public MultiChildElement {
		public:
			explicit RowElement(const Row& widget) : MultiChildElement(widget, createChildElements(widget)), childSpacing(widget.spacing()), childAlignment(widget.crossAxisAlignment()) { }

		protected:
			void updateOverride(const Widget& widget) override {
				const Row& row = static_cast<const Row&>(widget);

				childSpacing = row.spacing();
				childAlignment = row.crossAxisAlignment();

				updateChildren(createChildWidgetPointers(row));
			}

			Size measureOverride(LayoutContext& context, const Constraints& constraints) override {
				float totalChildWidth = 0.0f;
				float maximumChildHeight = 0.0f;

				float totalFlex = 0.0f;

				std::size_t visibleChildCount = 0;
				bool boundedMainAxis = constraints.hasBoundedWidth();

				Constraints childConstraints(0.0f, std::numeric_limits<float>::infinity(), 0.0f, constraints.maxHeight());
				for (const std::unique_ptr<Element>& child : children()) {
					if (!child)
						continue;

					++visibleChildCount;

					float flex = boundedMainAxis ? detail::flexFactor(*child) : 0.0f;
					if (flex > 0.0f) {
						totalFlex += flex;
						continue;
					}

					Size childSize = child->measure(context, childConstraints);

					totalChildWidth += childSize.width;
					maximumChildHeight = std::max(maximumChildHeight, childSize.height);
				}

				float totalSpacing = 0.0f;
				if (visibleChildCount > 1)
					totalSpacing = childSpacing * static_cast<float>(visibleChildCount - 1);

				float remainingWidth = 0.0f;
				if (boundedMainAxis && totalFlex > 0.0f) {
					remainingWidth = std::max(constraints.maxWidth() - totalChildWidth - totalSpacing, 0.0f);
					for (const std::unique_ptr<Element>& child : children()) {
						if (!child)
							continue;

						float flex = detail::flexFactor(*child);
						if (flex <= 0.0f)
							continue;

						float allocatedWidth = remainingWidth * flex / totalFlex;

						FlexFit fit = detail::flexFit(*child);

						Constraints flexConstraints;
						if (fit == FlexFit::Tight)
							flexConstraints = Constraints(allocatedWidth, allocatedWidth, 0.0f, constraints.maxHeight());
						else
							flexConstraints = Constraints(0.0f, allocatedWidth, 0.0f, constraints.maxHeight());

						Size childSize = child->measure(context, flexConstraints);
						totalChildWidth += childSize.width;

						maximumChildHeight = std::max(maximumChildHeight, childSize.height);
					}
				}

				totalChildWidth += totalSpacing;

				return constraints.constrain(Size(totalChildWidth, maximumChildHeight));
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
					switch (childAlignment) {
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

					currentX += childWidth + childSpacing;
				}
			}

		private:
			float childSpacing = 0.0f;

			CrossAxisAlignment childAlignment = CrossAxisAlignment::Start;
		};
	}

	Row::Row(std::vector<std::unique_ptr<Widget>> children, float spacing, CrossAxisAlignment crossAxisAlignment, Key key)
		: Widget(std::move(key)), childWidgets(std::move(children)), childSpacing(std::max(spacing, 0.0f)), childAlignment(crossAxisAlignment) { }

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