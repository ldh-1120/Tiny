#include <tiny/ui/widgets/Align.h>

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
		class AlignElement final : public SingleChildElement {
		public:
			explicit AlignElement(const Align& widget) : SingleChildElement(widget, createChild(widget)), alignmentValue(widget.alignment()) { }

		protected:
			void updateOverride(const Widget& widget) override {
				const Align& align = static_cast<const Align&>(widget);

				alignmentValue = align.alignment();

				updateChild(align.child());
				markNeedsLayout();
			}

			Size measureOverride(LayoutContext& context, const Constraints& constraints) override {
				Size childSize;
				if (hasChild())
					childSize = child()->measure(context, constraints.loosen());

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

				float childX = alignedX(bounds, childWidth);
				float childY = alignedY(bounds, childHeight);

				child()->arrange(Rect(childX, childY, childWidth, childHeight));
			}

		private:
			static std::unique_ptr<Element> createChild(const Align& widget) {
				const Widget* childWidget = widget.child();
				if (!childWidget)
					return nullptr;

				return childWidget->createElement();
			}

			float alignedX(const Rect& bounds, float childWidth) const {
				switch (alignmentValue) {
					case Alignment::TopLeft:
					case Alignment::CenterLeft:
					case Alignment::BottomLeft:
						return bounds.x;

					case Alignment::TopCenter:
					case Alignment::Center:
					case Alignment::BottomCenter:
						return bounds.x + (bounds.width - childWidth) * 0.5f;

					case Alignment::TopRight:
					case Alignment::CenterRight:
					case Alignment::BottomRight:
						return bounds.x + bounds.width - childWidth;
				}

				return bounds.x;
			}

			float alignedY(const Rect& bounds, float childHeight) const {
				switch (alignmentValue) {
					case Alignment::TopLeft:
					case Alignment::TopCenter:
					case Alignment::TopRight:
						return bounds.y;

					case Alignment::CenterLeft:
					case Alignment::Center:
					case Alignment::CenterRight:
						return bounds.y + (bounds.height - childHeight) * 0.5f;

					case Alignment::BottomLeft:
					case Alignment::BottomCenter:
					case Alignment::BottomRight:
						return bounds.y + bounds.height - childHeight;
				}

				return bounds.y;
			}

		private:
			Alignment alignmentValue = Alignment::Center;
		};
	}

	Align::Align(std::unique_ptr<Widget> child, Alignment alignment, Key key) : Widget(std::move(key)), childWidget(std::move(child)), alignmentValue(alignment) { }

	const Widget* Align::child() const {
		return childWidget.get();
	}

	Alignment Align::alignment() const {
		return alignmentValue;
	}

	std::unique_ptr<Element> Align::createElement() const {
		return std::make_unique<AlignElement>(*this);
	}
}