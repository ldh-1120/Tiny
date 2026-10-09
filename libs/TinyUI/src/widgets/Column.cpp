#include <tiny/ui/widgets/Column.h>

#include <algorithm>
#include <memory>
#include <utility>
#include <vector>

#include <tiny/ui/Element.h>
#include <tiny/ui/MultiChildElement.h>
#include <tiny/ui/layout/FlexElement.h>

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

		class ColumnElement final : public FlexElement {
		public:
			explicit ColumnElement(const Column& widget) : FlexElement(widget, createChildElements(widget), Axis::Vertical, widget.spacing(), widget.crossAxisAlignment()) { }

		protected:
			void updateOverride(const Widget& widget) override {
				const Column& column = static_cast<const Column&>(widget);

				setSpacing(column.spacing());
				setCrossAxisAlignment(column.crossAxisAlignment());

				updateChildren(createChildWidgetPointers(column));
			}
		};
	}

	Column::Column(std::vector<std::unique_ptr<Widget>> children, float spacing, CrossAxisAlignment crossAxisAlignment, MainAxisAlignment mainAxisAlignment, Key key)
		: Widget(std::move(key)), childWidgets(std::move(children)), childSpacing(std::max(0.0f, spacing)), childAlignment(crossAxisAlignment), mainAlignment(mainAxisAlignment) { }

	const std::vector<std::unique_ptr<Widget>>& Column::children() const {
		return childWidgets;
	}

	float Column::spacing() const {
		return childSpacing;
	}

	MainAxisAlignment Column::mainAxisAlignment() const {
		return mainAlignment;
	}

	CrossAxisAlignment Column::crossAxisAlignment() const {
		return childAlignment;
	}

	std::unique_ptr<Element> Column::createElement() const {
		return std::make_unique<ColumnElement>(*this);
	}
}