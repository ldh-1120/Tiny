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
			explicit FlexibleElement(const Flexible& widget) : SingleChildElement(widget, createChild(widget)) { 
				updateParentData(widget);
			}

		protected:
			void updateOverride(const Widget& widget) override {
				const Flexible& flexible = static_cast<const Flexible&>(widget);

				updateParentData(flexible);

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

			void updateParentData(const Flexible& widget) {
				std::unique_ptr<FlexParentData> data = std::make_unique<FlexParentData>();

				data->flex = widget.flex();
				data->fit = widget.fit();

				setParentData(std::move(data));
			}
		};
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