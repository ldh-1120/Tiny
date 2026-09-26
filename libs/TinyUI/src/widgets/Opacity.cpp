#include <tiny/ui/widgets/Opacity.h>

#include <algorithm>
#include <memory>
#include <utility>

#include <tiny/core/Rect.h>
#include <tiny/core/Size.h>

#include <tiny/graphics/Canvas.h>

#include <tiny/ui/Element.h>
#include <tiny/ui/LayoutContext.h>
#include <tiny/ui/SingleChildElement.h>
#include <tiny/ui/layout/Constraints.h>

namespace tiny {
	namespace {
		class OpacityElement final : public SingleChildElement {
		public:
			explicit OpacityElement(const Opacity& widget) : SingleChildElement(widget, createChild(widget)), opacityValue(widget.opacity()) { }

		protected:
			void updateOverride(const Widget& widget) override {
				const Opacity& opacity = static_cast<const Opacity&>(widget);

				opacityValue = opacity.opacity();

				updateChild(opacity.child());
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

			void paintOverride(Canvas& canvas) override {
				if (!hasChild())
					return;

				if (opacityValue <= 0.0f)
					return;

				if (opacityValue >= 1.0f) {
					SingleChildElement::paintOverride(canvas);
					return;
				}

				bool pushed = canvas.pushOpacity(opacityValue);
				if (!pushed) {
					SingleChildElement::paintOverride(canvas);
					return;
				}

				SingleChildElement::paintOverride(canvas);

				canvas.popOpacity();
			}

		private:
			static std::unique_ptr<Element> createChild(const Opacity& widget) {
				const Widget* childWidget = widget.child();
				if (!childWidget)
					return nullptr;

				return childWidget->createElement();
			}

		private:
			float opacityValue = 1.0f;
		};
	}

	Opacity::Opacity(float opacity, std::unique_ptr<Widget> child, Key key) : Widget(std::move(key)), opacityValue(std::clamp(opacity, 0.0f, 1.0f)), childWidget(std::move(child)) { }

	float Opacity::opacity() const {
		return opacityValue;
	}

	const Widget* Opacity::child() const {
		return childWidget.get();
	}

	std::unique_ptr<Element> Opacity::createElement() const {
		return std::make_unique<OpacityElement>(*this);
	}

}