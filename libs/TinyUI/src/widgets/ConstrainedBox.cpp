#include <tiny/ui/widgets/ConstrainedBox.h>

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
		class ConstrainedBoxElement final : public SingleChildElement {
		public:
			explicit ConstrainedBoxElement(const ConstrainedBox& widget) : SingleChildElement(widget, createChild(widget)), boxConstraints(widget.constraints()) { }

		protected:
			void updateOverride(const Widget& widget) override {
				const ConstrainedBox& box = static_cast<const ConstrainedBox&>(widget);

				boxConstraints = box.constraints();

				updateChild(box.child());
				markNeedsLayout();
			}

			Size measureOverride(LayoutContext& context, const Constraints& constraints) override {
				Constraints effectiveConstraints = boxConstraints.enforce(constraints);
				if (!hasChild())
					return effectiveConstraints.smallest();

				Size childSize = child()->measure(context, effectiveConstraints);
				return constraints.constrain(childSize);
			}

			void arrangeOverride(const Rect& bounds) override {
				if (!hasChild())
					return;

				child()->arrange(bounds);
			}

		private:
			static std::unique_ptr<Element> createChild(const ConstrainedBox& widget) {
				const Widget* childWidget = widget.child();
				if (!childWidget)
					return nullptr;

				return childWidget->createElement();
			}

		private:
			Constraints boxConstraints;
		};
	}

	ConstrainedBox::ConstrainedBox(const Constraints& constraints, std::unique_ptr<Widget> child, Key key) : Widget(std::move(key)), boxConstraints(constraints), childWidget(std::move(child)) { }

	const Constraints& ConstrainedBox::constraints() const {
		return boxConstraints;
	}

	const Widget* ConstrainedBox::child() const {
		return childWidget.get();
	}

	std::unique_ptr<Element> ConstrainedBox::createElement() const {
		return std::make_unique<ConstrainedBoxElement>(*this);
	}
}