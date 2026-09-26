#include <tiny/ui/widgets/IgnorePointer.h>

#include <memory>
#include <utility>

#include <tiny/core/Point.h>
#include <tiny/core/Rect.h>
#include <tiny/core/Size.h>

#include <tiny/ui/Element.h>
#include <tiny/ui/LayoutContext.h>
#include <tiny/ui/SingleChildElement.h>
#include <tiny/ui/layout/Constraints.h>

namespace tiny {
	namespace {
		class IgnorePointerElement final : public SingleChildElement {
		public:
			explicit IgnorePointerElement(const IgnorePointer& widget) : SingleChildElement(widget, createChild(widget)), ignoringValue(widget.ignoring()) { }

		protected:
			void updateOverride(const Widget& widget) override {
				const IgnorePointer& ignorePointer = static_cast<const IgnorePointer&>(widget);

				ignoringValue = ignorePointer.ignoring();

				updateChild(ignorePointer.child());
				markNeedsLayout();
			}

			Size measureOverride(LayoutContext& context, const Constraints& constraints) override {
				if (!hasChild())
					return constraints.smallest();

				return child()->measure(context, constraints);
			}

			void arrangeOverride(const Rect& bounds) override {
				if (!hasChild())
					return;

				child()->arrange(bounds);
			}

			Element* hitTestChildren(const Point& position) override {
				if (ignoringValue)
					return nullptr;

				return SingleChildElement::hitTestChildren(position);
			}

		private:
			static std::unique_ptr<Element> createChild(const IgnorePointer& widget) {
				const Widget* childWidget = widget.child();
				if (!childWidget)
					return nullptr;

				return childWidget->createElement();
			}

		private:
			bool ignoringValue = true;
		};
	}

	IgnorePointer::IgnorePointer(std::unique_ptr<Widget> child, bool ignoring, Key key) : Widget(std::move(key)), childWidget(std::move(child)), ignoringValue(ignoring) { }

	const Widget* IgnorePointer::child() const {
		return childWidget.get();
	}

	bool IgnorePointer::ignoring() const {
		return ignoringValue;
	}

	std::unique_ptr<Element> IgnorePointer::createElement() const {
		return std::make_unique<IgnorePointerElement>(*this);
	}
}