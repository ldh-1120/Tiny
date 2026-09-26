#include <tiny/ui/widgets/Positioned.h>

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
		class PositionedElement final : public SingleChildElement {
		public:
			explicit PositionedElement(const Positioned& widget) : SingleChildElement(widget, createChild(widget)), specValue(widget.spec()) { }

			const PositionedSpec& spec() const {
				return specValue;
			}

		protected:
			void updateOverride(const Widget& widget) override {
				const Positioned& positioned = static_cast<const Positioned&>(widget);

				specValue = positioned.spec();

				updateChild(positioned.child());
				markNeedsLayout();
			}

			Size measureOverride(LayoutContext& context, const Constraints& constraints) override {
				if (!hasChild())
					return constraints.smallest();

				Size childSize = child()->measure(context, constraints);
				return constraints.constrain(childSize);
			}

			void arrangeOverride(const Rect& bounds) {
				if (!hasChild())
					return;

				child()->arrange(bounds);
			}

		private:
			static std::unique_ptr<Element> createChild(const Positioned& widget) {
				const Widget* child = widget.child();
				if (!child)
					return nullptr;

				return child->createElement();
			}

		private:
			PositionedSpec specValue;
		};
	}

	namespace detail {
		const PositionedSpec* positionedSpec(const Element& element) {
			const PositionedElement* positioned = dynamic_cast<const PositionedElement*>(&element);
			if (!positioned)
				return nullptr;

			return &positioned->spec();
		}
	}

	Positioned::Positioned(PositionedSpec spec, std::unique_ptr<Widget> child, Key key) : Widget(std::move(key)), positionSpec(std::move(spec)), childWidget(std::move(child)) { }

	const PositionedSpec& Positioned::spec() const {
		return positionSpec;
	}

	const Widget* Positioned::child() const {
		return childWidget.get();
	}

	std::unique_ptr<Element> Positioned::createElement() const {
		return std::make_unique<PositionedElement>(*this);
	}
}