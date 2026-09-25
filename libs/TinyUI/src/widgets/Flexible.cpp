#include <tiny/ui/widgets/Flexible.h>

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
		class FlexibleElement final : public SingleChildElement {
		public:
			explicit FlexibleElement(const Flexible& widget) : SingleChildElement(widget, createChild(widget)), flexValue(widget.flex()), fitValue(widget.fit()) { }

			float flex() const {
				return flexValue;
			}

			FlexFit fit() const {
				return fitValue;
			}

		protected:
			void updateOverride(const Widget& widget) override {
				const Flexible& flexible = static_cast<const Flexible&>(widget);

				flexValue = flexible.flex();
				fitValue = flexible.fit();

				updateChild(flexible.child());
				markNeedsLayout();
			}

			Size measureOverride(LayoutContext& context, const Constraints& constraints) override {
				if (!hasChild())
					return constraints.smallest();

				Size childSize = child()->measure(context, constraints);
				return constraints.constrain(childSize);
			}

			void arrangeOverride(const Rect& bounds) override {
				if (!hasChild())
					return;

				child()->arrange(bounds);
			}

		private:
			static std::unique_ptr<Element> createChild(const Flexible& widget) {
				const Widget* childWidget = widget.child();
				if (!childWidget)
					return nullptr;

				return childWidget->createElement();
			}

		private:
			float flexValue = 1.0f;
			FlexFit fitValue = FlexFit::Loose;
		};
	}

	namespace detail {
		float flexFactor(const Element& element) {
			const FlexibleElement* flexible = dynamic_cast<const FlexibleElement*>(&element);
			if (!flexible)
				return 0.0f;

			return flexible->flex();
		}

		FlexFit flexFit(const Element& element) {
			const FlexibleElement* flexible = dynamic_cast<const FlexibleElement*>(&element);
			if (!flexible)
				return FlexFit::Loose;

			return flexible->fit();
		}
	}

	Flexible::Flexible(std::unique_ptr<Widget> child, float flex, FlexFit fit, Key key) : Widget(std::move(key)), childWidget(std::move(child)), flexValue(std::max(flex, 0.0f)), fitValue(fit) { }

	const Widget* Flexible::child() const {
		return childWidget.get();
	}

	float Flexible::flex() const {
		return flexValue;
	}

	FlexFit Flexible::fit() const {
		return fitValue;
	}

	std::unique_ptr<Element> Flexible::createElement() const {
		return std::make_unique<FlexibleElement>(*this);
	}

	Expanded::Expanded(std::unique_ptr<Widget> child, float flex, Key key) : Flexible(std::move(child), flex, FlexFit::Tight, std::move(key)) { }

	Spacer::Spacer(float flex, Key key) : Flexible(nullptr, flex, FlexFit::Tight, std::move(key)) { }
}