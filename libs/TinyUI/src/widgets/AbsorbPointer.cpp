#include <tiny/ui/widgets/AbsorbPointer.h>

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
		class AbsorbPointerElement final : public SingleChildElement {
        public:
            explicit AbsorbPointerElement(const AbsorbPointer& widget) : SingleChildElement(widget,  createChild(widget)), absorbingValue(widget.absorbing()) { }

		protected:
			void updateOverride(const Widget& widget) override {
				const AbsorbPointer& absorbPointer = static_cast<const AbsorbPointer&>(widget);

				absorbingValue = absorbPointer.absorbing();

				updateChild(absorbPointer.child());
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
				if (absorbingValue)
					return nullptr;

				return SingleChildElement::hitTestChildren(position);
			}

			bool acceptsPointerEvents() const override {
				return absorbingValue;
			}

			bool pointerDownOverride(const PointerEvent&) override {
				return absorbingValue;
			}

			bool pointerWheelOverride(const PointerWheelEvent&) override {
				return absorbingValue;
			}

		private:static std::unique_ptr<Element> createChild(const AbsorbPointer& widget) {
            const Widget* childWidget = widget.child();
            if (!childWidget)
                return nullptr;

            return childWidget-> createElement();
        }

		private:
			bool absorbingValue = true;
		};
	}

	AbsorbPointer::AbsorbPointer(std::unique_ptr<Widget> child, bool absorbing, Key key) : Widget(std::move(key)), childWidget(std::move(child)), absorbingValue(absorbing) { }

	const Widget* AbsorbPointer::child() const {
		return childWidget.get();
	}

	bool AbsorbPointer::absorbing() const {
		return absorbingValue;
	}

	std::unique_ptr<Element> AbsorbPointer::createElement() const {
		return std::make_unique<AbsorbPointerElement>(*this);
	}
}